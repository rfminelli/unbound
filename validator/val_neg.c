/*
 * validator/val_neg.c - validator aggressive negative caching functions.
 *
 * Copyright (c) 2008, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains helper functions for the validator module.
 * The functions help with aggressive negative caching.
 * This creates new denials of existance, and proofs for absence of types
 * from cached NSEC records.
 */
#include "config.h"
#include "validator/val_neg.h"
#include "validator/val_nsec.h"
#include "util/data/dname.h"
#include "util/data/msgreply.h"
#include "util/log.h"
#include "util/net_help.h"
#include "services/cache/rrset.h"

int val_neg_data_compare(const void* a, const void* b)
{
	struct val_neg_data* x = (struct val_neg_data*)a;
	struct val_neg_data* y = (struct val_neg_data*)b;
	int m;
	return dname_canon_lab_cmp(x->name, x->labs, y->name, y->labs, &m);
}

int val_neg_zone_compare(const void* a, const void* b)
{
	struct val_neg_zone* x = (struct val_neg_zone*)a;
	struct val_neg_zone* y = (struct val_neg_zone*)b;
	int m;
	if(x->dclass != y->dclass) {
		if(x->dclass < y->dclass)
			return -1;
		return 1;
	}
	return dname_canon_lab_cmp(x->name, x->labs, y->name, y->labs, &m);
}

struct val_neg_cache* val_neg_create()
{
	struct val_neg_cache* neg = (struct val_neg_cache*)calloc(1, 
		sizeof(*neg));
	if(!neg) {
		log_err("Could not create neg cache: out of memory");
		return NULL;
	}
	neg->max = 1024*1024; /* 1 M is thousands of entries */
	rbtree_init(&neg->tree, &val_neg_zone_compare);
	lock_basic_init(&neg->lock);
	lock_protect(&neg->lock, neg, sizeof(*neg));
	return neg;
}

int val_neg_apply_cfg(struct val_neg_cache* neg, struct config_file* cfg)
{
	/* TODO read max mem size from cfg */
	return 1;
}

size_t val_neg_get_mem(struct val_neg_cache* neg)
{
	size_t result;
	lock_basic_lock(&neg->lock);
	result = sizeof(*neg) + neg->use;
	lock_basic_unlock(&neg->lock);
	return result;
}

/** clear datas on cache deletion */
static void
neg_clear_datas(rbnode_t* n, void* ATTR_UNUSED(arg))
{
	struct val_neg_data* d = (struct val_neg_data*)n;
	free(d->name);
	free(d);
}

/** clear zones on cache deletion */
static void
neg_clear_zones(rbnode_t* n, void* ATTR_UNUSED(arg))
{
	struct val_neg_zone* z = (struct val_neg_zone*)n;
	/* delete all the rrset entries in the tree */
	traverse_postorder(&z->tree, &neg_clear_datas, NULL);
	free(z->name);
	free(z);
}

void neg_cache_delete(struct val_neg_cache* neg)
{
	if(!neg) return;
	lock_basic_destroy(&neg->lock);
	/* delete all the zones in the tree */
	traverse_postorder(&neg->tree, &neg_clear_zones, NULL);
	free(neg);
}

/**
 * Put data element at the front of the LRU list.
 * @param neg: negative cache with LRU start and end.
 * @param data: this data is fronted.
 */
static void neg_lru_front(struct val_neg_cache* neg, 
	struct val_neg_data* data)
{
	data->prev = NULL;
	data->next = neg->first;
	if(!neg->first)
		neg->last = data;
	else	neg->first->prev = data;
	neg->first = data;
}

/**
 * Remove data element from LRU list.
 * @param neg: negative cache with LRU start and end.
 * @param data: this data is removed from the list.
 */
static void neg_lru_remove(struct val_neg_cache* neg, 
	struct val_neg_data* data)
{
	if(data->prev)
		data->prev->next = data->next;
	else	neg->first = data->next;
	if(data->next)
		data->next->prev = data->prev;
	else	neg->last = data->prev;
}

/**
 * Touch LRU for data element, put it at the start of the LRU list.
 * @param neg: negative cache with LRU start and end.
 * @param data: this data is used.
 */
static void neg_lru_touch(struct val_neg_cache* neg, 
	struct val_neg_data* data)
{
	if(data == neg->first)
		return; /* nothing to do */
	/* remove from current lru position */
	neg_lru_remove(neg, data);
	/* add at front */
	neg_lru_front(neg, data);
}

/**
 * Delete a zone element from the negative cache.
 * May delete other zone elements to keep tree coherent, or
 * only mark the element as 'not in use'.
 * @param neg: negative cache.
 * @param z: zone element to delete.
 */
static void neg_delete_zone(struct val_neg_cache* neg, struct val_neg_zone* z)
{
	struct val_neg_zone* p, *np;
	if(!z) return;
	log_assert(z->in_use);
	log_assert(z->count > 0);
	z->in_use = 0;

	/* go up the tree and reduce counts */
	p = z;
	while(p) {
		log_assert(p->count > 0);
		p->count --;
		p = p->parent;
	}

	/* remove zones with zero count */
	p = z;
	while(p && p->count == 0) {
		np = p->parent;
		(void)rbtree_delete(&neg->tree, &p->node);
		neg->use -= p->len + sizeof(*p);
		free(p->name);
		free(p);
		p = np;
	}
}
	
/**
 * Delete a data element from the negative cache.
 * May delete other data elements to keep tree coherent, or
 * only mark the element as 'not in use'.
 * @param neg: negative cache.
 * @param el: data element to delete.
 */
static void neg_delete_data(struct val_neg_cache* neg, struct val_neg_data* el)
{
	struct val_neg_zone* z;
	struct val_neg_data* p, *np;
	if(!el) return;
	z = el->zone;
	log_assert(el->in_use);
	log_assert(el->count > 0);
	el->in_use = 0;

	/* remove it from the lru list */
	neg_lru_remove(neg, el);
	
	/* go up the tree and reduce counts */
	p = el;
	while(p) {
		log_assert(p->count > 0);
		p->count --;
		p = p->parent;
	}

	/* delete 0 count items from tree */
	p = el;
	while(p && p->count == 0) {
		np = p->parent;
		(void)rbtree_delete(&z->tree, &p->node);
		neg->use -= p->len + sizeof(*p);
		free(p->name);
		free(p);
		p = np;
	}

	/* check if the zone is now unused */
	if(z->tree.count == 0) {
		neg_delete_zone(neg, z);
	}
}

/**
 * Create more space in negative cache
 * The oldest elements are deleted until enough space is present.
 * Empty zones are deleted.
 * @param neg: negative cache.
 * @param need: how many bytes are needed.
 */
static void neg_make_space(struct val_neg_cache* neg, size_t need)
{
	/* delete elements until enough space or its empty */
	while(neg->last && neg->max < neg->use + need) {
		neg_delete_data(neg, neg->last);
	}
}

/**
 * Find the given zone, from the SOA owner name and class
 * @param neg: negative cache
 * @param soa: what to look for.
 * @return zone or NULL if not found.
 */
static struct val_neg_zone* neg_find_zone(struct val_neg_cache* neg, 
	struct ub_packed_rrset_key* soa)
{
	struct val_neg_zone lookfor;
	struct val_neg_zone* result;
	lookfor.node.key = &lookfor;
	lookfor.name = soa->rk.dname;
	lookfor.len = soa->rk.dname_len;
	lookfor.labs = dname_count_labels(lookfor.name);
	lookfor.dclass = ntohs(soa->rk.rrset_class);

	result = (struct val_neg_zone*)
		rbtree_search(&neg->tree, lookfor.node.key);
	return result;
}

/**
 * Calculate space needed for the data and all its parents
 * @param rep: NSEC entries.
 * @return size.
 */
static size_t calc_data_need(struct reply_info* rep)
{
	uint8_t* d;
	size_t i, len, res = 0;

	for(i=rep->an_numrrsets; i<rep->an_numrrsets+rep->ns_numrrsets; i++) {
		if(ntohs(rep->rrsets[i]->rk.type) == LDNS_RR_TYPE_NSEC) {
			d = rep->rrsets[i]->rk.dname;
			len = rep->rrsets[i]->rk.dname_len;
			res = sizeof(struct val_neg_data) + len;
			while(!dname_is_root(d)) {
				log_assert(len > 1); /* not root label */
				dname_remove_label(&d, &len);
				res += sizeof(struct val_neg_data) + len;
			}
		}
	}
	return res;
}

/**
 * Calculate space needed for zone and all its parents
 * @param soa: with name.
 * @return size.
 */
static size_t calc_zone_need(struct ub_packed_rrset_key* soa)
{
	uint8_t* d = soa->rk.dname;
	size_t len = soa->rk.dname_len;
	size_t res = sizeof(struct val_neg_zone) + len;
	while(!dname_is_root(d)) {
		log_assert(len > 1); /* not root label */
		dname_remove_label(&d, &len);
		res += sizeof(struct val_neg_zone) + len;
	}
	return res;
}

/**
 * Find closest existing parent zone of the given name.
 * @param neg: negative cache.
 * @param nm: name to look for
 * @param nm_len: length of nm
 * @param labs: labelcount of nm.
 * @param qclass: class.
 * @return the zone or NULL if none found.
 */
static struct val_neg_zone* neg_closest_zone_parent(struct val_neg_cache* neg,
	uint8_t* nm, size_t nm_len, int labs, uint16_t qclass)
{
	struct val_neg_zone key;
	struct val_neg_zone* result;
	rbnode_t* res = NULL;
	key.node.key = &key;
	key.name = nm;
	key.len = nm_len;
	key.labs = labs;
	key.dclass = qclass;
	if(rbtree_find_less_equal(&neg->tree, &key, &res)) {
		/* exact match */
		result = (struct val_neg_zone*)res;
	} else {
		/* smaller element (or no element) */
		int m;
		result = (struct val_neg_zone*)res;
		if(!result || result->dclass != qclass)
			return NULL;
		/* count number of labels matched */
		(void)dname_lab_cmp(result->name, result->labs, key.name,
			key.labs, &m);
		while(result) { /* go up until qname is subdomain of stub */
			if(result->labs <= m)
				break;
			result = result->parent;
		}
	}
	return result;
}

/**
 * Find closest existing parent data for the given name.
 * @param zone: to look in.
 * @param nm: name to look for
 * @param nm_len: length of nm
 * @param labs: labelcount of nm.
 * @return the data or NULL if none found.
 */
static struct val_neg_data* neg_closest_data_parent(
	struct val_neg_zone* zone, uint8_t* nm, size_t nm_len, int labs)
{
	struct val_neg_data key;
	struct val_neg_data* result;
	rbnode_t* res = NULL;
	key.node.key = &key;
	key.name = nm;
	key.len = nm_len;
	key.labs = labs;
	if(rbtree_find_less_equal(&zone->tree, &key, &res)) {
		/* exact match */
		result = (struct val_neg_data*)res;
	} else {
		/* smaller element (or no element) */
		int m;
		result = (struct val_neg_data*)res;
		if(!result)
			return NULL;
		/* count number of labels matched */
		(void)dname_lab_cmp(result->name, result->labs, key.name,
			key.labs, &m);
		while(result) { /* go up until qname is subdomain of stub */
			if(result->labs <= m)
				break;
			result = result->parent;
		}
	}
	return result;
}

/**
 * Create a single zone node
 * @param nm: name for zone (copied)
 * @param nm_len: length of name
 * @param labs: labels in name.
 * @param dclass: class of zone.
 * @return new zone or NULL on failure
 */
static struct val_neg_zone* neg_setup_zone_node(
	uint8_t* nm, size_t nm_len, int labs, uint16_t dclass)
{
	struct val_neg_zone* zone = 
		(struct val_neg_zone*)calloc(1, sizeof(*zone));
	if(!zone) {
		return NULL;
	}
	zone->node.key = zone;
	zone->name = memdup(nm, nm_len);
	if(!zone->name) {
		free(zone);
		return NULL;
	}
	zone->len = nm_len;
	zone->labs = labs;
	zone->dclass = dclass;

	rbtree_init(&zone->tree, &val_neg_data_compare);
	return zone;
}

/**
 * Create a linked list of parent zones, starting at longname ending on
 * the parent (can be NULL, creates to the root).
 * @param nm: name for lowest in chain
 * @param nm_len: length of name
 * @param labs: labels in name.
 * @param dclass: class of zone.
 * @param parent: NULL for to root, else so it fits under here.
 * @return zone; a chain of zones and their parents up to the parent.
 *  	or NULL on malloc failure
 */
static struct val_neg_zone* neg_zone_chain(
	uint8_t* nm, size_t nm_len, int labs, uint16_t dclass,
	struct val_neg_zone* parent)
{
	int i;
	int tolabs = parent?parent->labs:0;
	struct val_neg_zone* zone, *prev = NULL, *first = NULL;

	/* create the new subtree, i is labelcount of current creation */
	/* this creates a 'first' to z->parent=NULL list of zones */
	for(i=labs; i!=tolabs; i--) {
		/* create new item */
		zone = neg_setup_zone_node(nm, nm_len, i, dclass);
		if(!zone) {
			/* need to delete other allocations in this routine!*/
			struct val_neg_zone* p=first, *np;
			while(p) {
				np = p->parent;
				free(p);
				free(p->name);
				p = np;
			}
			return NULL;
		}
		if(i == labs) {
			first = zone;
		} else {
			prev->parent = zone;
		}
		/* prepare for next name */
		prev = zone;
		dname_remove_label(&nm, &nm_len);
	}
	return first;
}

/**
 * Create a new zone.
 * @param neg: negative cache
 * @param soa: what to look for.
 * @return zone or NULL if out of memory.
 */
static struct val_neg_zone* neg_create_zone(struct val_neg_cache* neg,
	struct ub_packed_rrset_key* soa)
{
	struct val_neg_zone* zone;
	struct val_neg_zone* parent;
	struct val_neg_zone* p, *np;
	uint8_t* nm = soa->rk.dname;
	size_t nm_len = soa->rk.dname_len;
	int labs = dname_count_labels(nm);
	uint16_t dclass = ntohs(soa->rk.rrset_class);

	/* find closest enclosing parent zone that (still) exists */
	parent = neg_closest_zone_parent(neg, nm, nm_len, labs, dclass);
	if(parent && query_dname_compare(parent->name, nm) == 0)
		return parent; /* already exists, weird */
	/* if parent exists, it is in use */
	log_assert(!parent || parent->count > 0);
	zone = neg_zone_chain(nm, nm_len, labs, dclass, parent);
	if(!zone) {
		return NULL;
	}
	zone->in_use = 1;

	/* insert the list of zones into the tree */
	p = zone;
	while(p) {
		np = p->parent;
		/* mem use */
		neg->use += sizeof(struct val_neg_zone) + p->len;
		/* insert in tree */
		(void)rbtree_insert(&neg->tree, &p->node);
		/* last one needs proper parent pointer */
		if(np == NULL)
			p->parent = parent;
		p = np;
	}
	/* increase usage count of all parents */
	for(p=zone; p; p = p->parent) {
		p->count++;
	}
	return zone;
}

/** find zone name of message, returns the SOA record */
static struct ub_packed_rrset_key* reply_find_soa(struct reply_info* rep)
{
	size_t i;
	for(i=rep->an_numrrsets; i< rep->an_numrrsets+rep->ns_numrrsets; i++){
		if(ntohs(rep->rrsets[i]->rk.type) == LDNS_RR_TYPE_SOA)
			return rep->rrsets[i];
	}
	return NULL;
}

/** see if the reply has NSEC records worthy of caching */
static int reply_has_nsec(struct reply_info* rep)
{
	size_t i;
	struct packed_rrset_data* d;
	if(rep->security != sec_status_secure)
		return 0;
	for(i=rep->an_numrrsets; i< rep->an_numrrsets+rep->ns_numrrsets; i++){
		if(ntohs(rep->rrsets[i]->rk.type) == LDNS_RR_TYPE_NSEC) {
			d = (struct packed_rrset_data*)rep->rrsets[i]->
				entry.data;
			if(d->security == sec_status_secure)
				return 1;
		}
	}
	return 0;
}


/**
 * Create single node of data element.
 * @param nm: name (copied)
 * @param nm_len: length of name
 * @param labs: labels in name.
 * @return element with name nm, or NULL malloc failure.
 */
static struct val_neg_data* neg_setup_data_node(
	uint8_t* nm, size_t nm_len, int labs)
{
	struct val_neg_data* el;
	el = (struct val_neg_data*)calloc(1, sizeof(*el));
	if(!el) {
		return NULL;
	}
	el->node.key = el;
	el->name = memdup(nm, nm_len);
	if(!el->name) {
		free(el);
		return NULL;
	}
	el->len = nm_len;
	el->labs = labs;
	return el;
}

/**
 * Create chain of data element and parents
 * @param nm: name
 * @param nm_len: length of name
 * @param labs: labels in name.
 * @param parent: up to where to make, if NULL up to root label.
 * @return lowest element with name nm, or NULL malloc failure.
 */
static struct val_neg_data* neg_data_chain(
	uint8_t* nm, size_t nm_len, int labs, struct val_neg_data* parent)
{
	int i;
	int tolabs = parent?parent->labs:0;
	struct val_neg_data* el, *first = NULL, *prev = NULL;

	/* create the new subtree, i is labelcount of current creation */
	/* this creates a 'first' to z->parent=NULL list of zones */
	for(i=labs; i!=tolabs; i--) {
		/* create new item */
		el = neg_setup_data_node(nm, nm_len, i);
		if(!el) {
			/* need to delete other allocations in this routine!*/
			struct val_neg_data* p = first, *np;
			while(p) {
				np = p->parent;
				free(p);
				free(p->name);
				p = np;
			}
			return NULL;
		}
		if(i == labs) {
			first = el;
		} else {
			prev->parent = el;
		}

		/* prepare for next name */
		prev = el;
		dname_remove_label(&nm, &nm_len);
	}
	return first;
}

/**
 * Remove NSEC records between start and end points.
 * By walking the tree, the tree is sorted canonically.
 * @param neg: negative cache.
 * @param zone: the zone
 * @param el: element to start walking at.
 * @param nsec: the nsec record with the end point
 */
static void wipeout(struct val_neg_cache* neg, struct val_neg_zone* zone, 
	struct val_neg_data* el, struct ub_packed_rrset_key* nsec)
{
	struct packed_rrset_data* d = (struct packed_rrset_data*)nsec->
		entry.data;
	uint8_t* end;
	size_t end_len;
	int end_labs, m;
	rbnode_t* walk, *next;
	struct val_neg_data* cur;
	/* get endpoint */
	if(!d || d->count == 0 || d->rr_len[0] < 2+1)
		return;
	end = d->rr_data[0]+2;
	end_len = dname_valid(end, d->rr_len[0]-2);
	end_labs = dname_count_labels(end);

	/* sanity check, both owner and end must be below the zone apex */
	if(!dname_subdomain_c(el->name, zone->name) || 
		!dname_subdomain_c(end, zone->name))
		return;

	/* detect end of zone NSEC ; wipe until the end of zone */
	if(query_dname_compare(end, zone->name) == 0) {
		end = NULL;
	}

	walk = rbtree_next(&el->node);
	while(walk && walk != RBTREE_NULL) {
		cur = (struct val_neg_data*)walk;
		/* sanity check: must be larger than start */
		if(dname_canon_lab_cmp(cur->name, cur->labs, 
			el->name, el->labs, &m) <= 0) {
			/* r == 0 skip original record. */
			/* r < 0  too small! */
			walk = rbtree_next(walk);
			continue;
		}
		/* stop at endpoint, also data at empty nonterminals must be
		 * removed (no NSECs there) so everything between 
		 * start and end */
		if(end && dname_canon_lab_cmp(cur->name, cur->labs,
			end, end_labs, &m) >= 0) {
			break;
		}
		/* this element has to be deleted, but we cannot do it
		 * now, because we are walking the tree still ... */
		/* get the next element: */
		next = rbtree_next(walk);
		/* now delete the original element, this may trigger
		 * rbtree rebalances, but really, the next element is
		 * the one we need.
		 * But it may trigger delete of other data and the
		 * entire zone. However, if that happens, this is done
		 * by deleting the *parents* of the element for deletion,
		 * and maybe also the entire zone if it is empty. 
		 * But parents are smaller in canonical compare, thus,
		 * if a larger element exists, then it is not a parent,
		 * it cannot get deleted, the zone cannot get empty.
		 * If the next==NULL, then zone can be empty. */
		if(cur->in_use)
			neg_delete_data(neg, cur);
		walk = next;
	}
}


/**
 * Insert data into the data tree of a zone
 * @param neg: negative cache
 * @param zone: zone to insert into
 * @param nsec: record to insert.
 */
static void neg_insert_data(struct val_neg_cache* neg, 
	struct val_neg_zone* zone, struct ub_packed_rrset_key* nsec)
{
	struct packed_rrset_data* d;
	struct val_neg_data* parent;
	struct val_neg_data* el;
	uint8_t* nm = nsec->rk.dname;
	size_t nm_len = nsec->rk.dname_len;
	int labs = dname_count_labels(nsec->rk.dname);

	d = (struct packed_rrset_data*)nsec->entry.data;
	if(d->security != sec_status_secure)
		return;
	log_nametypeclass(VERB_ALGO, "negcache rr", 
		nsec->rk.dname, LDNS_RR_TYPE_NSEC, ntohs(nsec->rk.rrset_class));

	/* find closest enclosing parent data that (still) exists */
	parent = neg_closest_data_parent(zone, nm, nm_len, labs);
	if(parent && query_dname_compare(parent->name, nm) == 0) {
		/* perfect match already exists */
		log_assert(parent->count > 0);
		el = parent;
	} else { 
		struct val_neg_data* p, *np;

		/* create subtree for perfect match */
		/* if parent exists, it is in use */
		log_assert(!parent || parent->count > 0);

		el = neg_data_chain(nm, nm_len, labs, parent);
		if(!el) {
			log_err("out of memory inserting NSEC negative cache");
			return;
		}
		el->in_use = 0; /* set on below */

		/* insert the list of zones into the tree */
		p = el;
		while(p) {
			np = p->parent;
			/* mem use */
			neg->use += sizeof(struct val_neg_data) + p->len;
			/* insert in tree */
			p->zone = zone;
			(void)rbtree_insert(&zone->tree, &p->node);
			/* last one needs proper parent pointer */
			if(np == NULL)
				p->parent = parent;
			p = np;
		}
	}

	if(!el->in_use) {
		struct val_neg_data* p;

		el->in_use = 1;
		/* increase usage count of all parents */
		for(p=el; p; p = p->parent) {
			p->count++;
		}

		neg_lru_front(neg, el);
	} else {
		/* in use, bring to front, lru */
		neg_lru_touch(neg, el);
	}

	/* wipe out the cache items between NSEC start and end */
	wipeout(neg, zone, el, nsec);
}

void val_neg_addreply(struct val_neg_cache* neg, struct reply_info* rep)
{
	size_t i, need;
	struct ub_packed_rrset_key* soa;
	struct val_neg_zone* zone;
	/* see if secure nsecs inside */
	if(!reply_has_nsec(rep))
		return;
	/* find the zone name in message */
	soa = reply_find_soa(rep);
	if(!soa)
		return;

	log_nametypeclass(VERB_ALGO, "negcache insert for zone",
		soa->rk.dname, LDNS_RR_TYPE_SOA, ntohs(soa->rk.rrset_class));
	
	/* ask for enough space to store all of it */
	need = calc_data_need(rep) + calc_zone_need(soa);
	lock_basic_lock(&neg->lock);
	neg_make_space(neg, need);

	/* find or create the zone entry */
	zone = neg_find_zone(neg, soa);
	if(!zone) {
		if(!(zone = neg_create_zone(neg, soa))) {
			lock_basic_unlock(&neg->lock);
			log_err("out of memory adding negative zone");
			return;
		}
	}

	/* insert the NSECs */
	for(i=rep->an_numrrsets; i< rep->an_numrrsets+rep->ns_numrrsets; i++){
		if(ntohs(rep->rrsets[i]->rk.type) != LDNS_RR_TYPE_NSEC)
			continue;
		if(!dname_subdomain_c(rep->rrsets[i]->rk.dname, 
			zone->name)) continue;
		/* insert NSEC into this zone's tree */
		neg_insert_data(neg, zone, rep->rrsets[i]);
	}
	lock_basic_unlock(&neg->lock);
}

/**
 * Lookup closest data record. For NSEC denial.
 * @param zone: zone to look in
 * @param qname: name to look for.
 * @param len: length of name
 * @param labs: labels in name
 * @param data: data element, exact or smaller or NULL
 * @return true if exact match.
 */
static int neg_closest_data(struct val_neg_zone* zone,
	uint8_t* qname, size_t len, int labs, struct val_neg_data** data)
{
	struct val_neg_data key;
	rbnode_t* r;
	key.node.key = &key;
	key.name = qname;
	key.len = len;
	key.labs = labs;
	if(rbtree_find_less_equal(&zone->tree, &key, &r)) {
		/* exact match */
		*data = (struct val_neg_data*)r;
		return 1;
	} else {
		/* smaller match */
		*data = (struct val_neg_data*)r;
		return 0;
	}
}

int val_neg_dlvlookup(struct val_neg_cache* neg, uint8_t* qname, size_t len,
        uint16_t qclass, struct rrset_cache* rrset_cache, uint32_t now)
{
	/* lookup closest zone */
	struct val_neg_zone* zone;
	struct val_neg_data* data;
	int labs;
	struct ub_packed_rrset_key* nsec;
	struct packed_rrset_data* d;
	uint32_t flags;
	uint8_t* wc;
	struct query_info qinfo;
	if(!neg) return 0;

	log_nametypeclass(VERB_ALGO, "negcache dlvlookup", qname, 
		LDNS_RR_TYPE_DLV, qclass);
	
	labs = dname_count_labels(qname);
	lock_basic_lock(&neg->lock);
	zone = neg_closest_zone_parent(neg, qname, len, labs, qclass);
	while(zone && !zone->in_use)
		zone = zone->parent;
	if(!zone) {
		lock_basic_unlock(&neg->lock);
		return 0;
	}
	log_nametypeclass(VERB_ALGO, "negcache zone", zone->name, 0, 
		zone->dclass);

	/* lookup closest data record */
	(void)neg_closest_data(zone, qname, len, labs, &data);
	while(data && !data->in_use)
		data = data->parent;
	if(!data) {
		lock_basic_unlock(&neg->lock);
		return 0;
	}
	log_nametypeclass(VERB_ALGO, "negcache rr", data->name, 
		LDNS_RR_TYPE_NSEC, zone->dclass);

	/* lookup rrset in rrset cache */
	flags = 0;
	if(query_dname_compare(data->name, zone->name) == 0)
		flags = PACKED_RRSET_NSEC_AT_APEX;
	nsec = rrset_cache_lookup(rrset_cache, data->name, data->len,
		LDNS_RR_TYPE_NSEC, zone->dclass, flags, now, 0);

	/* check if secure and TTL ok */
	if(!nsec) {
		lock_basic_unlock(&neg->lock);
		return 0;
	}
	d = (struct packed_rrset_data*)nsec->entry.data;
	if(!d || now > d->ttl) {
		lock_rw_unlock(&nsec->entry.lock);
		/* delete data record if expired */
		neg_delete_data(neg, data);
		lock_basic_unlock(&neg->lock);
		return 0;
	}
	if(d->security != sec_status_secure) {
		lock_rw_unlock(&nsec->entry.lock);
		neg_delete_data(neg, data);
		lock_basic_unlock(&neg->lock);
		return 0;
	}
	verbose(VERB_ALGO, "negcache got secure rrset");

	/* check NSEC security */
	/* check if NSEC proves no DLV type exists */
	/* check if NSEC proves NXDOMAIN for qname */
	qinfo.qname = qname;
	qinfo.qtype = LDNS_RR_TYPE_DLV;
	qinfo.qclass = qclass;
	if(!nsec_proves_nodata(nsec, &qinfo, &wc) &&
		!val_nsec_proves_name_error(nsec, qname)) {
		/* the NSEC is not a denial for the DLV */
		lock_rw_unlock(&nsec->entry.lock);
		lock_basic_unlock(&neg->lock);
		verbose(VERB_ALGO, "negcache not proven");
		return 0;
	}
	/* so the NSEC was a NODATA proof, or NXDOMAIN proof. */

	/* no need to check for wildcard NSEC; no wildcards in DLV repos */
	/* no need to lookup SOA record for client; no response message */

	lock_rw_unlock(&nsec->entry.lock);
	/* if OK touch the LRU for neg_data element */
	neg_lru_touch(neg, data);
	lock_basic_unlock(&neg->lock);
	verbose(VERB_ALGO, "negcache DLV denial proven");
	return 1;
}
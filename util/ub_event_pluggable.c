/*
 * util/ub_event_pluggable.c - call registered pluggable event functions
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains an implementation for the indirection layer for pluggable
 * events that calls the registered pluggable event loop.  It also defines a
 * default pluggable event loop based on the default libevent (compatibility)
 * functions.
 */
#include "config.h"
#include <sys/time.h>
#include "util/ub_event.h"
#include "libunbound/unbound-event.h"
#include "util/netevent.h"
#include "util/log.h"
#include "util/fptr_wlist.h"

/* We define libevent structures here to hide the libevent stuff. */

#ifdef USE_MINI_EVENT
#  ifdef USE_WINSOCK
#    include "util/winsock_event.h"
#  else
#    include "util/mini_event.h"
#  endif /* USE_WINSOCK */
#else /* USE_MINI_EVENT */
   /* we use libevent */
#  ifdef HAVE_EVENT_H
#    include <event.h>
#  else
#    include "event2/event.h"
#    include "event2/event_struct.h"
#    include "event2/event_compat.h"
#  endif
#endif /* USE_MINI_EVENT */

struct my_event_base {
	struct ub_event_base super;
	struct event_base* base;
};

struct my_event {
	struct ub_event super;
	struct event ev;
};

#define AS_MY_EVENT_BASE(x) \
	(((union {struct ub_event_base* a; struct my_event_base* b;})x).b)
#define AS_MY_EVENT(x) \
	(((union {struct ub_event* a; struct my_event* b;})x).b)

const char* ub_event_get_version()
{
	return "pluggable-event"PACKAGE_VERSION;
}

void
ub_get_event_sys(struct ub_event_base* base, const char** n, const char** s,
	const char** m)
{
	(void)base;
	*n = "pluggable-event";

#ifdef USE_WINSOCK
	*s = "winsock";
	*m = "WSAWaitForMultipleEvents";
#elif defined(USE_MINI_EVENT)
	(void)base;
	*s = "internal";
	*m = "select";
#else
	*s = event_get_version();
#  ifdef HAVE_EVENT_BASE_GET_METHOD
	*n = "pluggable-libevent";
	*m = event_base_get_method(b);
#  elif defined(HAVE_EV_LOOP) || defined(HAVE_EV_DEFAULT_LOOP)
	*n = "pluggable-libev";
	*m = ev_backend2str(ev_backend((struct ev_loop*)b));
#  else
	*m = "not obtainable";
#  endif
#endif
}

void
my_event_add_bits(struct ub_event* ev, short bits)
{
	AS_MY_EVENT(ev)->ev.ev_events |= bits;
}

void
my_event_del_bits(struct ub_event* ev, short bits)
{
	AS_MY_EVENT(ev)->ev.ev_events &= ~bits;
}

void
my_event_set_fd(struct ub_event* ev, int fd)
{
	AS_MY_EVENT(ev)->ev.ev_fd = fd;
}

void
my_event_free(struct ub_event* ev)
{
	free(AS_MY_EVENT(ev));
}

int
my_event_add(struct ub_event* ev, struct timeval* tv)
{
	return event_add(&AS_MY_EVENT(ev)->ev, tv);
}

int
my_event_del(struct ub_event* ev)
{
	return event_del(&AS_MY_EVENT(ev)->ev);
}

int
my_timer_add(struct ub_event* ev, struct ub_event_base* base,
	void (*cb)(int, short, void*), void* arg, struct timeval* tv)
{
	event_set(&AS_MY_EVENT(ev)->ev, -1, UB_EV_TIMEOUT, cb, arg);
	if (event_base_set(AS_MY_EVENT_BASE(base)->base, &AS_MY_EVENT(ev)->ev)
		!= 0)
		return -1;
	return evtimer_add(&AS_MY_EVENT(ev)->ev, tv);
}

int
my_timer_del(struct ub_event* ev)
{
	return evtimer_del(&AS_MY_EVENT(ev)->ev);
}

int
my_signal_add(struct ub_event* ev, struct timeval* tv)
{
	return signal_add(&AS_MY_EVENT(ev)->ev, tv);
}

int
my_signal_del(struct ub_event* ev)
{
	return signal_del(&AS_MY_EVENT(ev)->ev);
}

void
my_winsock_unregister_wsaevent(struct ub_event* ev)
{
#if defined(USE_MINI_EVENT) && defined(USE_WINSOCK)
	winsock_unregister_wsaevent(&AS_MY_EVENT(ev)->ev);
	free(AS_MY_EVENT(ev));
#else
	(void)ev;
#endif
}

void
my_winsock_tcp_wouldblock(struct ub_event* ev, int eventbits)
{
#if defined(USE_MINI_EVENT) && defined(USE_WINSOCK)
	winsock_tcp_wouldblock(&AS_MY_EVENT(ev)->ev, eventbits);
#else
	(void)ev;
	(void)eventbits;
#endif
}

static struct ub_event_vmt default_event_vmt = {
	my_event_add_bits, my_event_del_bits, my_event_set_fd,
	my_event_free, my_event_add, my_event_del,
	my_timer_add, my_timer_del, my_signal_add, my_signal_del,
	my_winsock_unregister_wsaevent, my_winsock_tcp_wouldblock
};

void
my_event_base_free(struct ub_event_base* base)
{
#ifdef USE_MINI_EVENT
	event_base_free(AS_MY_EVENT_BASE(base)->base);
#elif defined(HAVE_EVENT_BASE_FREE) && defined(HAVE_EVENT_BASE_ONCE)
	/* only libevent 1.2+ has it, but in 1.2 it is broken - 
	   assertion fails on signal handling ev that is not deleted
 	   in libevent 1.3c (event_base_once appears) this is fixed. */
	event_base_free(AS_MY_EVENT_BASE(base)->base);
#endif /* HAVE_EVENT_BASE_FREE and HAVE_EVENT_BASE_ONCE */
	free(AS_MY_EVENT_BASE(base));
}

int
my_event_base_dispatch(struct ub_event_base* base)
{
	return event_base_dispatch(AS_MY_EVENT_BASE(base)->base);
}

int
my_event_base_loopexit(struct ub_event_base* base, struct timeval* tv)
{
	return event_base_loopexit(AS_MY_EVENT_BASE(base)->base, tv);
}

struct ub_event*
my_event_new(struct ub_event_base* base, int fd, short bits,
	void (*cb)(int, short, void*), void* arg)
{
	struct my_event *my_ev = (struct my_event*)calloc(1,
		sizeof(struct my_event));

	if (!my_ev)
		return NULL;

	event_set(&my_ev->ev, fd, bits, cb, arg);
	if (event_base_set(AS_MY_EVENT_BASE(base)->base, &my_ev->ev) != 0) {
		free(my_ev);
		return NULL;
	}
	my_ev->super.magic = UB_EVENT_MAGIC;
	my_ev->super.vmt = &default_event_vmt;
	return &my_ev->super;
}

struct ub_event*
my_signal_new(struct ub_event_base* base, int fd,
	void (*cb)(int, short, void*), void* arg)
{
	struct my_event *my_ev = (struct my_event*)calloc(1,
		sizeof(struct my_event));

	if (!my_ev)
		return NULL;

	signal_set(&my_ev->ev, fd, cb, arg);
	if (event_base_set(AS_MY_EVENT_BASE(base)->base, &my_ev->ev) != 0) {
		free(my_ev);
		return NULL;
	}
	my_ev->super.magic = UB_EVENT_MAGIC;
	my_ev->super.vmt = &default_event_vmt;
	return &my_ev->super;
}

struct ub_event*
my_winsock_register_wsaevent(struct ub_event_base* base, void* wsaevent,
	void (*cb)(int, short, void*), void* arg)
{
#if defined(USE_MINI_EVENT) && defined(USE_WINSOCK)
	struct my_event *my_ev = (struct my_event*)calloc(1,
		sizeof(struct my_event));

	if (!my_ev)
		return NULL;

	if (!winsock_register_wsaevent(AS_MY_EVENT_BASE(base)->base,
		&my_ev->ev, wsaevent, cb, arg)) {
		free(my_ev);
		return NULL;

	}
	my_ev->super.magic = UB_EVENT_MAGIC;
	my_ev->super.vmt = &default_event_vmt;
	return &my_ev->super;
#else
	(void)base;
	(void)wsaevent;
	(void)cb;
	(void)arg;
	return NULL;
#endif
}

static struct ub_event_base_vmt default_event_base_vmt = {
	my_event_base_free, my_event_base_dispatch,
	my_event_base_loopexit, my_event_new, my_signal_new,
	my_winsock_register_wsaevent
};

struct ub_event_base*
ub_default_event_base(int sigs, time_t* time_secs, struct timeval* time_tv)
{
	struct my_event_base* my_base = (struct my_event_base*)calloc(1,
		sizeof(struct my_event_base));

	if (!my_base)
		return NULL;

#ifdef USE_MINI_EVENT
	(void)sigs;
	/* use mini event time-sharing feature */
	my_base->base = event_init(time_secs, time_tv);
#else
	(void)time_secs;
	(void)time_tv;
#  if defined(HAVE_EV_LOOP) || defined(HAVE_EV_DEFAULT_LOOP)
	/* libev */
	if(sigs)
		my_base->base = ev_default_loop(EVFLAG_AUTO);
	else
		my_base->base = ev_loop_new(EVFLAG_AUTO);
#  else
	(void)sigs;
#    ifdef HAVE_EVENT_BASE_NEW
	my_base->base = event_base_new();
#    else
	my_base->base = event_init();
#    endif
#  endif
#endif
	if (!my_base->base) {
		free(my_base);
		return NULL;
	}
	my_base->super.magic = UB_EVENT_MAGIC;
	my_base->super.vmt = &default_event_base_vmt;
	return &my_base->super;
}

struct ub_event_base*
ub_libevent_event_base(struct event_base* base)
{
#ifdef USE_MINI_EVENT
	return NULL;
#else
	struct my_event_base* my_base = (struct my_event_base*)calloc(1,
		sizeof(struct my_event_base));

	if (!my_base)
		return NULL;
	my_base->super.magic = UB_EVENT_MAGIC;
	my_base->super.vmt = &default_event_base_vmt;
	return &my_base->super;
#endif
}

struct event_base*
ub_libevent_get_event_base(struct ub_event_base* base)
{
#ifdef USE_MINI_EVENT
	return NULL;
#else
	return AS_MY_EVENT_BASE(base)->base;
#endif
}

void
ub_event_base_free(struct ub_event_base* base)
{
	if (base && base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->free == my_event_base_free);
		(*base->vmt->free)(base);
	}
}

int
ub_event_base_dispatch(struct ub_event_base* base)
{
	if (base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->dispatch == my_event_base_dispatch);
		return (*base->vmt->dispatch)(base);
	}
	return -1;
}

int
ub_event_base_loopexit(struct ub_event_base* base)
{
	if (base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->loopexit == my_event_base_loopexit);
		return (*base->vmt->loopexit)(base, NULL);
	}
	return -1;
}

struct ub_event*
ub_event_new(struct ub_event_base* base, int fd, short bits,
	void (*cb)(int, short, void*), void* arg)
{
	if (base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->new_event == my_event_new);
		return (*base->vmt->new_event)(base, fd, bits, cb, arg);
	}
	return NULL;
}

struct ub_event*
ub_signal_new(struct ub_event_base* base, int fd,
	void (*cb)(int, short, void*), void* arg)
{
	if (base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->new_signal == my_signal_new);
		return (*base->vmt->new_signal)(base, fd, cb, arg);
	}
	return NULL;
}

struct ub_event*
ub_winsock_register_wsaevent(struct ub_event_base* base, void* wsaevent,
	void (*cb)(int, short, void*), void* arg)
{
	if (base->magic == UB_EVENT_MAGIC) {
		fptr_ok(base->vmt != &default_event_base_vmt ||
			base->vmt->winsock_register_wsaevent ==
			my_winsock_register_wsaevent);
		return (*base->vmt->winsock_register_wsaevent)(base, wsaevent, cb, arg);
	}
	return NULL;
}

void
ub_event_add_bits(struct ub_event* ev, short bits)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->add_bits == my_event_add_bits);
		(*ev->vmt->add_bits)(ev, bits);
	}
}

void
ub_event_del_bits(struct ub_event* ev, short bits)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->del_bits == my_event_del_bits);
		(*ev->vmt->del_bits)(ev, bits);
	}
}

void
ub_event_set_fd(struct ub_event* ev, int fd)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->set_fd == my_event_set_fd);
		(*ev->vmt->set_fd)(ev, fd);
	}
}

void
ub_event_free(struct ub_event* ev)
{
	if (ev && ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->free == my_event_free);
		ev->vmt->free(ev);
	}
}

int
ub_event_add(struct ub_event* ev, struct timeval* tv)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->add == my_event_add);
		return (*ev->vmt->add)(ev, tv);
	}
       return -1;
}

int
ub_event_del(struct ub_event* ev)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->del == my_event_del);
		return (*ev->vmt->del)(ev);
	}
	return -1;
}

int
ub_timer_add(struct ub_event* ev, struct ub_event_base* base,
	void (*cb)(int, short, void*), void* arg, struct timeval* tv)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->add_timer == my_timer_add);
		return (*ev->vmt->add_timer)(ev, base, cb, arg, tv);
	}
	return -1;
}

int
ub_timer_del(struct ub_event* ev)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->del_timer == my_timer_del);
		return (*ev->vmt->del_timer)(ev);
	}
	return -1;
}

int
ub_signal_add(struct ub_event* ev, struct timeval* tv)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->add_signal == my_signal_add);
		return (*ev->vmt->add_signal)(ev, tv);
	}
	return -1;
}

int
ub_signal_del(struct ub_event* ev)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->del_signal == my_signal_del);
		return (*ev->vmt->del_signal)(ev);
	}
	return -1;
}

void
ub_winsock_unregister_wsaevent(struct ub_event* ev)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->winsock_unregister_wsaevent ==
			my_winsock_unregister_wsaevent);
		(*ev->vmt->winsock_unregister_wsaevent)(ev);
	}
}

void
ub_winsock_tcp_wouldblock(struct ub_event* ev, int eventbits)
{
	if (ev->magic == UB_EVENT_MAGIC) {
		fptr_ok(ev->vmt != &default_event_vmt ||
			ev->vmt->winsock_tcp_wouldblock ==
			my_winsock_tcp_wouldblock);
		(*ev->vmt->winsock_tcp_wouldblock)(ev, eventbits);
	}
}

void ub_comm_base_now(struct comm_base* cb)
{
	time_t *tt;
	struct timeval *tv;

#ifdef USE_MINI_EVENT
/** minievent updates the time when it blocks. */
	if (comm_base_internal(cb)->magic == UB_EVENT_MAGIC &&
	    comm_base_internal(cb)->vmt == &default_event_base_vmt)
		return; /* Actually using mini event, so do not set time */
#endif /* USE_MINI_EVENT */

/** fillup the time values in the event base */
	comm_base_timept(cb, &tt, &tv);
	if(gettimeofday(tv, NULL) < 0) {
		log_err("gettimeofday: %s", strerror(errno));
	}
	*tt = tv->tv_sec;
}


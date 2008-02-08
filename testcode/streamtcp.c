/*
 * testcode/streamtcp.c - debug program perform multiple DNS queries on tcp.
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
 * This program performs multiple DNS queries on a TCP stream.
 */

#include "config.h"
#include <signal.h>
#include "util/locks.h"
#include "util/log.h"
#include "util/net_help.h"
#include "util/data/msgencode.h"
#include "util/data/msgreply.h"
#include "util/data/dname.h"

/** usage information for streamtcp */
void usage(char* argv[])
{
	printf("usage: %s [options] name type class ...\n", argv[0]);
	printf("	sends the name-type-class queries over TCP.\n");
	printf("-f server	what ipaddr@portnr to send the queries to\n");
	printf("-h 		this help text\n");
	exit(1);
}

/** open TCP socket to svr */
static int
open_svr(char* svr)
{
	struct sockaddr_storage addr;
	socklen_t addrlen;
	int fd = -1;
	/* svr can be ip@port */
	if(!extstrtoaddr(svr, &addr, &addrlen)) {
		printf("fatal: bad server specs '%s'\n", svr);
		exit(1);
	}
	fd = socket(addr_is_ip6(&addr, addrlen)?PF_INET6:PF_INET,
		SOCK_STREAM, 0);
	if(fd == -1) {
		perror("socket() error");
		exit(1);
	}
	if(connect(fd, (struct sockaddr*)&addr, addrlen) < 0) {
		perror("connect() error");
		exit(1);
	}
	return fd;
}

/** write a query over the TCP fd */
static void
write_q(int fd, ldns_buffer* buf, int id, 
	char* strname, char* strtype, char* strclass)
{
	struct query_info qinfo;
	ldns_rdf* rdf;
	int labs;
	uint16_t len;
	/* qname */
	rdf = ldns_dname_new_frm_str(strname);
	if(!rdf) {
		printf("cannot parse query name: '%s'\n", strname);
		exit(1);
	}
	qinfo.qname = memdup(ldns_rdf_data(rdf), ldns_rdf_size(rdf));
	labs = dname_count_size_labels(qinfo.qname, &qinfo.qname_len);
	ldns_rdf_deep_free(rdf);
	if(!qinfo.qname) fatal_exit("out of memory");

	/* qtype and qclass */
	qinfo.qtype = ldns_get_rr_type_by_name(strtype);
	qinfo.qclass = ldns_get_rr_class_by_name(strclass);

	/* make query */
	qinfo_query_encode(buf, &qinfo);
	ldns_buffer_write_u16_at(buf, 0, (uint16_t)id);
	ldns_buffer_write_u16_at(buf, 2, BIT_RD);

	/* send it */
	len = (uint16_t)ldns_buffer_limit(buf);
	len = htons(len);
	if(write(fd, &len, sizeof(len)) < (ssize_t)sizeof(len)) {
		perror("write() len failed");
		exit(1);
	}
	if(write(fd, ldns_buffer_begin(buf), ldns_buffer_limit(buf)) < 
		(ssize_t)ldns_buffer_limit(buf)) {
		perror("write() data failed");
		exit(1);
	}

	free(qinfo.qname);
}

/** receive DNS datagram over TCP and print it */
static void
recv_one(int fd, ldns_buffer* buf)
{
	uint16_t len;
	ldns_pkt* pkt;
	ldns_status status;
	if(read(fd, &len, sizeof(len)) < (ssize_t)sizeof(len)) {
		perror("read() len failed");
		exit(1);
	}
	len = ntohs(len);
	ldns_buffer_clear(buf);
	ldns_buffer_set_limit(buf, len);
	if(read(fd, ldns_buffer_begin(buf), len) < (ssize_t)len) {
		perror("read() data failed");
		exit(1);
	}
	printf("\nnext received packet\n");
	log_buf(0, "data", buf);

	status = ldns_wire2pkt(&pkt, ldns_buffer_begin(buf), len);
	if(status != LDNS_STATUS_OK) {
		printf("could not parse incoming packet: %s\n",
			ldns_get_errorstr_by_id(status));
		log_buf(0, "data was", buf);
		exit(1);
	}
	ldns_pkt_print(stdout, pkt);
	ldns_pkt_free(pkt);
}

/** send the TCP queries and print answers */
static void
send_em(char* svr, int num, char** qs)
{
	ldns_buffer* buf = ldns_buffer_new(65553);
	int fd = open_svr(svr);
	int i;
	if(!buf) fatal_exit("out of memory");
	for(i=0; i<num; i+=3) {
		printf("\nNext query is %s %s %s\n", qs[i], qs[i+1], qs[i+2]);
		write_q(fd, buf, i, qs[i], qs[i+1], qs[i+2]);
		/* print at least one result */
		recv_one(fd, buf);
	}

	close(fd);
	ldns_buffer_free(buf);
	printf("orderly exit\n");
}

/** SIGPIPE handler */
static RETSIGTYPE sigh(int sig)
{
	if(sig == SIGPIPE) {
		printf("got SIGPIPE, remote connection gone\n");
		exit(1);
	}
	printf("Got unhandled signal %d\n", sig);
	exit(1);
}

/** getopt global, in case header files fail to declare it. */
extern int optind;
/** getopt global, in case header files fail to declare it. */
extern char* optarg;

/** main program for streamtcp */
int main(int argc, char** argv) 
{
	int c;
	char* svr = "127.0.0.1";

	/* lock debug start (if any) */
	log_init(0, 0, 0);
	checklock_start();

	if(signal(SIGPIPE, &sigh) == SIG_ERR) {
		perror("could not install signal handler");
		return 1;
	}

	/* command line options */
	if(argc == 1) {
		usage(argv);
	}
	while( (c=getopt(argc, argv, "f:h")) != -1) {
		switch(c) {
			case 'f':
				svr = optarg;
				break;
			case 'h':
			case '?':
			default:
				usage(argv);
		}
	}
	argc -= optind;
	argv += optind;

	if(argc % 3 != 0) {
		printf("queries must be multiples of name,type,class\n");
		return 1;
	}
	send_em(svr, argc, argv);
	checklock_stop();
	return 0;
}
/*
 * util/winsock_event.h - unbound event handling for winsock on windows
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
 * This file contains interface functions with the WinSock2 API on Windows.
 * It uses the winsock WSAWaitForMultipleEvents interface on a number of
 * sockets.
 *
 * Note that windows can only wait for max 64 events at one time.
 * 
 * Also, file descriptors cannot be waited for.
 *
 * Named pipes are not easily available (and are not usable in select() ).
 * For interprocess communication, it is possible to wait for a hEvent to
 * be signaled by another thread.
 *
 * When a socket becomes readable, then it will not be flagged as 
 * readable again until you have gotten WOULDBLOCK from a recv routine.
 * That means the event handler must store the readability (edge notify)
 * and process the incoming data until it blocks. 
 * The function performing recv then has to inform the event handler that
 * the socket has blocked, and the event handler can mark it as such.
 * Thus, this file transforms the edge notify from windows to a level notify
 * that is compatible with UNIX.
 * However, the WSAEventSelect page says that it does do level notify, as long
 * as you call a recv/write/accept at least once when it is signalled.
 *
 * To stay 'fair', instead of emptying a socket completely, the event handler
 * can test the other (marked as blocking) sockets for new events.
 *
 * Additionally, TCP accept sockets get special event support.
 *
 * Socket numbers are not starting small, they can be any number (say 33060).
 * Therefore, bitmaps are not used, but arrays.
 */

#ifndef UTIL_WINSOCK_EVENT_H
#define UTIL_WINSOCK_EVENT_H

#ifdef USE_WINSOCK

#ifndef HAVE_EVENT_BASE_FREE
#define HAVE_EVENT_BASE_FREE
#endif

/** event timeout */
#define EV_TIMEOUT      0x01
/** event fd readable */
#define EV_READ         0x02
/** event fd writable */
#define EV_WRITE        0x04
/** event signal */
#define EV_SIGNAL       0x08
/** event must persist */
#define EV_PERSIST      0x10

/* needs our redblack tree */
#include "rbtree.h"

/** max number of signals to support */
#define MAX_SIG 32

/** The number of items that the winsock event handler can service.
 * Windows cannot handle more anyway */
#define WSK_MAX_ITEMS 64

/**
 * event base for winsock event handler
 */
struct event_base
{
	/** sorted by timeout (absolute), ptr */
	rbtree_t* times;
	/** array (first part in use) of handles to work on */
	struct event** items;
	/** number of items in use in array */
	int max;
	/** capacity of array, size of array in items */
	int cap;
	/** array of 0 - maxsig of ptr to event for it */
        struct event** signals;
	/** if we need to exit */
	int need_to_exit;
	/** where to store time in seconds */
	uint32_t* time_secs;
	/** where to store time in microseconds */
	struct timeval* time_tv;
};

/**
 * Event structure. Has some of the event elements.
 */
struct event {
        /** node in timeout rbtree */
        rbnode_t node;
        /** is event already added */
        int added;

        /** event base it belongs to */
        struct event_base *ev_base;
        /** fd to poll or -1 for timeouts. signal number for sigs. */
        int ev_fd;
        /** what events this event is interested in, see EV_.. above. */
        short ev_events;
        /** timeout value */
        struct timeval ev_timeout;

        /** callback to call: fd, eventbits, userarg */
        void (*ev_callback)(int, short, void *arg);
        /** callback user arg */
        void *ev_arg;

	/* ----- nonpublic part, for winsock_event only ----- */
	/** index of this event in the items array (if added) */
	int idx;
	/** the event handle to wait for new events to become ready */
	WSAEVENT hEvent;
};

/** create event base */
void *event_init(uint32_t* time_secs, struct timeval* time_tv);
/** get version */
const char *event_get_version(void);
/** get polling method (select,epoll) */
const char *event_get_method(void);
/** run select in a loop */
int event_base_dispatch(struct event_base *);
/** exit that loop */
int event_base_loopexit(struct event_base *, struct timeval *);
/** free event base. Free events yourself */
void event_base_free(struct event_base *);
/** set content of event */
void event_set(struct event *, int, short, void (*)(int, short, void *), void *);

/** add event to a base. You *must* call this for every event. */
int event_base_set(struct event_base *, struct event *);
/** add event to make it active. You may not change it with event_set anymore */
int event_add(struct event *, struct timeval *);
/** remove event. You may change it again */
int event_del(struct event *);

#define evtimer_add(ev, tv)             event_add(ev, tv)
#define evtimer_del(ev)                 event_del(ev)

/* uses different implementation. Cannot mix fd/timeouts and signals inside
 * the same struct event. create several event structs for that.  */
/** install signal handler */
int signal_add(struct event *, struct timeval *);
/** set signal event contents */
#define signal_set(ev, x, cb, arg)      \
        event_set(ev, x, EV_SIGNAL|EV_PERSIST, cb, arg)
/** remove signal handler */
int signal_del(struct event *);

/** compare events in tree, based on timevalue, ptr for uniqueness */
int mini_ev_cmp(const void* a, const void* b);

#endif /* USE_WINSOCK */
#endif /* UTIL_WINSOCK_EVENT_H */
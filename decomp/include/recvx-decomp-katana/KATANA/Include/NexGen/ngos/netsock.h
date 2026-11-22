/*****************************************************************************
 * $Id: netsock.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Transport Layer/Socket Definitions
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 18/07/2000 - Regis Feneon
 *  moved from net_sock.h
 *****************************************************************************/

#ifndef __NG_NETSOCK_H_INCLUDED__
#define __NG_NETSOCK_H_INCLUDED__

#include <ngos/queue.h>

NG_BEGIN_DECLS

/* socket descriptor structure */
typedef struct NGsock_S {
    NGnode      so_node;
    int         so_handle;      /* type of handle (NG_FD_SOCK) */
    int         so_options;
#define	NG_SO_DEBUG	            0x0001  /* turn on debugging info recording */
#define	NG_SO_ACCEPTCONN        0x0002	/* socket has had listen() */
#define	NG_SO_REUSEADDR	        0x0004	/* allow local address reuse */
#define	NG_SO_KEEPALIVE	        0x0008	/* keep connections alive */
#define	NG_SO_DONTROUTE	        0x0010	/* just use interface addresses */
#define	NG_SO_BROADCAST	        0x0020	/* permit sending of broadcast msgs */
#define	NG_SO_USELOOPBACK	    0x0040	/* bypass hardware when possible */
#define	NG_SO_LINGER	        0x0080	/* linger on close if data present */
#define	NG_SO_OOBINLINE	        0x0100	/* leave received OOB data in line */
#define NG_SO_HDRINCL           0x1000  /* include IP header (raw sockets) */
    int         so_state;
#define NG_SS_ASYNC             0x0001  /* send asynchronous signals */
#define NG_SS_NBIO              0x0002  /* set non-blocking io */
#define NG_SS_CANTRCVMORE       0x0004  /* recv connection closed */
#define NG_SS_CANTSENDMORE      0x0008  /* send connection closed */
#define NG_SS_ISCONFIRMING      0x0010  /* confirmation in progress */
#define NG_SS_ISCONNECTED       0x0020  /* connection completed */
#define NG_SS_ISCONNECTING      0x0040  /* connection in progress */
#define NG_SS_ISDISCONNECTING   0x0080  /* disconnection in progress */
#define NG_SS_RCVATMARK         0x0100  /* pointer is on out-of-band data */
#define NG_SS_NOFDREF           0x0200  /* socket is detached from application */
    int         so_nsleep;          /* nb of tasks waiting on socket */
    void       *so_sem;             /* semaphore for waiting tasks */
    int         so_linger;          /* time to linger while closing */
/* connection queueing */
    struct NGsock_S *so_head;       /* back pointer to accept socket */
    struct NGsock_S *so_acceptq0;   /* queue of partial connections */
    int         so_aq0len;          /* number of partial connections */
    struct NGsock_S *so_acceptq;    /* queue of incoming connections */
    int         so_aqlen;           /* number of incoming connections */
    int         so_aqmax;           /* maximum number of queued connections */
    int         so_timeo;           /* connection timeout */
    int         so_error;           /* asynchronous error */
    int         so_oobmark;         /* chars to oob mark */
/* receive/send buffers */
    int         so_rcv_cc;          /* number of bytes in receive queue */
    int         so_rcv_hiwat;       /* maximum size of receive queue */
    int         so_rcv_lowat;       /* low water mark */
    NGuint      so_rcv_timeo;       /* timeout on receive calls */
    NGqueue     so_rcv_q;           /* receive queue */
    int         so_snd_cc;          /* number of bytes in send queue */
    int         so_snd_hiwat;       /* maximum size of send queue */
    int         so_snd_lowat;       /* low water mark */
    NGuint      so_snd_timeo;       /* timeout on send calls */
/* transport protocol data */
    void       *so_proto;           /* pointer to protocol structure */
    int         so_ipproto;         /* IP protocol number (raw sockets) */
    NGuint      so_faddr;           /* foreign address */
    NGuint      so_laddr;           /* local address */
    NGushort    so_fport;           /* foreign port */
    NGushort    so_lport;           /* local port */
    NGubyte     so_pad1;            /* padding */
    NGubyte     so_ttl;             /* ip datagram ttl */
    NGubyte     so_tos;             /* ip datagram tos */
    NGubyte     so_mcast_ttl;       /* ttl for multicasts */
    void       *so_mcast_ifnetp;    /* interface for multicasts */
    void       *so_data;            /* protocol specific data */
} NGsock;

NG_END_DECLS

#endif


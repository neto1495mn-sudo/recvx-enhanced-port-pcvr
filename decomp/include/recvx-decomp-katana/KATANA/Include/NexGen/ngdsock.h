/*****************************************************************************
 * $Id: ngdsock.h,v 1.4 2000/09/14 14:57:37 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Socket Interface Definitions
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
 * 07/10/98 - Regis Feneon
 * 12/12/98 -
 *  Change socket recv/send buffers organization
 * 06/01/99 -
 *  changes in proprietary socket interface
 *  new NGsockctrl structure, ngSockOpen() function changed
 * 13/01/99 -
 *  added NGtimeval and NGlinger for socket options
 *  NG_SOCKBUF_XXX macros
 * 19/01/99 -
 *  ngSoSleepOn() added
 * 11/04/99 -
 *  SOL_IP, SOL_TCP, SOL_UDP added
 * 28/05/99 -
 *  added so_ipproto in NGsock structure
 *  added NG_SO_HDRINCL in so_options
 * 12/08/99 -
 *  ngSockFdGetPtr(), ngSockGetState(), ngSockReadByte() added
 * 10/09/99 -
 *  Flags NG_IOSTATE_ISDISCONNECTED & NG_IOSTATE_HASERROR added
 *  for ngSockGetState()
 * 11/10/99 -
 *  NG_IOSTATE_XXX & NGtimeval moved to ng_stdio.h
 *  now include ng_stdio.h
 *  ngSockGetState() prototype changed
 *  ngSockBufFree() prototype changed
 *  ngSockGetNRead() prototype added
 *  ngSoOption() prototype changed
 * 09/01/2000 -
 *  added prototypes for ngSockPrintf() and ngSockVPrintf()
 * 09/02/2000 _
 *  added NG_IF_MULTICAST options
 *  added so_mcast fields in NGsock structure
 * 11/02/2000 -
 *  new proto for ngSockVPrintf() and ngSockPrintf()
 * 24/02/2000 -
 *  added NG_IF_MULTICAST_IFP option
 * 08/06/2000 -
 *  changed ngSoFindEphemeral to return (void)
 * 19/07/2000 -
 *  NG_PRU_XXX and NG_PRC_XXX moved to netprot.h
 * 04/09/2000 -
 *  added prototypes for ngSockInChar(),ngSockOutChar(),ngSoRecv(),ngSoSend()
 * 14/09/2000 -
 *  added NG_SOCKBUF_SETIF() macro
 *****************************************************************************/

#ifndef __NG_NGDSOCK_H_INCLUDED__
#define __NG_NGDSOCK_H_INCLUDED__

#include <ngip.h>

NG_BEGIN_DECLS

/*****************************************************************************
 * Public
 *****************************************************************************/

/* address family */
#define	NG_AF_UNSPEC	    0       /* unspecified */
#define	NG_AF_INET		    2       /* internetwork: UDP, TCP, etc. */

/* type of socket */
#define NG_SOCK_TCP         1
#define NG_SOCK_UDP         2
#define NG_SOCK_RAW         3

/* level of option */
#define	NG_IOCTL_SOCKET	    0xffffU	    /* options for socket level */
#define NG_IOCTL_IP         IPPROTO_IP      /* IP level (raw sockets)  */
#define NG_IOCTL_TCP        IPPROTO_TCP     /* TCP level */
#define NG_IOCTL_UDP        IPPROTO_UDP     /* UDP level */
/* set/get operation */
#define NG_SO_SET           0x8000U /* set option */
#define NG_SO_GET           0x0000U /* get option */
/* socket level options */
#define NG_SO_SNDBUF	    0x1001  /* send buffer size */
#define NG_SO_RCVBUF	    0x1002	/* receive buffer size */
#define NG_SO_SNDLOWAT	    0x1003	/* send low-water mark */
#define NG_SO_RCVLOWAT	    0x1004	/* receive low-water mark */
#define NG_SO_SNDTIMEO	    0x1005	/* send timeout */
#define NG_SO_RCVTIMEO	    0x1006	/* receive timeout */
#define	NG_SO_ERROR	        0x1007	/* get error status and clear (ro) */
#define	NG_SO_TYPE		    0x1008	/* get socket type (ro) */
/* NGDSock specific options */
#define NG_SO_NBIO          0x1f01  /* set nonblocking mode (NG_SS_NBIO flag) */
#define NG_SO_NREAD         0x1f02  /* nb of bytes in rcv buffer (ro) */
#define NG_SO_ATMARK        0x1f03  /* returns SS_RCVATMARK flag (ro) */
#define NG_SO_SOCKNAME      0x1f04  /* get socket name (ro) */
#define NG_SO_PEERNAME      0x1f05  /* get peer name (ro) */
/* IP level options */
#define	NG_IP_HDRINCL	    0x0002  /* header is included with data (raw) */
#define	NG_IP_TOS		    0x0003  /* IP type of service and precedence */
#define	NG_IP_TTL		    0x0004  /* IP time to live */
#define NG_IP_MULTICAST_IF  0x0005  /* Interface for outgoing multicasts */
#define NG_IP_MULTICAST_TTL 0x0006  /* time to live for outgoing multicasts */
#define NG_IP_MULTICAST_IFP 0x0007  /* interface for outgoing multicasts */
/* flags for recv/send operations */
#define NG_SOCKIO_DONTROUTE 0x0010
#define NG_SOCKIO_EOR       0x0020
#define NG_SOCKIO_TRUNC     0x0040
#define NG_SOCKIO_CTRUNC    0x0080

/* socket address structure */
typedef struct {
    NGubyte     sin_len;        /* address length (not used) */
    NGubyte     sin_family;     /* address family (not used) */
    NGushort    sin_port;       /* port number (net byte order) */
    NGuint      sin_addr;       /* ip address (net byte order) */
} NGsockaddr;

/* used by NG_SO_LINGER */
typedef struct {
    int l_onoff;        /* option on/off */
    int l_linger;       /* linget time in seconds */
} NGlinger;


/* socket open control structure */
typedef struct {
    int         sc_family;          /* protocol family: PF_INET or AF_INET */
    int         sc_type;            /* type of socket (SOCK_DGRAM,SOCK_STREAM) */
    int         sc_protocol;        /* protocol for raw sockets */
    int         sc_flags;           /* open flags: */
#define NG_SOCKF_NONBLOCK   0x0001      /* non-blocking operations */
#define NG_SOCKF_PASSIVE    0x0002      /* force passive open */
    NGsockaddr  sc_laddr;           /* local address and port */
    NGsockaddr  sc_faddr;           /* foreign address and port */
    int         sc_options;         /* socket options */
    int         sc_timeo;           /* connection time-out (seconds) */
    int         sc_backlog;         /* maximum number of pending connections */
    int         sc_linger;          /* linger time (seconds) */
    int         sc_rcv_buf;         /* size of receive buffer */
    int         sc_snd_buf;         /* size of send buffer */
    int         sc_rcv_lowat;       /* low water mark */
    int         sc_snd_lowat;
    u_long      sc_rcv_timeo_ms;    /* receive timeout (milliseconds) */
    u_long      sc_snd_timeo_ms;    /* send timeout (milliseconds) */
} NGsockctrl;

int ngSockAccept( NGsock *so, NGsockaddr *addr, NGsock **newso);
int ngSockClose( NGsock *so);
int ngSockFdGetPtr( int fd, NGsock **so);
int ngSockGetOption( NGsock *so, int level, int option, void *optval, int *optlen);
int ngSockGetState( NGsock *so);
int ngSockGetNRead( NGsock *so);
int ngSockOpen( NGsockctrl *ctrlp, NGsock **newso);
int ngSockRecv( NGsock *so, void *datap, int datalen, int *flags, NGsockaddr *addr);
int ngSockRecvv( NGsock *so, NGiovec *iov, int iovcnt, int *flags, NGsockaddr *addr);
int ngSockSend( NGsock *so, void *datap, int datalen, int flags, NGsockaddr *addr);
int ngSockSendv( NGsock *so, NGiovec *iov, int iovcnt, int flags, NGsockaddr *addr);
int ngSockSetOption( NGsock *so, int level, int option, const void *optval, int optlen);
int ngSockShutdown( NGsock *so);
int ngSockReadByte( NGsock *so, int flags);
int ngSockPrintf( NGsock *so, char *fmt,...);
int ngSockVPrintf( NGsock *so, char *fmt, NGva_list args);
/* zero-copy interface */
int ngSockBufAlloc( NGsock *so, NGbuf **bufp);
int ngSockBufFree( NGsock *so, NGbuf *bufp);
int ngSockBufRecv( NGsock *so, int *flags, NGsockaddr *addr, NGbuf **bufp);
int ngSockBufSend( NGsock *so, NGbuf *bufp, int flags, NGsockaddr *addr);

/* zero-copy buffer macros */
#define NG_SOCKBUF_PTR( bufp)           ((void *)((bufp)->buf_datap))
#define NG_SOCKBUF_LEN( bufp)           ((bufp)->buf_datalen)
#define NG_SOCKBUF_SETLEN( bufp, len)   (bufp)->buf_datalen = (len)
#define NG_SOCKBUF_SETIF( bufp, netp) { \
    (bufp)->buf_flags |= NG_BUFF_SETIF; \
    (bufp)->buf_ifnetp = (netp); \
}

/* max iovec structures for ngSockSendv() and ngSockRecvv() */
#define NG_SOCK_IOVMAX          8

/* standard input/output redirections */
int  ngSockInChar( void *data);
void ngSockOutChar( int c, void *data);

/*
 * BSD compatible names
 */

/* address family */
#define AF_UNSPEC           NG_AF_UNSPEC
#define AF_INET             NG_AF_INET
/* protocol family (== address family) */
#define	PF_UNSPEC	        NG_AF_UNSPEC
#define	PF_INET		        NG_AF_INET
/* types of socket */
#define SOCK_STREAM         NG_SOCK_TCP
#define SOCK_DGRAM          NG_SOCK_UDP
#define SOCK_RAW            NG_SOCK_RAW
/* level of option */
#define	SOL_SOCKET          NG_IOCTL_SOCKET
#define SOL_IP              NG_IOCTL_IP
#define SOL_TCP             NG_IOCTL_TCP
#define SOL_UDP             NG_IOCTL_UDP
/* socket level options */
#define	SO_DEBUG            NG_SO_DEBUG
#define	SO_ACCEPTCONN       NG_SO_ACCEPTCONN
#define	SO_REUSEADDR        NG_SO_REUSEADDR
#define	SO_KEEPALIVE        NG_SO_KEEPALIVE
#define	SO_DONTROUTE        NG_SO_DONTROUTE
#define	SO_BROADCAST        NG_SO_BROADCAST
#define	SO_USELOOPBACK      NG_SO_USELOOPBACK
#define	SO_LINGER           NG_SO_LINGER
#define	SO_OOBINLINE        NG_SO_OOBINLINE
#define SO_SNDBUF           NG_SO_SNDBUF
#define SO_RCVBUF           NG_SO_RCVBUF
#define SO_SNDLOWAT         NG_SO_SNDLOWAT
#define SO_RCVLOWAT         NG_SO_RCVLOWAT
#define SO_SNDTIMEO         NG_SO_SNDTIMEO
#define SO_RCVTIMEO         NG_SO_RCVTIMEO
#define	SO_ERROR            NG_SO_ERROR
#define	SO_TYPE             NG_SO_TYPE
/* ip level options */
#define IP_HDRINC           NG_IP_HDRINC
#define IP_TOS              NG_IP_TOS
#define IP_TTL              NG_IP_TTL
#define IP_MULTICAST_IF     NG_IP_MULTICAST_IF
#define IP_MULTICAST_TTL    NG_IP_MULTICAST_TTL
#define IP_MULTICAST_IFP    NG_IP_MULTICAST_IFP
/* recv/send flags */
#define MSG_DONTWAIT        NG_IO_DONTWAIT
#define MSG_WAITALL         NG_IO_WAITALL
#define MSG_OOB             NG_IO_OOB
#define MSG_PEEK            NG_IO_PEEK
#define MSG_DONTROUTE       NG_SOCKIO_DONTROUTE
#define MSG_EOR             NG_SOCKIO_EOR
#define MSG_TRUNC           NG_SOCKIO_TRUNC
#define MSG_CTRUNC          NG_SOCKIO_CTRUNC


/*****************************************************************************
 * Private
 *****************************************************************************/

/* default socket buffers 'lowat' */
#define NG_SOCK_RECV_LOWAT      1
#define NG_SOCK_SEND_LOWAT      1024

/* internal prototypes */
NGsock *ngSoAlloc( NGsock *head);
void    ngSoFree( NGsock *so);
int     ngSoBind( NGsock *so, NGsockaddr *addr, NGsock *head, NGushort *eport);
int     ngSoConnect( NGsock *so, NGsockaddr *addr, NGsock *head, NGushort *eport);
int     ngSoOption( NGsock *so, u_int level, u_int option, void *optval, int *optlen);
NGsock *ngSoLookup( NGsock *head, NGuint laddr, NGushort lport, NGuint faddr, NGuint fport);
void    ngSoIsConnected( NGsock *so);
void    ngSoIsDisconnected( NGsock *so);
int     ngSoSleepOn( NGsock *so, u_long timeo);
#define ngSoSleepOnRecv( so, timeo)     ngSoSleepOn( so, timeo)
#define ngSoSleepOnSend( so, timeo)     ngSoSleepOn( so, timeo)
#define ngSoSleepOnCnct( so, timeo)     ngSoSleepOn( so, timeo)
void    ngSoFindEphemeral( NGsock *so, NGsock *head, NGushort *eport);
int     ngSoRecv( NGsock *so, NGiovec *iov, int iovcnt, int *flags, NGsockaddr *addr);
int     ngSoSend( NGsock *so, NGiovec *iov, int iovcnt, int flags, NGsockaddr *addr);


/* call socket user function */
#define NG_SOCKCALL_USER( so, code, data, len, addr) \
 ( (int (*)( NGsock *, int, void *, int, NGsockaddr *)) \
 (((NGproto *) (so)->so_proto)->pr_user_f))( so, code, data, len, addr)


/* free receive socket buffer */
#define NG_SOCK_RCVBUF_FLUSH( so) { \
    NGbuf *bufp, *tbufp; \
    while( 1) { \
        NG_QUEUE_OUT( &((so)->so_rcv_q), bufp); \
        if( bufp == NULL) break; \
        while( bufp) { \
            tbufp = bufp->buf_next; \
            ngBufFree( bufp); \
            bufp = tbufp; \
        } \
    } \
    (so)->so_rcv_cc = 0; \
}

/* socket state changes macros */
#define ngSoIsConnecting( so) (so)->so_state |= NG_SS_ISCONNECTING

#ifdef NG_RTOS

#define ngSoDataReceived( so) { \
    int count; \
    count = (so)->so_nsleep; \
    while( count--) ngOSSemPost( (so)->so_sem); \
}

#define ngSoDataSent( so) { \
    int count; \
    count = (so)->so_nsleep; \
    while( count--) ngOSSemPost( (so)->so_sem); \
}

#define ngSoIsDisconnecting( so) { \
    int count; \
    (so)->so_state &= ~NG_SS_ISCONNECTING; \
    (so)->so_state |= NG_SS_ISDISCONNECTING|NG_SS_CANTRCVMORE|NG_SS_CANTSENDMORE; \
    count = (so)->so_nsleep; \
    while( count--) ngOSSemPost( (so)->so_sem); \
}

#define ngSoCantRcvMore( so) { \
    int count; \
    (so)->so_state |= NG_SS_CANTRCVMORE; \
    count = (so)->so_nsleep; \
    while( count--) ngOSSemPost( (so)->so_sem); \
}

#define ngSoCantSendMore( so) { \
    int count; \
    (so)->so_state |= NG_SS_CANTSENDMORE; \
    count = (so)->so_nsleep; \
    while( count--) ngOSSemPost( (so)->so_sem); \
}

#else

#define ngSoDataReceived( so)

#define ngSoDataSent( so)

#define ngSoIsDisconnecting( so) { \
    (so)->so_state &= ~NG_SS_ISCONNECTING; \
    (so)->so_state |= NG_SS_ISDISCONNECTING|NG_SS_CANTRCVMORE|NG_SS_CANTSENDMORE; \
}

#define ngSoCantRcvMore( so) { \
    (so)->so_state |= NG_SS_CANTRCVMORE; \
}

#define ngSoCantSendMore( so) { \
    (so)->so_state |= NG_SS_CANTSENDMORE; \
}

#endif /* NG_RTOS */

NG_END_DECLS

#endif


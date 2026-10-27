/*****************************************************************************
 * $Id: ngsocket.h,v 1.2 2000/08/18 16:58:02 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * BSD Compatible Socket Interface
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
 *  This file includes parts which are Copyright (c) 1982-1988 Regents
 *  of the University of California.  All rights reserved.  The
 *  Berkeley Software License Agreement specifies the terms and
 *  conditions for redistribution.
 *----------------------------------------------------------------------------
 * 04/12/98 - Regis Feneon
 * 01/02/99 -
 *  fcntlsocket() and ioctlsocket() definitions added
 * 25/05/99 -
 *  inet_xxx() routines/macros added
 * 30/09/99 -
 *  htons(), htonl(), ntohs(), ntohl() macros added
 * 11/10/99 -
 *  now include ng_stdio.h for some definitions
 * 09/01/2000 -
 *  added definitions for selectsocket()
 * 16/02/2000 -
 *  send() is called inet_send() if NG_RTOS_OSE is defined
 *  send is a OSE kernel call
 *****************************************************************************/

#ifndef __NG_NGSOCKET_H_INCLUDED__
#define __NG_NGSOCKET_H_INCLUDED__

#include <ngip.h>
#include <ngdsock.h>

NG_BEGIN_DECLS

/* standart types */
typedef char *caddr_t;

/* generic socket address */
struct sockaddr {
    NGubyte     sa_len;         /* total length */
    NGubyte     sa_family;      /* address family */
    NGubyte     sa_data[14];    /* address value */
};

/* socket address in internet domain */
struct in_addr {
    NGuint      s_addr;         /* 32 bits IP address, net byte order */
};

struct sockaddr_in {
    NGubyte     sin_len;        /* total length (16) */
    NGubyte     sin_family;     /* AF_INET */
    NGushort    sin_port;       /* 16 bits port number, net byte order */
    struct in_addr sin_addr;    /* 32 bits IP address */
    NGubyte     sin_zero[8];    /* unused */
};

/* structure used by readv/writev/recvmsg/writemsg */
struct iovec {
    void   *iov_base;
    int     iov_len;
};
/* used by SO_SNDTIMEO/SO_RCVTIMEO socket options */
struct timeval {
    long    tv_sec;
    long    tv_usec;
};
/* use by SO_LINGER socket option */
struct linger {
    int     l_onoff;
    int     l_linger;
};

/* message structure for sendmsg/recvmsg */
struct msghdr {
    caddr_t     msg_name;       /* optional address */
    u_int       msg_namelen;    /* size of address */
    struct iovec *msg_iov;      /* scatter/gather array */
    u_int       msg_iovlen;     /* # elements in msg_iov */
    caddr_t     msg_control;    /* ancillary data */
    u_int       msg_controllen; /* ancillary data buffer len */
    int         msg_flags;      /* flags on received message */
};

struct cmsghdr {
	u_int       cmsg_len;       /* data byte count, including hdr */
	int         cmsg_level;     /* originating protocol */
	int         cmsg_type;      /* protocol-specific type */
                                /* followed by	u_char  cmsg_data[]; */
};

/* commands for fcntlsocket() */
#define F_GETFL         NG_FCNTL_GFLAGS     /* get flags */
#define F_SETFL         NG_FCNTL_SFLAGS     /* set flags */
#define O_NONBLOCK      NG_O_NONBLOCK       /* set/clear nonblocking io */

/* commands for ioctlsocket() */
#define IOC_OUT         0x40000000L     /* copy out parameters */
#define IOC_IN          0x80000000L     /* copy in parameters */
#define IOCPARM_MASK    0x1fff          /* parameter length, at most 13 bits */
#define _IOC(inout,group,num,len) \
        (inout | (((long)len & IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define _IOR(g,n,t)     _IOC(IOC_OUT,   (g), (n), sizeof(t))
#define _IOW(g,n,t)     _IOC(IOC_IN,    (g), (n), sizeof(t))

#define FIONBIO         _IOW('f', 126, int)     /* set/clear nonblocking io */
#define FIONREAD        _IOR('f', 127, int)     /* get # bytes to read */
#define SIOCATMARK      _IOR('s',  7, int)      /* at oob mark? */

/* definitions for selectsocket() */
typedef struct {
    u_long fds[(NG_FD_MAX+31)/32];
} fd_set;

#define FD_ZERO( fdset) { int i; for( i=0; i<((NG_FD_MAX+31)/32); i++) (fdset)->fds[i] = 0; }
#define FD_SET( fd, fdset)      ((fdset)->fds[(fd)>>5] |= 1UL<<((fd) & 0x1f))
#define FD_CLR( fd, fdset)      ((fdset)->fds[(fd)>>5] &= ~(1UL<<((fd) & 0x1f)))
#define FD_ISSET( fd, fdset)    ((fdset)->fds[(fd)>>5] & (1UL<<((fd) & 0x1f)))

/* prototypes */
int	accept( int, struct sockaddr *, int *);
int	bind( int, const struct sockaddr *, int);
int closesocket( int);
int	connect( int, const struct sockaddr *, int);
int fcntlsocket( int s, int cmd, ...);
int	getpeername( int, struct sockaddr *, int *);
int	getsockname( int, struct sockaddr *, int *);
int	getsockopt( int, int, int, void *, int *);
int ioctlsocket( int s, long cmd, int *arg);
int	listen( int, int);
/* int readsocket( int, void *, int); see below */
/* int recv( int, void *, int, int); see below */
int	recvfrom( int, void *, int, int, struct sockaddr *, int *);
int	recvmsg( int, struct msghdr *, int);
int selectsocket( int fdmax, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);
/* int send( int, const void *, int, int); see below */
int	sendto( int, const void *, int, int, const struct sockaddr *, int);
int	sendmsg( int, const struct msghdr *, int);
int	setsockopt( int, int, int, const void *, int);
int	shutdown( int, int);
int	socket( int, int, int);
/* int writesocket( int, void *, int); see below */

#define recv( s, buf, len, flags)   recvfrom( s, buf, len, flags, NULL, NULL)
#define readsocket( s, buf, len)    recvfrom( s, buf, len, 0, NULL, NULL)
#ifndef NG_RTOS_OSE
#define send( s, buf, len, flags)   sendto( s, buf, len, flags, NULL, 0)
#else
#define inet_send( s, buf, len, flags)  sendto( s, buf, len, flags, NULL, 0)
#endif
#define writesocket( s, buf, len)   sendto( s, buf, len, 0, NULL, 0)

/* internet address manipulation routines */
u_long  inet_addr( const char *);
/* int inet_aton(const char *, struct in_addr *); see below */
#define inet_aton( s, a)           (ngInetATON( (s), (NGuint *) (a)) ? 0 : 1)
char   *inet_ntoa( struct in_addr);

/* network/host byte order conversions */
#define ntohs( a)   ngNTOHS( a)
#define ntohl( a)   ngNTOHL( a)
#define htons( a)   ngHTONS( a)
#define htonl( a)   ngHTONL( a)

NG_END_DECLS

#endif


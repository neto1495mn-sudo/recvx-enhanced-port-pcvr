/*****************************************************************************
 * $Id: netbuf.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Network Buffers Management
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
 * 21/07/2000 - Regis Feneon
 *  see ngip/buf.h
 *****************************************************************************/

#ifndef __NG_NETBUF_H_INCLUDED__
#define __NG_NETBUF_H_INCLUDED__

#include <ngos/stdio.h>
#include <ngos/queue.h>

NG_BEGIN_DECLS

/*
 * Public
 */

typedef struct NGbuf_S {
    NGnode          buf_node;       /* linked list information */
    struct NGbuf_S *buf_next;       /* next chained buffer */
    void           *buf_ifnetp;     /* associated network interface */
    int             buf_flags;      /* flags: */
#define NG_BUFF_PROTO       0x000f  /* protocol mask */
#define NG_BUFF_RAW         0x0010  /* raw datagram */
#define NG_BUFF_BCAST       0x0020  /* broadcast datagram */
#define NG_BUFF_MCAST       0x0040  /* multicast datagram */
#define NG_BUFF_DONTFREE    0x0100  /* dont free buffer after sending */
#define NG_BUFF_BUSY        0x0200  /* buffer currently used by driver */
#define NG_BUFF_UNALIGNED   0x0400  /* data not aligned on word boundary */
#define NG_BUFF_EXTRADATA   0x1000  /* extra data associated */
#define NG_BUFF_MOREDATA    0x2000  /* another buffer chained */
#define NG_BUFF_SETIF       0x4000  /* set output interface from structure */
    NGubyte        *buf_datap;      /* pointer to data */
    int             buf_datalen;    /* size of data */
    NGiovec        *buf_iov;        /* external attached data */
    int             buf_iovcnt;     /* number of iov structures */
} NGbuf;

NG_END_DECLS

#endif


/*****************************************************************************
 * $Id: buf.h,v 1.3 2000/08/31 16:51:56 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
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
 * 07/09/98 - Regis Feneon
 * 22/12/98 -
 *  New buffer flags NG_BUFF_DONTFREE and NG_BUFF_BUSY
 *  Added ngBufOutputFree()
 * 29/12/98 -
 *  Free buffer queue name changed
 * 04/04/99 -
 *  ngBufGetPhysAddr() added for dma devices
 * 22/06/99 -
 *	 Parentheses for ngBufXXX macros parameter
 *  NGbuf : NG_BUFF_MCAST & NG_BUFF_SETIF flags added
 * 27/06/99 -
 *  added ngBufEnqueue()/ngBufDequeue() macros
 * 25/01/00 - Adrien Felon
 *  added NG_BUFF_IPFWD flag
 * 08/02/2000 - Regis Feneon
 *  removed NG_BUFF_IPFWD, NG_BUFF_NATFWP, ngBufEnqueue(), ngBufGetPhysAddr()
 * 25/01/00 - Adrien Felon
 *  added initialization of member buf_ifnetp of structure NGbuf
 *  in ngBufAlloc()
 * 18/07/2000 - Regis Feneon
 *  NGiovec moved to stdio.h
 *****************************************************************************/

#ifndef __NG_BUF_H_INCLUDED__
#define __NG_BUF_H_INCLUDED__

#include <ngos/stdio.h>
#include <ngos/queue.h>

NG_BEGIN_DECLS

/* memory management */
#define ngBufAlloc( bufp) { \
    int ictl; \
    ictl = ngOSIntrCtl( NG_INTRCTL_DISABLE); \
    NG_QUEUE_OUT( &ngBuf_freeq, (bufp)); \
    ngOSIntrCtl( ictl); \
    if( (bufp) != NULL) { \
        (bufp)->buf_flags = 0; \
        (bufp)->buf_ifnetp = NULL; \
        (bufp)->buf_next  = NULL; \
    } \
}
#define ngBufFree( bufp) { \
    int ictl; \
    ictl = ngOSIntrCtl( NG_INTRCTL_DISABLE); \
    NG_QUEUE_IN( &ngBuf_freeq, (bufp)); \
    ngOSIntrCtl( ictl); \
}

#define ngBufOutputFree( bufp) { \
    int ictl; \
    ictl = ngOSIntrCtl( NG_INTRCTL_DISABLE); \
    if( (bufp)->buf_flags & NG_BUFF_DONTFREE) \
        (bufp)->buf_flags &= ~NG_BUFF_BUSY; \
    else \
        NG_QUEUE_IN( &ngBuf_freeq, (bufp)); \
    ngOSIntrCtl( ictl); \
}

/* dequeue output buffer */
#define ngBufDequeue( netp, bufp) NG_QUEUE_OUT( &(netp)->if_outq, (bufp))

NG_END_DECLS

#endif

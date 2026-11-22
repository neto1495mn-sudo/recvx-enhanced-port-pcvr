/*****************************************************************************
 * $Id: netglob.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Network Global Variables
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
 *  v1.2 - moved from net.h
 *****************************************************************************/

#ifndef __NG_NETGLOB_H_INCLUDED__
#define __NG_NETGLOB_H_INCLUDED__

#include <ngos/queue.h>
#include <ngos/netfd.h>
#include <ngos/netbuf.h>
#include <ngos/netif.h>
#include <ngos/netprot.h>
#include <ngos/netsock.h>

NG_BEGIN_DECLS

/* global data */

extern NGproto *ngProtoList;            /* chained list of protocols */
extern NGifnet *ngIfList;               /* chained list of interfaces */

extern void    *ngBuf_pool;             /* pointer to message buffer pool */
extern NGuint   ngBuf_physaddr;         /* physical address of message buffer pool */
extern NGqueue  ngBuf_freeq;            /* queue of free message buffers */
extern NGqueue  ngBuf_inputq;           /* global message input queue */

extern u_int    ngBufDataMax;           /* maximum data size in message buffer */
extern u_int    ngBufDataOffset;        /* ip header offset in message buffer */

extern NGqueue  ngSock_freeq;           /* queue of free socket descriptors */
extern NGfdent *ngFd_table[NG_FD_MAX];  /* table of 'file' descriptors */

#ifdef NG_RTOS
/* internal tasks */
void ngInputFunc( void *data);
void ngTimerFunc( void *data);
/* tasks control blocks */
extern NGOStask     ngInputTask;
extern NGOStask     ngTimerTask;
/* synchros */
extern NGOSievent   ngInputEvent;

#endif

NG_END_DECLS

#endif


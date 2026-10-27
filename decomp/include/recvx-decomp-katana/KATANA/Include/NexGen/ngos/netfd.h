/*****************************************************************************
 * $Id: netfd.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * File Descriptors Definitions
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
 * 22/01/99 - Regis Feneon
 *  fd declarations moved from net.h
 *****************************************************************************/

#ifndef __NG_NET_FD_INCLUDED__
#define __NG_NET_FD_INCLUDED__

#include <ngos/queue.h>

NG_BEGIN_DECLS

/*
 * Public
 */

/* generic 'file' descriptor entry */
typedef struct {
    NGnode  fd_node;        /* link information */
    int     fd_type;        /* type of 'file' descriptor */
#define NG_FD_ANY       0   /* any... */
#define NG_FD_SOCK      1   /* socket */
#define NG_FD_DEV       2   /* serial device */
} NGfdent;

/*
 * Private
 */

int   ngFdAlloc( void *iocb);
void  ngFdFree( int fd);
void *ngFdGetPtr( int fd, int type);

NG_END_DECLS

#endif


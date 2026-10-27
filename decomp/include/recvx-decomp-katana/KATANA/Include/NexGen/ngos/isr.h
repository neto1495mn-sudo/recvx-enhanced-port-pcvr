/*****************************************************************************
 * $Id: isr.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Interrupt Handling Interface
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 * 	work and the use of a copyright notice does not imply otherwise.
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
 * 26/11/98 - Regis Feneon
 * 02/08/99 -
 *  used NGinterruptPTR
 *****************************************************************************/

#ifndef __NG_ISR_H_INCLUDED__
#define __NG_ISR_H_INCLUDED__

NG_BEGIN_DECLS

/* structure used by ngIsrXXX generic functions */
typedef struct {
    void             (*it_handler)(void *);     /* isr address */
    void              *it_datap;                /* associated data */
    NGinterruptPTR     it_oldvect;              /* saved vector */
    int                it_imask;                /* saved it mask */
} NGisrtable;

/* standard values for ngCpuIntrCtl() */
#define NG_INTRCTL_ENABLE       0x1000
#define NG_INTRCTL_DISABLE      0x1001

NG_END_DECLS

#endif


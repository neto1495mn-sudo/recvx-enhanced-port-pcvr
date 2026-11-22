/*****************************************************************************
 * $Id: hw.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Hardware Interface Definitions
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
 * 16/11/98 - Regis Feneon
 * 30/07/99 -
 *  use NGinterruptPTR type
 *****************************************************************************/

#ifndef __NG_HW_H_INCLUDED__
#define __NG_HW_H_INCLUDED__

NG_BEGIN_DECLS

/* generic clock handler */
void ngClockHandler( void);

/* setting interrupt vectors */
NGinterruptPTR ngItGetVect( int irno, int flags);
int            ngItSetVect( int irno, NGinterruptPTR handler, int flags);

/* flags for ngItSetVect() and ngItGetVect() */
#define NG_ITF_VECTOR       0x0001
#define NG_ITF_EXTERNAL	    0x0002
#define NG_ITF_IRQ          NG_ITF_EXTERNAL
#define NG_ITF_TIMER        0x0004
#define NG_ITF_MASK			0x0010
#define NG_ITF_UNMASK       0x0020

NG_END_DECLS

#endif


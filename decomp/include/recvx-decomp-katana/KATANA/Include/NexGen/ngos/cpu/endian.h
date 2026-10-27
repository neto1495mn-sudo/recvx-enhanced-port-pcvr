/*****************************************************************************
 * $Id: endian.h,v 1.1.1.1 2000/07/26 09:53:23 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * CPU files - Big/Little Endian conversion macros
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
 * 19/07/2000 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_ENDIAN_H_INCLUDED__
#define __NG_ENDIAN_H_INCLUDED__

/*
 * Byte swapping
 */

#define ngConstBSwap16(a)   ( ((((NGushort) (a)) << 8)&0xff00) | \
                              ((((NGushort) (a)) >> 8)&0x00ff) )
#define ngConstBSwap32(a)   ( ((((NGuint) (a)) << 24)&0xff000000U) | \
                              ((((NGuint) (a)) <<  8)&0x00ff0000U) | \
                              ((((NGuint) (a)) >>  8)&0x0000ff00U) | \
                              ((((NGuint) (a)) >> 24)&0x000000ffU) )

#if defined (NG_LITTLE_ENDIAN)
#define ngHTOLE16( val)         (val)
#define ngHTOLE32( val)         (val)
#define ngLETOH16( val)         (val)
#define ngLETOH32( val)         (val)
#define ngHTOBE16( val)         ngBSwap16( val)
#define ngHTOBE32( val)         ngBSwap32( val)
#define ngBETOH16( val)         ngBSwap16( val)
#define ngBETOH32( val)         ngBSwap32( val)
#define ngConstHTOLE16( val)    (val)
#define ngConstHTOLE32( val)    (val)
#define ngConstLETOH16( val)    (val)
#define ngConstLETOH32( val)    (val)
#define ngConstHTOBE16( val)    ngConstBSwap16( val)
#define ngConstHTOBE32( val)    ngConstBSwap32( val)
#define ngConstBETOH16( val)    ngConstBSwap16( val)
#define ngConstBETOH32( val)    ngConstBSwap32( val)
#elif defined (NG_BIG_ENDIAN)
#define ngHTOLE16( val)         ngBSwap16( val)
#define ngHTOLE32( val)         ngBSwap32( val)
#define ngLETOH16( val)         ngBSwap16( val)
#define ngLETOH32( val)         ngBSwap32( val)
#define ngHTOBE16( val)         (val)
#define ngHTOBE32( val)         (val)
#define ngBETOH16( val)         (val)
#define ngBETOH32( val)         (val)
#define ngConstHTOLE16( val)    ngConstBSwap16( val)
#define ngConstHTOLE32( val)    ngConstBSwap32( val)
#define ngConstLETOH16( val)    ngConstBSwap16( val)
#define ngConstLETOH32( val)    ngConstBSwap32( val)
#define ngConstHTOBE16( val)    (val)
#define ngConstHTOBE32( val)    (val)
#define ngConstBETOH16( val)    (val)
#define ngConstBETOH32( val)    (val)
#else
#error "NG_XXX_ENDIAN macro not defined"
#endif

#endif


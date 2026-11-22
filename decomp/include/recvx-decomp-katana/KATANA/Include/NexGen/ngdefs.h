/*****************************************************************************
 * $Id: ngdefs.h,v 1.1.1.1 2000/09/18 11:04:32 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * SEGA Dreamcast Porting Files
 *----------------------------------------------------------------------------
 *		Copyright (c) 2000 NexGen Software.
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
 *    	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 06/09/2000 - Regis Feneon
 *  v1.2 update
 *****************************************************************************/

#ifndef __NG_NGDEFS_H_INCLUDED__
#define __NG_NGDEFS_H_INCLUDED__

#ifdef NG_RTOS
#error "Must be linked with POLLING libraries"
#endif

/* chipset clocks */
#define NG_BOARD_FREQ   100000000           /* 100 MHz cristal */
#define NG_CPU_FREQ     (NG_BOARD_FREQ*2)   /* 200 MHz cpu clock */
#define SH4_PPHI_FREQ   (NG_BOARD_FREQ/2)   /*  50 MHz peripherals clock */

/* system clock */
#define NG_CLOCK_FREQ   (SH4_PPHI_FREQ/64)  /* 781250 Hz timer */

#endif


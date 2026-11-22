/*****************************************************************************
 * $Id: rtos.h,v 1.1.1.1 2000/07/26 09:53:23 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * RTOS Version Global Object Declaration
 * Note: It must be included in one source file only
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
 * 14/01/99 - Regis Feneon
 * 18/07/2000 -
 *  added devices support
 *****************************************************************************/

#ifdef NG_RTOS

/* global lock */
NGOSmutex   ngGlobalLock;
/* network */
NGOSievent  ngInputEvent;
NGOStask    ngInputTask;
NGOStask    ngTimerTask;
/* devices */
NGOSievent  ngDevioEvent;
NGOStask    ngDevioTask;

#endif


/*****************************************************************************
 * $Id: ngsegadc.h,v 1.1.1.1 2000/09/18 11:04:32 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Sega Dreamcast Porting Files
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
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 06/09/2000 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_NGSEGADC_H_INCLUDED__
#define __NG_NGSEGADC_H_INCLUDED__

#include <ngos.h>
#include <ngdev.h>
#include <ngnet.h>
#include <sh4.h>

extern const ngSerDrv_Trisignal;

/* ethernet driver */
enum {
    DCLAN_AUTO = 0,    /* Auto detect speed */
    DCLAN_10BaseT,     /* 10BaseT */
    DCLAN_10BaseTX,    /* 10BaseT Full Duplex */
    DCLAN_100BaseT,    /* 100BaseT */
    DCLAN_100BaseTX    /* 100BaseT Full Duplex */
};

#define NG_IFO_DCLAN_REQUESTED_SPEED (NG_IFO_DEV1)
#define NG_IFO_DCLAN_CURRENT_SPEED   (NG_IFO_DEV2)

extern const ngNetDrv_DCLAN;

#endif

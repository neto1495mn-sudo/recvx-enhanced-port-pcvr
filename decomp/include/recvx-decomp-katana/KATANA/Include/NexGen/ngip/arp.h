/*****************************************************************************
 * $Id: arp.h,v 1.2 2000/08/18 16:58:06 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Address Resolution Protocol
 * Low Level Definitions
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
 * 17/09/98 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_ARP_H_INCLUDED__
#define __NG_ARP_H_INCLUDED__

NG_BEGIN_DECLS

/* ethernet ARP message */
typedef struct {
    NGushort    arp_hrd;            /* format of hardware address */
#define ARPHRD_ETHER        1
    NGushort    arp_pro;            /* format of protocol address */
    NGubyte     arp_hln;            /* lenght of hardware address */
    NGubyte     arp_pln;            /* lenght of protocol address */
    NGushort    arp_op;             /* ARP/RARP operation */
#define ARPOP_REQUEST       1
#define ARPOP_REPLY         2
#define ARPOP_REVREQUEST    3
#define ARPOP_REVREPLY      4
    NGubyte     arp_sha[6];         /* sender hardware address */
    NGubyte     arp_spa[4];         /* sender protocol address */
    NGubyte     arp_tha[6];         /* target hardware address */
    NGubyte     arp_tpa[4];         /* target protocol address */
} NGetherarp;

NG_END_DECLS

#endif


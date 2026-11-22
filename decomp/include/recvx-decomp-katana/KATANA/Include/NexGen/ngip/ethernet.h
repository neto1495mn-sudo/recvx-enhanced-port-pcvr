/*****************************************************************************
 * $Id: ethernet.h,v 1.2 2000/08/18 16:58:07 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Ethernet Protocol
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
 * 07/09/98 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_ETHERNET_H_INCLUDED__
#define __NG_ETHERNET_H_INCLUDED__

NG_BEGIN_DECLS

/* ethernet header */
typedef struct {
    NGubyte     ether_dhost[6];   /* destination address */
    NGubyte     ether_shost[6];   /* source address */
    NGushort    ether_type;       /* frame type */
#define ETHERTYPE_IP            0x0800  /* IP protocol */
#define ETHERTYPE_ARP           0x0806  /* Address Resolution Protocol */
#define ETHERTYPE_REVARP        0x8035  /* Reverse Addr Resolution Protocol */
#define ETHERTYPE_IPTRAILERS    0x1000  /* Trailer packet */
#define	ETHERTYPE_PPPOEDISC     0x8863  /* PPP Over Ethernet Discovery Stage */
#define	ETHERTYPE_PPPOE         0x8864  /* PPP Over Ethernet Session Stage */
} NGetherhdr;

#define ETHERMTU            1500
#define ETHERMIN            46

NG_END_DECLS

#endif


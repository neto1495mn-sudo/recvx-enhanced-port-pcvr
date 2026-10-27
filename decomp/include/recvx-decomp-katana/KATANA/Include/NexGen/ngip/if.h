/*****************************************************************************
 * $Id: if.h,v 1.3 2000/09/12 19:04:18 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Low-Level Network Interface Structures
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
 * 29/09/98 - Regis Feneon
 * 31/10/98 -
 * 09/12/98 -
 *  Added ppp interface and protocol support
 *  new configuration system - supressed NGifcfg structure
 * 22/12/98 - Thierry Chantry
 *  Added ppp specific data structure
 * 29/12/98 - Regis Feneon
 *  Specific adaptator structures moved to net_eth.h and net_ppp.h
 *  network protocol structure removed
 *  init function in driver added
 *  in NGifnet removed if_args, if_devaddr, if_devaddrlen
 * 12/01/99 -
 *  clarification in adaptator specific data now in if_data[]
 * 30/06/99 -
 *  structure NGnetdrv: added netd_mtu and netd_bps members
 *                      suppressed netd_init_f
 *  ngIfGenInit() prototype suppressed
 * 15/09/99 -
 *  structure NGnetdrv: netd_init_f is back
 * 30/09/99 -
 *  ngIfAdd(), ngIfOpen(), ngIfClose(), ngIfSend() removed
 * 24/01/00 - Adrien Felon
 *  flags NG_IFF_EXTNAT added for external NAT interfaces
 * 08/02/2000 - Regis Feneon
 *  added proto for ngIfGenInput()
 * 24/02/2000 -
 *  added test for previous definition of NG_IFDATA_MAX
 * 07/03/2000 - Adrien Felon
 *  ngIfOpen() prototype added
 * 15/03/2000 -
 *  ngIfClose() prototype added
 * 19/07/2000 - Regis Feneon
 *  structure NGifnet:
 *   if_data removed
 *   if_buflist,if_dev1,if_dev2,if_devptr1,if_devptr2 added
 *   options NG_IFO_DEV1,NG_IFO_DEV2,NG_IFO_DEVPTR1,NG_IFO_DEVPTR2 added
 *  added protos for ngIfGenBufAttach() and ngIfGenBufDetach()
 *  added fromisr parameter to ngIfGenInput()
 *****************************************************************************/

#ifndef __NG_IF_H_INCLUDED__
#define __NG_IF_H_INCLUDED__

#include <ngos/netif.h>

NG_BEGIN_DECLS

/* interface options */
#define NG_IFO_NAME         0x0100
#define NG_IFO_DRIVER       0x0101
#define NG_IFO_OUTQ_MAX     0x0102
#define NG_IFO_ADDR         0x0103
#define NG_IFO_NETMASK      0x0104
#define NG_IFO_BRDADDR      0x0105
#define NG_IFO_DSTADDR      0x0106
#define NG_IFO_FLAGS        0x0107
#define NG_IFO_ADDMULTI     0x0108
#define NG_IFO_DELMULTI     0x0109
#define NG_IFO_MTU          0x010a
#define NG_IFO_BPS          0x010b
#define NG_IFO_DEV1         0x010c
#define NG_IFO_DEV2         0x010d
#define NG_IFO_DEVPTR1      0x010e
#define NG_IFO_DEVPTR2      0x010f
 
/*
 * User Functions
 */

int      ngIfOpen( NGifnet *netp);
int      ngIfClose( NGifnet *netp);
NGifnet *ngIfGetPtr( const char *name);
int      ngIfSetAddr( NGifnet *netp, NGuint addr, NGuint netmask);
int      ngIfSetOption( NGifnet *netp, int opt, void *optval);
int      ngIfGetOption( NGifnet *netp, int opt, void *optval);

/*
 * Private
 */

/* generic interface functions */
int    ngIfGenCntl( NGifnet *netp, int cmd, int opt, void *optval);
void   ngIfGenInput( NGifnet *netp, NGbuf *bufp, int fromisr);
/* internal buffer management */
void   ngIfBufAttach( NGifnet *netp, NGbuf *bufp);
NGbuf *ngIfBufDetach( NGifnet *netp, void *datap);

NG_END_DECLS

#endif


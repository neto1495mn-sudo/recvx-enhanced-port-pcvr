/*****************************************************************************
 * $Id: debug.h,v 1.4 2000/09/15 10:01:47 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Debug and Trace Module
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
 * 03/12/98 - Regis Feneon
 * 18/10/99 -
 *  added FTP,Telnet,Web
 * 25/01/00 - Adrien Felon
 *  added NAT
 * 11/09/2000 - Regis Feneon
 *  added global variables
 *****************************************************************************/

#ifndef __NG_DEBUG_H_INCLUDED__
#define __NG_DEBUG_H_INCLUDED__

NG_BEGIN_DECLS

/* module */
#define NG_DBG_CORE     1
#define NG_DBG_DRV      2
#define NG_DBG_ETH      3
#define NG_DBG_ARP      4
#define NG_DBG_PPP      5
#define NG_DBG_IP       6
#define NG_DBG_ICMP     7
#define NG_DBG_UDP      8
#define NG_DBG_TCP      9
#define NG_DBG_RAWIP   10
#define NG_DBG_SOCK    11
#define NG_DBG_APP     12
#define NG_DBG_BOOTP   13
#define NG_DBG_DHCP    14
#define NG_DBG_TFTP    15
#define NG_DBG_MAIL    16
#define NG_DBG_FTP     17
#define NG_DBG_FTPS    18
#define NG_DBG_FTPC    19
#define NG_DBG_TELNET  20
#define NG_DBG_TELNETC 21
#define NG_DBG_WEB     22
#define NG_DBG_NAT     23
#define NG_DBG_PPPOE   24
#define NG_DBG_MAX     24

/* sub-module */
#define NG_DBG_INIT     1
#define NG_DBG_EXIT     2
#define NG_DBG_INPUT    3
#define NG_DBG_OUTPUT   4
#define NG_DBG_TIMER    5
#define NG_DBG_USER     6

/* prototypes */
void ngDebug( int module, int submod, int level, const char *fmt,...);
void ngDebugSetLevel( int level);
void ngDebugSetModule( int module, int onoff);

/* globals */
extern int         ngDebugLevel;
extern int         ngDebugModOnOff[];
extern const char *ngDebugModStr[];

NG_END_DECLS

#endif


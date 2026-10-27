/*****************************************************************************
 * $Id: ppp.h,v 1.3 2000/09/12 19:05:34 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Point-to-Point Protocol definitions
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998-2000 NexGen Software.
 *
 *    All rights reserved. NexGen Software' source code is an
 *  unpublished work and the use of a copyright notice does not imply
 *  otherwise.  This source code contains confidential, trade secret
 *  material of NexGen Software. Any attempt or participation in
 *  deciphering, decoding, reverse engineering or in any way altering
 *  the source code is strictly prohibited, unless the prior written
 *  consent of NexGen Software is obtained.
 *
 *    This software is supplied under the terms of a license agreement
 *  or nondisclosure agreement with NexGen Software, and may not be
 *  copied or disclosed except in accordance with the terms of that
 *  agreement.
 *
 *----------------------------------------------------------------------------
 * 17/12/1998 - Thierry Chantry
 * 31/03/2000 - Adrien Felon
 *  ngDebugPppDump() prototype added
 * 13/07/2000 -
 *  adding/correcting comments
 * 12/09/2000 - Regis Feneon
 *  removed NG_PPP_MRU_MIN/NG_PPP_MRU_MAX
 *****************************************************************************/

#ifndef __NG_PPP_PPP_H_INCLUDED
#define __NG_PPP_PPP_H_INCLUDED

NG_BEGIN_DECLS

/*
 * The basic PPP frame.
 */
#define PPP_PROTOLEN 2  /* length of PPP protocol number field */

/*
 * Packet sizes
 *
 * Note - lcp shouldn't be allowed to negotiate stuff outside these
 *  limits. See lcp.h in the pppd directory.
 * (XXX - these constants should simply be shared by lcp.c instead
 *  of living in lcp.h)
 */

#define PPP_MTU      1500     /* Default MTU (size of Info field) */
#define PPP_MAXMTU  65535 - (PPP_HDRLEN + PPP_FCSLEN)
#define PPP_MINMTU     64
#define PPP_MRU      1500     /* default MRU = max length of info field */
#define PPP_MAXMRU  65000     /* Largest MRU we allow */
#define PPP_MINMRU    128

/*
 * Protocol field values.
 */
#define PPP_IP         0x0021 /* Internet Protocol */
#define PPP_AT         0x0029 /* AppleTalk Protocol */
#define PPP_IPX        0x002b /* IPX protocol */
#define PPP_VJC_COMP   0x002d /* VJ compressed TCP */
#define PPP_VJC_UNCOMP 0x002f /* VJ uncompressed TCP */
#define PPP_IPV6       0x0057 /* Internet Protocol Version 6 */
#define PPP_COMP       0x00fd /* compressed packet */
#define PPP_IPCP       0x8021 /* IP Control Protocol */
#define PPP_ATCP       0x8029 /* AppleTalk Control Protocol */
#define PPP_IPXCP      0x802b /* IPX Control Protocol */
#define PPP_CCP        0x80fd /* Compression Control Protocol */
#define PPP_LCP        0xc021 /* Link Control Protocol */
#define PPP_PAP        0xc023 /* Password Authentication Protocol */
#define PPP_LQR        0xc025 /* Link Quality Report protocol */
#define PPP_CHAP       0xc223 /* Cryptographic Handshake Auth. Protocol */
#define PPP_CBCP       0xc029 /* Callback Control Protocol */

#ifdef NG_DEBUG
void ngDebugPppDump(
   NGbuf    *bufp,
   NGifnet  *netp,
   int       module,
   int       submod,
   int       evel);
#endif

NG_END_DECLS

#endif


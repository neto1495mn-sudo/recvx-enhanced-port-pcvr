/*****************************************************************************
 * $Id: ip.h,v 1.2 2000/08/18 16:58:07 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Internet Protocol Definitions
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998-2000 NexGen Software.
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
 * 13/01/99 -
 *  ip address definitions and udp/tcp port list moved to net_ip.h
 * 20/03/2000 -
 *  added test for shc compiler
 * 30/05/2000 -
 *  replaced endianness test with NG_XXX_ENDIAN_BITFIELD test
 * 19/07/2000 -
 *  list of protocols moved to ngip.h
 *****************************************************************************/

#ifndef __NG_IP_H_INCLUDED__
#define __NG_IP_H_INCLUDED__

NG_BEGIN_DECLS

/* internet header */
typedef struct {
#ifdef NG_LITTLE_ENDIAN_BITFIELD
    NGubyte     ip_hl:4,    /* header lenght */
                ip_v:4;     /* version */
#elif defined(NG_BIG_ENDIAN_BITFIELD)
    NGubyte     ip_v:4,     /* version */
                ip_hl:4;    /* header lenght */
#else
#error "NG_XXX_ENDIAN_BITFIELD not defined"
#endif
#define IPVERSION    4              /* current IP version */
    NGubyte     ip_tos;     /* type of service */
#define IPTOS_LOWDELAY      0x10    /* minimize delay */
#define IPTOS_THROUGHPUT    0x08    /* maximize throughput */
#define IPTOS_RELIABILITY   0x04    /* maximize reliability */
    NGushort    ip_len;     /* total length */
    NGushort    ip_id;      /* identification */
    NGushort    ip_off;     /* fragment offset field */
#define IPOFF_DF            0x4000  /* dont fragment flag */
#define IPOFF_MF            0x2000  /* more fragment flag */
#define IPOFF_MASK          0x1fff  /* mask for fragmenting bits */
    NGubyte     ip_ttl;     /* time to live */
    NGubyte     ip_p;       /* protocol */
    NGushort    ip_sum;     /* checksum */
    NGuint      ip_src;     /* source address */
    NGuint      ip_dst;     /* destination address */
} NGiphdr;

#define MAXTTL  255

NG_END_DECLS

#endif


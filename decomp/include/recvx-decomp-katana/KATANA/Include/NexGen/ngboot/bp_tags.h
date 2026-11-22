/*****************************************************************************
 * $Id: bp_tags.h,v 1.1.1.1 2000/09/19 09:50:37 af Exp $
 *----------------------------------------------------------------------------
 * NexgenBOOT v1.2
 * Macros for BOOTP and DHCP option tags
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
 * 16/06/1999 - Adrien Felon
 *****************************************************************************/

#ifndef __BP_TAGS_H_INCLUDED__
#define __BP_TAGS_H_INCLUDED__

NG_BEGIN_DECLS

/* Magic cookie for vendor option */
#define VD_MAGIC_COOKIE ((NGuint)0x63825363)  /* eg. 99.130.83.99 in decimal */

/* Well-known vendor options */

/*
 * Tag values used to specify what information is being supplied in
 * the vendor (options) data area of the packet.
 */
/* RFC 1048 */
/* End of cookie */
#define TAG_END            ((NGubyte) 255)
/* padding for alignment */
#define TAG_PAD            ((NGubyte)   0)
/* Subnet mask */
#define TAG_SUBNET_MASK    ((NGubyte)   1)
/* Time offset from UTC for this system */
#define TAG_TIME_OFFSET    ((NGubyte)   2)
/* List of routers on this subnet */
#define TAG_GATEWAY        ((NGubyte)   3)
/* List of rfc868 time servers available to client */
#define TAG_TIME_SERVER    ((NGubyte)   4)
/* List of IEN 116 name servers */
#define TAG_NAME_SERVER    ((NGubyte)   5)
/* List of DNS name servers */
#define TAG_DOMAIN_SERVER  ((NGubyte)   6)
/* List of MIT-LCS UDL log servers */
#define TAG_LOG_SERVER     ((NGubyte)   7)
/* List of rfc865 cookie servers */
#define TAG_COOKIE_SERVER	((NGubyte)   8)
/* List of rfc1179 printer servers (in order to try) */
#define TAG_LPR_SERVER		((NGubyte)   9)
/* List of Imagen Impress servers (in prefered order) */
#define TAG_IMPRESS_SERVER	((NGubyte)  10)
/* List of rfc887 Resourse Location servers */
#define TAG_RLP_SERVER		((NGubyte)  11)
/* Hostname of client */
#define TAG_HOST_NAME		((NGubyte)  12)
/* boot file size */
#define TAG_BOOT_SIZE		((NGubyte)  13)
/* RFC 1395 */
/* path to dump to in case of crash */
#define TAG_DUMP_FILE		((NGubyte)  14)
/* domain name for use with the DNS */
#define TAG_DOMAIN_NAME		((NGubyte)  15)
/* IP address of the swap server for this machine */
#define TAG_SWAP_SERVER		((NGubyte)  16)
/* The path name to the root filesystem for this machine */
#define TAG_ROOT_PATH      ((NGubyte)  17)
/* RFC 1497 */
/* filename to tftp with more options in it */
#define TAG_EXTEN_FILE		((NGubyte)  18)
/* RFC 1533 */
/* The following are in rfc1533 and may be used by BOOTP/DHCP */
/* IP forwarding enable/disable */
#define TAG_IP_FORWARD          ((NGubyte)  19)
/* Non-Local source routing enable/disable */
#define TAG_IP_NLSR             ((NGubyte)  20)
/* List of pairs of addresses/masks to allow non-local source routing to */
#define TAG_IP_POLICY_FILTER    ((NGubyte)  21)
/* Maximum size of datagrams client should be prepared to reassemble */
#define TAG_IP_MAX_DRS          ((NGubyte)  22)
/* Default IP TTL */
#define TAG_IP_TTL              ((NGubyte)  23)
/* Timeout in seconds to age path MTU values found with rfc1191 */
#define TAG_IP_MTU_AGE          ((NGubyte)  24)
/* Table of MTU sizes to use when doing rfc1191 MTU discovery */
#define TAG_IP_MTU_PLAT         ((NGubyte)  25)
/* MTU to use on this interface */
#define TAG_IP_MTU              ((NGubyte)  26)
/* All subnets are local option */
#define TAG_IP_SNARL            ((NGubyte)  27)
/* broadcast address */
#define TAG_IP_BROADCAST        ((NGubyte)  28)
/* perform subnet mask discovery using ICMP */
#define TAG_IP_SMASKDISC        ((NGubyte)  29)
/* act as a subnet mask server using ICMP */
#define TAG_IP_SMASKSUPP        ((NGubyte)  30)
/* perform rfc1256 router discovery */
#define TAG_IP_ROUTERDISC       ((NGubyte)  31)
/* address to send router solicitation requests */
#define TAG_IP_ROUTER_SOL_ADDR  ((NGubyte)  32)
/* list of static routes to addresses (addr, router) pairs */
#define TAG_IP_STATIC_ROUTES    ((NGubyte)  33)
/* use trailers (rfc893) when using ARP */
#define TAG_IP_TRAILER_ENC      ((NGubyte)  34)
/* timeout in seconds for ARP cache entries */
#define TAG_ARP_TIMEOUT         ((NGubyte)  35)
/* use either Ethernet version 2 (rfc894) or IEEE 802.3 (rfc1042) */
#define TAG_ETHER_IEEE          ((NGubyte)  36)
/* default TCP TTL when sending TCP segments */
#define TAG_IP_TCP_TTL          ((NGubyte)  37)
/* time for client to wait before sending a keepalive on a TCP connection */
#define TAG_IP_TCP_KA_INT       ((NGubyte)  38)
/* don't send keepalive with an octet of garbage for compatability */
#define TAG_IP_TCP_KA_GARBAGE   ((NGubyte)  39)
/* NIS domainname */
#define TAG_NIS_DOMAIN	        ((NGubyte)  40)
/* list of NIS servers */
#define TAG_NIS_SERVER          ((NGubyte)  41)
/* list of NTP servers */
#define TAG_NTP_SERVER          ((NGubyte)  42)
/* and stuff vendors may want to add */
#define TAG_VEND_SPECIFIC       ((NGubyte)  43)
/* NetBios over TCP/IP name server */
#define TAG_NBNS_SERVER         ((NGubyte)  44)
/* NetBios over TCP/IP NBDD servers (rfc1001/1002) */
#define TAG_NBDD_SERVER         ((NGubyte)  45)
/* NetBios over TCP/IP node type option for use with above */
#define TAG_NBOTCP_OTPION       ((NGubyte)  46)
/* NetBios over TCP/IP scopt option for use with above */
#define TAG_NB_SCOPE            ((NGubyte)  47)
/* list of X Window system font servers */
#define TAG_XFONT_SERVER        ((NGubyte)  48)
/* list of systems running X Display Manager (xdm) available to this client */
#define TAG_XDISPLAY_SERVER     ((NGubyte)  49)

NG_END_DECLS

#endif /* __BP_TAGS_H_INCLUDED__ */

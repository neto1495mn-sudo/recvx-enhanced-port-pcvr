/*****************************************************************************
 * $Id: dhcp.h,v 1.1.1.1 2000/09/19 09:50:37 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT v1.2
 * Dynamic Host Configuration Protocol definition
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
 * DHCP packet structure (NGdhcpppk)
 * Macros for DHCP option tags and option values
 * structure NGdhcpoption
 *----------------------------------------------------------------------------
 * 01/07/1999 - Adrien Felon - Initial releas
 * 06/04/2000 - Regis Feneon
 *  removed IPPORT_BOOTP defines (already defined in net_ip.h)
 * 12/04/2000 - Adrien Felon
 *  all client code moved to dhcpc.h
 *****************************************************************************/

#ifndef __DHCP_H_INCLUDED__
#define __DHCP_H_INCLUDED__

#include <ngos.h>
#include <ngip/ethernet.h> /* ETHERMTU */

NG_BEGIN_DECLS

/*****************************************************************************/
/* Defining pure DHCP protocol */
/*****************************************************************************/

/* Some macros to deal with DHCP packet length */
/* fixed len fields */
#define DHCP_CHADDR_LEN     16
#define DHCP_SNAME_LEN      64
#define DHCP_FILE_LEN      128
/* Some others to compute max option field length */
/*   Note: recall that IP/UDP overhead is 28 = ip (20) + udp (8) */
#define DHCP_FIXED_NON_UDP 236 /* DHCP packet len w/o IP / UDP / option field */
#define DHCP_FIXED_LEN     264 /* DHCP_UDP_OVERHEAD + DHCP_FIXED_NON_UDP */ 
#define DHCP_MTU_MAX       ETHERMTU
#define DHCP_OPTION_LEN    (DHCP_MTU_MAX - DHCP_FIXED_LEN)
/* Finally: minimum DHCP packet size (without IP/UDP overheads) */
#define DHCP_MIN_LEN       548 /* got this one from Linux... */

/* NGdhcppk defines DHCP packets format (very like BOOTP packets...) */
typedef struct {
  NGubyte  op;     /* packet opcode / message type */
#define BOOTREQUEST     1
#define BOOTREPLY       2
  NGubyte  htype;  /* Hardware address type */
  NGubyte  hlen;   /* hardware address length */
  NGubyte  hops;   /* Client sets to zero, optionally used in cross-gatew. booting */
  NGuint   xid;    /* transaction ID */
  NGushort secs;   /* Client data, seconds elapsed since 1s boot request */
  NGushort flags;  /* RFC1532 broadcast flag.  not used by the stack... */
#define BP_FBROADCAST   (ngHTONS((NGushort)1<<15))
  NGuint   ciaddr; /* Client IP address (if already in use) */
  NGuint   yiaddr; /* Client IP address */
  NGuint   siaddr; /* IP address of next server to talk to */
  NGuint   giaddr; /* DHCP relay agent IP address */
  NGubyte  chaddr[DHCP_CHADDR_LEN];  /* client hardware addr */
  NGubyte  sname[DHCP_SNAME_LEN];    /* Server name */
  NGubyte  file[DHCP_FILE_LEN];      /* Boot file name */
  NGubyte  options[DHCP_OPTION_LEN]; /* Options (size depends on MTU) */
} NGdhcppk;

/* Options tags */

/* BOOTP tags */
#include <ngboot/bp_tags.h>

/* DHCP specific option tags */
/* requested IP address */
#define TAG_DHCP_REQ_IP         ((NGubyte)  50)
/* IP address lease time */
#define TAG_DHCP_LEASE_TIME     ((NGubyte)  51)
/* options overload (1:file, 2:sname, 3:both) */
#define TAG_DHCP_OPTOVER        ((NGubyte)  52)
/* type of DHCP message */
#define TAG_DHCP_MESS_TYPE      ((NGubyte)  53)
/* server identification */
#define TAG_DHCP_SERVER_ID      ((NGubyte)  54)
/* ordered list of requested option */
#define TAG_DHCP_REQ_LIST       ((NGubyte)  55)
/* reply message */
#define TAG_DHCP_TEXT_MESSAGE   ((NGubyte)  56)
/* largest DHCP message client will accept */
#define TAG_DHCP_MAX_MSGSZ      ((NGubyte)  57)
/* time 'til client needs to renew (T1) */
#define TAG_DHCP_RENEWAL_TIME   ((NGubyte)  58)
/* time 'til client needs to rebind (T2) */
#define TAG_DHCP_REBIND_TIME    ((NGubyte)  59)
/* vendor class identifier */
#define TAG_DHCP_CLASSID        ((NGubyte)  60)
/* client identifier (unique on client's subnet) */
#define TAG_DHCP_CLIENTID       ((NGubyte)  61)

/* some option missing ??? */

/* name of NIS domain */
#define TAG_DHCP_NIS_DOMAIN     ((NGubyte)  64)
/* IP addresses of NIS servers */
#define TAG_DHCP_NIS_SERVERS    ((NGubyte)  65)
/* name of TFTP server (if sname has been used for options) */
#define TAG_DHCP_TFTP_SERVER    ((NGubyte)  66)
/* bootfile name (if sname has been used for options) */
#define TAG_DHCP_BOOTFILE       ((NGubyte)  67)
/* IP addresses of mobile IP home agents */
#define TAG_DHCP_MOB_AGENTS     ((NGubyte)  68)
/* IP addresses of SMTP servers */
#define TAG_DHCP_SMTP_SERVERS   ((NGubyte)  69)
/* IP addresses of POP3 servers */
#define TAG_DHCP_POP3_SERVERS   ((NGubyte)  70)
/* IP addresses of NNTP servers */
#define TAG_DHCP_NNTP_SERVERS   ((NGubyte)  71)
/* IP addresses of WWW servers */
#define TAG_DHCP_WWW_SERVERS    ((NGubyte)  72)
/* IP addresses of finger servers */
#define TAG_DHCP_FING_SERVERS   ((NGubyte)  73)
/* IP addresses of IRC servers */
#define TAG_DHCP_IRC_SERVERS    ((NGubyte)  74)
/* IP addresses of StreetTalk servers */
#define TAG_DHCP_ST_SERVERS     ((NGubyte)  75)
/* IP addresses of StreetTalk Directory Assistance servers */
#define TAG_DHCP_STDA_SERVERS   ((NGubyte)  76)

/* Please add any new tags here */

/* DHCP option values */

/* DHCP message types (see TAG_DHCP_MESS_TYPE) */
#define DHCPDISCOVER    1
#define DHCPOFFER       2
#define DHCPREQUEST     3
#define DHCPDECLINE     4
#define DHCPACK         5
#define DHCPNACK        6
#define DHCPRELEASE     7
#define DHCPINFORM      8

/* Overload option tag values (see TAG_DHCP_OPTOVER) */
#define DHCP_OPTOVER_FILE  1 /* Only file is filled with options */
#define DHCP_OPTOVER_SNAME 2 /* Only sname is filled with options */
#define DHCP_OPTOVER_BOTH  3 /* Both file and sname are filled with options */

/* structure to allow to request specific options */
typedef struct {
  NGubyte   do_tag;          /* tag (cf. header files dhcp.h bp_tags.h )*/
  NGubyte   do_size;         /* length of the value (bytes count) */
  NGubyte * do_data;         /* location of data */
} NGdhcpoption;

NG_END_DECLS

#endif /* __DHCP_H_INCLUDED__ */

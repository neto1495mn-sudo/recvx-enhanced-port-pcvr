/*****************************************************************************
 * $Id: bootp.h,v 1.2 2000/10/06 13:00:28 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT v1.2
 * BOOTP header file
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
 * struct NGbootppk; (BOOTP packet)
 *----------------------------------------------------------------------------
 * 14/01/1998 - Thierry Chantry
 * 16/05/1999 - Adrien Felon
 *   - field resv1 changed to flags, added some macros
 * 06/06/1999
 *   - some tags added for DHCP messages (options from 64 to 76)
 * 16/06/1999
 *   - all tags moved to file "bp_tags.h"
 * 06/04/2000 - Regis Feneon
 *   - removed IPPORT_BOOTP defines (already defined in net_ip.h)
 * 06/10/2000 - Adrien Felon
 *   - defining macro BOOTPC_PREVENT_UNICAST to force broadcast
 *****************************************************************************/

#ifndef __BOOTP_H_INCLUDED__
#define __BOOTP_H_INCLUDED__

#include <ngdefs.h>

NG_BEGIN_DECLS

/* some macros for arrays in next structures */
#define BP_CHADDR_LEN   16
#define BP_HNAME_LEN    64
#define BP_FILE_LEN    128
#define BP_VEND_LEN     64
#define BP_MINPKTSZ    300   /* we want to check sizeof(struct NGbootppk) */

/* NGbootppk defines BOOTP packets format */
typedef struct {
   NGubyte  bp_op;     /* packet opcode / message type */
#define BOOTREQUEST         1
#define BOOTREPLY           2
   NGubyte  bp_htype;  /* Hardware address type */
   NGubyte  bp_hlen;   /* hardware address length */
   NGubyte  bp_hops;   /* Client sets to zero, optionally used in cross-gatew. booting */
   NGuint   bp_xid;    /* transaction ID */
   NGushort bp_secs;   /* Client data, seconds elapsed since 1s boot request */
   NGushort bp_flags;  /* RFC1532 broadcast flag.  This field is set to null */
#define BP_FBROADCAST    (ngHTONS((NGushort)1<<15))
   /* here as the stack doesn't need IP broadcast to get packet. IP unicast  */
   /* is ok with the stack, even if its own IP addr is not known (chaddr suffices) */
   NGuint   bp_ciaddr; /* Client data, client IP addr */
   NGuint   bp_yiaddr; /* Server data, your IP addr */
   NGuint   bp_siaddr; /* Server data, server IP addr */
   NGuint   bp_giaddr; /* Server data, gateway IP addr */
   NGubyte  bp_chaddr[BP_CHADDR_LEN]; /* Client data, client hardware addr */
   NGubyte  bp_sname[BP_HNAME_LEN]; /* optional, server hostname (null terminated) */
   NGubyte  bp_file[BP_FILE_LEN]; /* optional, boot file name (null terminated) */
   NGubyte  bp_vend[BP_VEND_LEN]; /* optional, vendor specific area */
} NGbootppk;

/*
 * The following macro is used to prevent bootp client to unicast a
 * bootp query to a server if its outgoing interface is not
 * initialized (missing IP address). Once ngip will be able to do that
 * it is safe to undef it.
 */
#define BOOTPC_PREVENT_UNICAST

NG_END_DECLS

#endif /* __BOOTP_H_INCLUDED__ */

/*****************************************************************************
 * $Id: net_dhcp.h,v 1.2 2000/10/26 10:46:11 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT
 * Dynamic Host Configuration Protocol generic support
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
 * DHCP module private header file
 *----------------------------------------------------------------------------
 * 01/07/1999 - Adrien Felon
 * 12/04/2000 - client code moved to net_dc.h 
 * 26/10/2000 -
 *  - macro DHCP_PREVENT_UNICAST added (moved from net_dc.h)
 *****************************************************************************/

#ifndef __NET_DHCP_H_INCLUDED__
#define __NET_DHCP_H_INCLUDED__

#include <ngnet.h>
#include <ngboot/dhcp.h>

NG_BEGIN_DECLS

/* Functions dealing with core memory (low level) */

/*   Write DHCP options in core memory */
int ngDhcpWriteCore(NGubyte *core, int *len, NGdhcpoption *dop);

/*   Read DHCP options from core memory */
int ngDhcpReadCore(
  NGubyte      *core,      /* Core memory to read option from */
  int           coreLen,   /* Length of core */
  NGdhcpoption *dop,       /* DHCP option */
  NGubyte      *optOver,   /* output: special option overload value */
  NGdhcpoption *udop,      /* User requested DHCP options */
  NGuint       *udopmask   /* Mask for satus of user requested DHCP options */
);

/* Functions to format and parse whole DHCP packets (higher level) */

/* First a litle structure to hold send parameters */
typedef struct {
  NGsockaddr   *to;        /* destination socket address */
  NGbuf        *bufp;      /* buffer to send */
  NGifnet      *netp;      /* network interface */
  NGdhcpoption *dop;       /* DHCP options */
  NGuint        xid;       /* transaction id */
  NGubyte       proto;     /* protocol */
#define SEND_PROTO_DHCP  0
#define SEND_PROTO_BOOTP 1
  NGubyte       op;        /* BOOTREQUEST or BOOTREPLY */
} NGdhcpsendparam;

/*   Make & send a DHCP packet according given parameters */
int ngDhcpSendPacket( NGdhcpsendparam * param);

/*   Parse a DHCP packet and store options we're interested in */
int ngDhcpParsePacket(
  NGbuf         *bufp,     /* Packet to parse */
  NGdhcpoption  *dop,      /* Usual DHCP packet option */
  NGdhcpoption  *udop,     /* User requested DHCP options */
  NGuint        *udopmask  /* Mask for satus of user requested DHCP options */
);

/* Debugging time functions */
#ifdef NG_DEBUG
void ngDhcpDebugPacket( NGbuf *bufp, int submod);
#endif

/*
 * The following macro is used to prevent dhcp client to unicast dhcp
 * packet to a server if its outgoing interface is not initialized
 * (missing IP address). Once ngip will be able to do that it is safe
 * to undef it.
 */
#define DHCP_PREVENT_UNICAST

NG_END_DECLS

#endif /* __NET_DHCP_H_INCLUDED__ */

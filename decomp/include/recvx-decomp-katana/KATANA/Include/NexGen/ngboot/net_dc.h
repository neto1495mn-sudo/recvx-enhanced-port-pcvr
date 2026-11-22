/*****************************************************************************
 * $Id: net_dc.h,v 1.3 2000/10/26 10:46:11 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT v1.2
 * Dynamic Host Configuration Protocol client
 * DHCP client module private header file
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
 * 12/04/2000 - Adrien Felon - Initial import
 * 06/10/2000 -
 *  - defining macro DHCP_PREVENT_UNICAST
 * 26/10/2000 -
 *  - macro DHCP_PREVENT_UNICAST moved to net_dhcp.h
 *****************************************************************************/

#ifndef __NET_DC_H_INCLUDED__
#define __NET_DC_H_INCLUDED__

#include <ngos/netbuf.h>
#include <ngsocket.h>

#include <ngdhcpc.h>

NG_BEGIN_DECLS
                              
/* DHCP client timers values */
/*   General retransmit timer  (approx values taken from rfc1541 set 4.1) */
#define DHCPC_TIMER_RMIT       NG_TIMERHZ*2  /* 2 second timer (must be >1s) */
#define DHCPC_TIMER_MAX        NG_TIMERHZ*64 /* max timer value is 64 sec */
/*   SELECTING state timer (default value) */
#define DHCPC_TIMER_SELECTING  NG_TIMERHZ*5  /* 5 seconds */
/*   Wait before init when DHCPNACK received from server */
#define DHCPC_TIMER_WAITINIT   NG_TIMERHZ*10 /* 10 seconds */

/* Structure for DHCP global variables */
typedef struct {
  NGuint         seed;           /* DHCP random seed */
  NGdhcpclient * clients;        /* DHCP client lists (points to 1st one) */
  NGdhcpclient * clientCur;      /* Pointer to current client */
  unsigned long  timerSelecting; /* Selecting state timer */
} NGdhcpcvar;

extern NGdhcpcvar ngDhcpcVar;

/* Internal function declarations */

/* Usual NexGen protocol functions */
void dhcpcInit( void);
int  dhcpcCntl( int cmd, int opt, void *arg);
void dhcpcTimer( void);
void dhcpcInput( NGbuf *bufp, NGsockaddr *from);

/* DHCP client state machine */
int ngDhcpcStateMachine( NGdhcpclient *dcp, NGbuf *bufp);

/* Some useful tool functions */
/*   Find matching DHCP client of netp interface */
NGdhcpclient * ngDhcpcFindClient( NGifnet *netp);

/* Debugging time function */
#ifdef NG_DEBUG
void ngDhcpcDebugClient( NGdhcpclient *dcp, int submod);
#endif

NG_END_DECLS

#endif /* __NET_DC_H_INCLUDED__ */

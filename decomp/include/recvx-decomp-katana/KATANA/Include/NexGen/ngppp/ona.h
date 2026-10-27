/*****************************************************************************
 * $Id: ona.h,v 1.3 2000/08/31 18:11:43 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Option Negotiation automaton definitions
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
 * 20/12/98 - Thierry Chantry
 * 20/12/99 - Adrien Felon
 *    Support for Echo Request / Echo Reply added
 *****************************************************************************/

#ifndef __NG_PPP_ONA_H_INCLUDED__
#define __NG_PPP_ONA_H_INCLUDED__

NG_BEGIN_DECLS

#define ONA_HDRLEN 4

typedef struct {
   NGubyte   onah_code;
   NGubyte   onah_id;
   NGushort  onah_length;
   NGubyte   onah_ci;
}NGonahdr;


/*
 *  CPs protocols (LCP, IPCP, etc.) negociation codes (RFC 1661).
 */
#define ONA_CONFREQ		1  /* Configuration Request */
#define ONA_CONFACK		2	 /* Configuration Ack */
#define ONA_CONFNAK		3	 /* Configuration Nak */
#define ONA_CONFREJ		4	 /* Configuration Reject */
#define ONA_TERMREQ		5	 /* Termination Request */
#define ONA_TERMACK		6	 /* Termination Ack */
#define ONA_CODEREJ		7	 /* Code Reject */
#define ONA_PROTREJ		8	 /* Protocol Reject */
#define ONA_ECHOREQ		9	 /* Echo Request */
#define ONA_ECHOREP		10 /* Echo Reply */
#define ONA_DISCREQ   11 /* Discard Request */

/*
 * Finite state machine  'states'.
 */
#define ONA_INITIAL      0  /* Down, hasn't been opened */
#define ONA_STARTING     1  /* Down, been opened */
#define ONA_CLOSED       2  /* Up, hasn't been opened */
#define ONA_STOPPED      3  /* Open, waiting for down event */
#define ONA_CLOSING      4  /* Terminating the connection, not open */
#define ONA_STOPPING     5  /* Terminating, but open */
#define ONA_REQSENT      6  /* We've sent a Config Request */
#define ONA_ACKRCVD      7  /* We've received a Config Ack */
#define ONA_ACKSENT      8  /* We've sent a Config Ack */
#define ONA_OPENED       9  /* Connection available */

/*
 * Flags - indicate options controlling ONA operation
 */
#define NG_ONAF_PASSIVE  0x0001   /* Don't die if we don't get a response */
#define NG_ONAF_RESTART  0x0002   /* Treat 2nd OPEN as DOWN, UP */
#define NG_ONAF_SILENT   0x0004   /* Wait for peer to speak first */
#define NG_ONAF_TIMER    0x0008   /* Activate protocol timer */ 

/*
 * Prototypes
 */
void ngOnaInit (struct NGifnet_S *netp);
void ngOnaUp (struct NGifnet_S *netp);
void ngOnaDown (struct NGifnet_S *netp);
void ngOnaOpen (struct NGifnet_S *netp);
void ngOnaClose (struct NGifnet_S *netp);
void ngOnaInput (struct NGifnet_S *netp, NGbuf *bufp);
void ngOnaSend (struct NGifnet_S *netp, NGbuf *bufp, int, int);
void ngOnaTimer (struct NGifnet_S *netp);

NG_END_DECLS

#endif

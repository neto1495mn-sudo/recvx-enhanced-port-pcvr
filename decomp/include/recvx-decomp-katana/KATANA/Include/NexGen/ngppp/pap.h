/*****************************************************************************
 * $Id: pap.h,v 1.3 2000/08/31 18:11:43 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Password Authentication Protocol definitions
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
 * 03/06/98 - Thierry Chantry
 *****************************************************************************/

#ifndef __NG_PPP_PAP_H_INCLUDED__
#define __NG_PPP_PAP_H_INCLUDED__

NG_BEGIN_DECLS

#define PAP_HDRLEN 4

#define AUTH_REQ 1
#define AUTH_ACK 2
#define AUTH_NAK 3

/*
 * Options.
 */
typedef struct {
  NGubyte  ack_code;
  NGubyte  ack_id;
  NGshort  ack_length;
  NGubyte  ack_data;
} NGauthack;

typedef struct {
  NGubyte   req_code;
  NGubyte   req_id;
  NGushort  req_length;
  NGubyte   req_data;
} NGauthreq;

/* PAP packet union */
typedef union {
  NGubyte   pap_code;
  NGauthreq pap_req;  /* PAP authentication request */
  NGauthack pap_ack;  /* PAP ack/nak response packet */
} NGpappk;

/* PAP Authentication protocol functions */
void ngPapTimer( NGifnet *netp);
void ngPapInput( NGifnet *netp, NGbuf *bufp);
void ngPapInit( NGifnet *netp);

NG_END_DECLS

#endif

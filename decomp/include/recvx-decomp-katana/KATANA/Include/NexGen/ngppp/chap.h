/*****************************************************************************
 * $Id: chap.h,v 1.4 2000/08/31 18:11:42 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Challenge Handshake Authentication Protocol header
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
 * 03/06/1998 - Thierry Chantry
 * 18/08/2000 - Adrien Felon
 *  definition of CHAP_ALGO_MD5 added
 *****************************************************************************/

#ifndef __NG_PPP_CHAP_H_INCLUDED__
#define __NG_PPP_CHAP_H_INCLUDED__

NG_BEGIN_DECLS

#define CHAP_ALGO_MD5 0x05 /* using CHAP with MD5 algorithm */

#define CHAP_MD5_SIGNATURE_SIZE 16

#define CHAP_RC_HDRLEN 5
#define CHAP_FS_HDRLEN 4

#define CHAP_CHALLENGE 1
#define CHAP_RESPONSE 2
#define CHAP_SUCCESS 3
#define CHAP_FAILURE 4

/* Response or challenge packet definition */
typedef struct {
  NGubyte   rc_code;
  NGubyte   rc_id;
  NGushort  rc_length;
  NGubyte   rc_valsize;
  NGubyte   rc_valoff;
} NGchaprc;

/* Success or Failure packet definition */
typedef struct {
  NGubyte   fs_code;
  NGubyte   fs_id;
  NGushort  fs_length;
  NGubyte   fs_msgoff;
} NGchapfs;

/* Chap packet union */
typedef union {
  NGubyte  ch_code;
  NGchaprc ch_rc;  /* Chap Response or challenge packet */
  NGchapfs ch_fs;  /* Chap Success or failuer packet */
} NGchappk;


/* Chap functions */
void ngChapTimer( NGifnet *netp);
void ngChapInput( NGbuf *bufp);
void ngChapInit( NGifnet *netp);

NG_END_DECLS

#endif


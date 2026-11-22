/*****************************************************************************
 * $Id: lcp.h,v 1.3 2000/08/31 18:11:43 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Link Control Protocol definitions
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
 * 20/12/1998 - Thierry Chantry
 * 02/02/2000 - Adrien Felon
 *  Alignement corrected in struct NGlcpci (ci_data is now of type NGubyte)
 * 11/05/2000 -
 *  CILEN_MAGIC definition changed from 6 to 4 (bytes)
 *****************************************************************************/

#ifndef __NG_PPP_LCP_H_INCLUDED__
#define __NG_PPP_LCP_H_INCLUDED__

NG_BEGIN_DECLS

#define LCP_HDRLEN 4

/*
 * Options.
 */
#define CI_MRU            1 /* Maximum Receive Unit */
#define CI_ASYNC          2 /* Async Control Character Map */
#define CI_AUTH           3 /* Authentication Type */
#define CI_QUALITY        4 /* Quality Protocol */
#define CI_MAGIC          5 /* Magic Number */
#define CI_PCOMP          7 /* Protocol Field Compression */
#define CI_ACOMP          8 /* Address/Control Field Compression */
#define CI_CALLBACK      13 /* callback */

#define CI_HDRLEN 2
#define CILEN_MRU 2
#define CILEN_AUTH 2 /* headear + ... */
#define CILEN_ASYNC 4
#define CILEN_QUALITY 2 /* header + ... */
#define CILEN_MAGIC 4
#define CILEN_PCOMP 0
#define CILEN_ACOMP 0

#define LCP_PROTREJ 8
#define LCP_ECHOREQ 9
#define LCP_ECHOREP 10
#define LCP_DISCREQ 11

 /* LCP configuration option */
typedef struct {
   NGubyte  ci_code;
   NGubyte  ci_length;
   NGubyte  ci_data;
}NGlcpci;

 /* LCP header packet */
typedef struct {
   NGubyte   lcp_code;
   NGubyte   lcp_id;
   NGushort  lcp_length;
   NGubyte   lcp_ci;
}NGlcphdr;

 /* To skip to the next option */
#define LCP_NEXTOPT( o, l) {\
   l += o->ci_length;\
   o = (NGlcpci*) (((NGubyte*) o) + o->ci_length);\
}


/* LCP function */
void ngLcpInit( NGifnet *netp);
void ngLcpRestore( NGifnet *netp);

 /* LCP option negociation automaton call-back functions */
extern NGonapfunc lcp_pfuncs;

NG_END_DECLS

#endif

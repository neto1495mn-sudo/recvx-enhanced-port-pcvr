/*****************************************************************************
 * $Id: ipcp.h,v 1.4 2000/08/31 18:11:43 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Internet Protocol Control Protocol definitions
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
 * 29/11/1999 - Regis Feneon
 *  - corrected NGipcpi structure definition that caused alignement problems
 * 13/07/2000 - Adrien Felon
 *  - adding Van Jacobson compression support
 * 18/08/2000 -
 *  - nothing new but reorganization of CI definition
 *****************************************************************************/

#ifndef __NG_PPP_IPCP_H_INCLUDED__
#define __NG_PPP_IPCP_H_INCLUDED__

NG_BEGIN_DECLS

#define IPCP_HDRLEN 4 /* IPCP packet header length */


#define IPCP_CI_ADDRS       1 /* IP Addresses */
#define IPCP_CI_IPCOMP      2 /* Compression Type */
#define IPCP_CI_ADDR        3 /* IP address */
#define IPCP_CI_MS_DNS1	  129 /* Primary DNS value */
#define IPCP_CI_MS_WINS1  130 /* Primary WINS value */
#define IPCP_CI_MS_DNS2   131 /* Secondary DNS value */
#define IPCP_CI_MS_WINS2  132 /* Secondary WINS value */

#define IPCP_CI_HDRLEN 2
#define IPCP_CILEN_ADDRS 2 /* Obsolete CI */
#define IPCP_CILEN_IPCOMP 2 /* + ... (not counting any data here) */
#define IPCP_CILEN_VJCOMP IPCP_CILEN_IPCOMP+2 /* 2 bytes of data with VJcomp */
#define IPCP_CILEN_ADDR 4

#define IPCP_MAX_STATES 16		/* from slcompress.h */

#define IPCP_VJMODE_OLD     1 /* "old" mode (option # = 0x0037) */
#define IPCP_VJMODE_RFC1172 2 /* "old-rfc"mode (option # = 0x002d) */
#define IPCP_VJMODE_RFC1332 3 /* "new-rfc"mode (option # = 0x002d, */

#define IPCP_VJ_COMP     0x002d /* current value for VJ compression option*/
#define IPCP_VJ_COMP_OLD 0x0037	/* "old" (i.e, broken) value for VJ */

 /* IPCP configuration option */
typedef struct {
   NGubyte  ci_code;
   NGubyte  ci_length;
   NGubyte  ci_data[1];     /* 29/11/99 */
}NGipcpci;

/* IPCP Header packet */
typedef struct {
   NGubyte   ipcp_code;
   NGubyte   ipcp_id;
   NGushort  ipcp_length;
   NGubyte   ipcp_ci;
}NGipcphdr;


 /* To skip to the next option */
#define IPCP_NEXTOPT( o, l) {\
   l += o->ci_length;\
   o = (NGipcpci*) (((NGubyte*) o) + o->ci_length);\
}

 /* IPCP functions */
void ngIpcpInit( NGifnet *netp);
void ngIpcpRestore( NGifnet *netp);

 /* IPCP option negociation automaton call-back functions */
extern NGonapfunc ipcp_pfuncs;

NG_END_DECLS

#endif


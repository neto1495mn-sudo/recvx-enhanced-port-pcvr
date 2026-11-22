/*****************************************************************************
 * $Id: pppoe.h,v 1.6 2000/12/13 20:43:19 dg Exp $
 *----------------------------------------------------------------------------
 * NexGenPPPoE v1.2
 * PPP Over Ethernet private header
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
 * 31/08/2000 - David Girault
 *****************************************************************************/

#ifndef __NG_PPPOEPRIV_H_INCLUDED__
#define __NG_PPPOEPRIV_H_INCLUDED__

NG_BEGIN_DECLS

#define NG_PPPOE_MTU 1492

/*
 * Global vairables of PPPoE protocol
 */
extern NGuint ngPppoe_RandomSeed;
extern NGpppoeifnet *ngPppoe_Ifhead;


/*
 * PPPoE Header structure
 */
typedef struct NGpppoehdr_S {
#ifdef NG_LITTLE_ENDIAN_BITFIELD
  NGubyte     pppoe_ver:4,    /* version */
              pppoe_typ:4;    /* type */
#elif defined(NG_BIG_ENDIAN_BITFIELD)
  NGubyte     pppoe_typ:4,    /* type */
              pppoe_ver:4;    /* version */
#else
#error "NG_XXX_ENDIAN_BITFIELD not defined"
#endif
#define PPPOE_VERSION          0x01   /* current PPPoE version */
#define PPPOE_TYPE             0x01   /* current PPPoE type */
  NGubyte pppoe_code;
#define PPPOE_CODE_NULL        0x00
#define PPPOE_CODE_PADI        0x09
#define PPPOE_CODE_PADO        0x07
#define PPPOE_CODE_PADR        0x19
#define PPPOE_CODE_PADS        0x65
#define PPPOE_CODE_PADT        0xa7
  NGushort pppoe_session;
  NGushort pppoe_length;
} NGpppoehdr;

NG_END_DECLS

#endif /* __NG_PPPOEPRIV_H_INCLUDED__ */



/*****************************************************************************
 * $Id: tags.h,v 1.2 2000/10/30 11:27:45 dg Exp $
 *----------------------------------------------------------------------------
 * NexGenPPPoE v1.2
 * PPP Over Ethernet private header for parsed tags
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
 * 05/09/2000 - David Girault
 *****************************************************************************/

#ifndef __NG_PPPOETAGS_H_INCLUDED__
#define __NG_PPPOETAGS_H_INCLUDED__

NG_BEGIN_DECLS

typedef struct NGpppoetag_S {
  NGuuint16 poetag_type;       /* Read using  ngURDBE16() macro */
#define PPPOE_TAG_EOL          0x0000
#define PPPOE_TAG_SERVNAME     0x0101
#define PPPOE_TAG_ACNAME       0x0102
#ifdef NG_PPPOE_HOSTUNIQ_SUPPORTED
#define PPPOE_TAG_HOSTUNIQ     0x0103
#endif
#define PPPOE_TAG_ACCOOKIE     0x0104
#define PPPOE_TAG_VENDOR       0x0105
#define PPPOE_TAG_RELAYID      0x0110
#define PPPOE_TAG_SERVNAME_ERR 0x0201
#define PPPOE_TAG_ACSYSTEM_ERR 0x0202
#define PPPOE_TAG_GENERIC_ERR  0x0203
  NGuuint16 poetag_length;     /* Read using  ngURDBE16() macro */
} NGpppoetags;

typedef struct NGpppoeparsetag_S {
  NGuint      poetag_flags;
#define PPPOE_PARSETAG_SERVNAME_MASK  0x000f
#define PPPOE_PARSETAG_VENDOR_MASK    0x00f0
#define PPPOE_PARSETAG_ACNAME         0x0100
#define PPPOE_PARSETAG_ACCOOKIE       0x0200
#define PPPOE_PARSETAG_RELAYID        0x0400
#define PPPOE_PARSETAG_SERVNAME_ERR   0x2000
#define PPPOE_PARSETAG_ACSYSTEM_ERR   0x4000
#define PPPOE_PARSETAG_GENERIC_ERR    0x8000
  NGpppoetags *poetag_serv_name[4];
  NGpppoetags *poetag_vendor[4];
  NGpppoetags *poetag_ac_name;
  NGpppoetags *poetag_ac_cookie;
  NGpppoetags *poetag_relayid;
  NGpppoetags *poetag_serv_name_error;
  NGpppoetags *poetag_ac_error;
  NGpppoetags *poetag_generic_error;
#ifdef NG_PPPOE_HOSTUNIQ_SUPPORTED
#define PPPOE_PARSETAG_HOSTUNIQ       0x0800
  NGpppoetags *poetag_hostuniq;
#endif
} NGpppoeparsetags;
  
extern int pppoeParseTag(
                         NGpppoeparsetags *, 
                         NGpppoetags *, 
                         int);
extern int pppoeAddTag(
                NGpppoetags **,
                int *,
                NGushort,
                void *,
                int);

NG_END_DECLS

#endif /* __NG_PPPOETAGS_H_INCLUDED__ */

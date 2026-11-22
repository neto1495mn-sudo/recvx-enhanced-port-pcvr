/*****************************************************************************
 * $Id: ngbootpc.h,v 1.1.1.1 2000/09/19 09:50:37 af Exp $
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
 *  struct NGbootinfo;
 *  struct NGbootOption;
 *----------------------------------------------------------------------------
 * 14/01/1998 - Thierry Chantry
 * 16/05/1999 - Adrien Felon
 *   field resv1 changed to flags, added: ngBootpApplyParameters, some macros
 *****************************************************************************/

#ifndef __NGBOOTPC_H_INCLUDED__
#define __NGBOOTPC_H_INCLUDED__

#include <ngdefs.h>
#include <ngnet.h>
#include <ngboot/bootp.h>
#include <ngboot/bp_tags.h>

NG_BEGIN_DECLS

/* NGbootinfo : return BOOTP infos to calling fct (see NGbootpQuery below) */
typedef struct {
    NGuint   bi_local;                  /* Local interface address */
    NGuint   bi_server;                 /* Server IP address */
    NGuint   bi_cookie;                 /* Vendor magic cookie */
    NGuint   bi_subnetmask;             /* Subnet mask */
    NGuint   bi_gateway;                /* Default getway */
    NGuint   bi_dns1;                   /* First DNS IP address */
    NGuint   bi_dns2;                   /* second DNS IP address */
    NGubyte  bi_servname[BP_HNAME_LEN]; /* Server name */
    NGubyte  bi_hostname[BP_HNAME_LEN]; /* Host name */
    NGubyte  bi_bootname[BP_FILE_LEN];  /* Boot file name + path */
    NGushort bi_bootsize;               /* boot file size */
} NGbootinfo;

/* NGbootOption struct: to request some vendor options with ngBootpQuery */
typedef struct {
    NGubyte   bo_tag;
    NGubyte   bo_size;
    NGubyte * bo_data;
} NGbootOption;

/* Geting infos (binfp is output arg) from a BOOTP server through netp */
int ngBootpQuery( NGifnet *netp, NGbootinfo *binfp,
                 int retry, NGbootOption *list);

NG_END_DECLS

#endif /* __NGBOOTPC_H_INCLUDED__ */

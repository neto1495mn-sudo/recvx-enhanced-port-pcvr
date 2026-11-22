/*****************************************************************************
 * $Id: vjcomp.h,v 1.3 2000/10/04 10:56:53 af Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * Van Jacobson compression for IP/TCP header
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
 * 13/07/2000 - Adrien Felon
 * 4/10/2000 -
 *  - added member vj_ierrors
 *****************************************************************************/

#ifndef __NG_PPP_VJCOMP_H_INCLUDED__
#define __NG_PPP_VJCOMP_H_INCLUDED__

#include <ngip/ip.h>

NG_BEGIN_DECLS

/* Entries of Van Jacobson table */

typedef struct NGpppvjent_S {
  struct NGpppvjent_S *
            vje_next;           /* next most recently used xmit state */
  NGushort  vje_hlen;           /* header length */
  NGubyte   vje_id;             /* slot id associated with this entry */
#define MAX_HDR 128             /* max TCP + IP hdr length */
  union {
    NGubyte vje_hdr[MAX_HDR];   /* header */
    NGiphdr vje_ip;             /* to access IP header easely */
  } vje_union;
} NGpppvjent;

#define vje_hdr vje_union.vje_hdr
#define vje_ip  vje_union.vje_ip

/*
 * Note: each entry holds the state of a single compressed TCP
 * flow. All required infos are stored here, including a copy of last
 * seen IP and TCP headers.
 */

/* Structure holding VJ comp info in each NGpppifdata (see pif_ipcp_vjdata) */

typedef struct {
  NGubyte         vj_last_recv; /* last rcvd conn. id (for cid compression) */
  NGubyte         vj_last_xmit; /* last xmit conn. id (for cid compression) */
  NGushort        vj_flags;     /* */
  NGpppvjent *    vj_last_vje;  /* most recently used xmit entry state */
  NGpppvjent *    vj_table;     /* xmit and then received conn states */
  NGubyte         vj_tablemax;  /* size of prev table */
  NGubyte         vj_pmaxslot;  /* peer max num of slot (negotiated) */
  NGubyte         vj_pcompslot; /* 1 if doing cid comp (negotiated) */
  NGubyte         vj_pad;       /* padding byte */
  NGuint          vj_ierrors;   /* last err count on net interface */
} NGvjcompdata;
#define NG_IPCP_VJCOMP_MAXSLOT 16  /* default value of negociated maxslot */

/*
 * Notes:
 * - first half of vj_table holds entries that matches outgoing TCP
 *   packets, and second half holds entries for received packets.
 * - vj_tablemax holds the max size of the table (application defined)
 *   which is not the same as the negotiated maxslot value.
 */


/* API calls (used by PPP input/output functions) */

void     ngPppVJReset( NGifnet * netp);
void     ngPppVJInit( NGifnet * netp);

NGushort ngPppVJCompressTcp(
  NGbuf *        bufp,          /* buffer to compress */
  NGvjcompdata * comp           /* compression data of PPP interface */
);

NGbuf * ngPppVJUncompressTcp(
  NGbuf *        bufp,          /* buffer to decompress */
  NGvjcompdata * comp,          /* compression data of PPP interface */
  NGushort       type           /* type of packet from proto field */
);

NG_END_DECLS

#endif /* __NG_VJCOMP_H_INCLUDED__ */
















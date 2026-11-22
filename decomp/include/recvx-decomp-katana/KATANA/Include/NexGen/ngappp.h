/*****************************************************************************
 * $Id: ngappp.h,v 1.3 2000/09/11 15:05:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * PPP AHDLC driver header
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
 * 31/03/2000 - Adrien Felon
 *  ngPppLinkTerminate() moved to ppp_link.c (private static function)
 * 17/07/2000 - Regis Feneon
 *  renamed ngappp.h
 *  new serial driver interface
 *  ngPppLinkProgress moved to ngppp.h
 *****************************************************************************/

#ifndef __NG_NGAPPP_H_INCLUDED__
#define __NG_NGAPPP_H_INCLUDED__

#include <ngppp.h>
#include <ngdev.h>

NG_BEGIN_DECLS

/*
 * PPP AHDLC Interface
 */
extern const NGnetdrv ngNetDrv_AHDLC;

#define NG_APPPIFO_DEVICE       0x300 /* Pointer to device structure */
#define NG_APPPIFO_MODEM        0x306 /* Use modem control lines */

/* compatibility with previous versions */
#define NG_PPPIFO_DEVICE    NG_APPPIFO_DEVICE
#define NG_PPPIFO_MODEM     NG_APPPIFO_MODEM

#define NG_APPPIF_DATA( netp, member) (((NGapppifnet *)(netp))->member)

typedef struct {
    NGpppifnet  apif_net;       /* generic ppp interface */
    NGdev      *apif_devp;      /* attached device */
    NGdevcb    *apif_dcb;       /* device control block */
    int         apif_options;   /* driver options */
#define NG_APIFO_MODEM      0x0010  /* Use DRT/DSR to detect close connection */
    int         apif_cflags;    /* last control flags */
/* input */
    int         apif_iflags;    /* input/output flags */
#define NG_APIFOF_ESCAPED   0x0001  /* output escaped data in progress */
#define NG_APIFIF_ESCAPED   0x0002  /* input escaped data in progress */
    NGbuf      *apif_ibuf;      /* input buf */
    NGushort    apif_icrc;      /* input checksum */
/* output */
    NGushort    apif_ocrc;      /* ouput checksum */
    int         apif_oflags;    /* output flags */
    NGbuf      *apif_obuf;      /* output buffer */
} NGapppifnet;

/* serial device upcall routine */
void ngApppDevioUpcall( NGdevcb *dcb, void *data, int flags);

/*
 * AHDLC encoding
 */
/* header */
#define NG_AHDLC_HDRLEN         2   /* length of ahdlc header */
#define NG_AHDLC_ALLSTATIONS 0xff   /* All-Stations broadcast address */
#define NG_AHDLC_UI          0x03   /* Unnumbered Information */
/* Values for FCS calculations */
#define NG_AHDLC_FCSLEN         2   /* length of FCS */
#define NG_AHDLC_INITFCS    0xffff  /* Initial FCS value */
#define NG_AHDLC_GOODFCS    0xf0b8  /* Good final FCS value */
#define NG_AHDLC_FCS( fcs, c)   (((fcs) >> 8) ^ fcstab[((fcs) ^ (c)) & 0xff])
/* Escaping */
#define NG_AHDLC_FLAG        0x7e   /* Flag Sequence */
#define NG_AHDLC_ESCAPE      0x7d   /* Asynchronous Control Escape */
#define NG_AHDLC_TRANS       0x20   /* Asynchronous transparency modifier */

NG_END_DECLS

#endif


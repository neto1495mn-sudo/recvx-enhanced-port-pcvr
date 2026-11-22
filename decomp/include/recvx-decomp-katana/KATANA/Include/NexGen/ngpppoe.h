/*****************************************************************************
 * $Id: ngpppoe.h,v 1.7 2000/10/30 11:27:44 dg Exp $
 *----------------------------------------------------------------------------
 * NexGenPPPoE v1.2
 * PPP Over Ethernet driver header
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
 * 17/08/2000 - David Girault
 *****************************************************************************/

#ifndef __NG_PPPOE_H_INCLUDED__
#define __NG_PPPOE_H_INCLUDED__

#include <ngip.h>
#include <ngppp.h>
#include <ngdev.h>
#include <ngeth.h>

NG_BEGIN_DECLS

#define NG_PPPOE_HOSTUNIQ_SUPPORTED

/*
 * PPPoE Protocol
 */
extern const NGproto ngProto_PPPOE;

/* Protocol Options */
#define NG_PPPOEO_ADDIF        0x360  /* Add PPPoE IF to the IF list */
#define NG_PPPOEO_TESTIF       0x361  /* Check if IF is in the PPPoE IF list */
#define NG_PPPOEO_RANDOMSEED   0x362

/*
 * PPPoE Interface
 */
extern const NGnetdrv ngNetDrv_PPPOE;

#define NG_POEIF_MAXCHAR 32

/* Interfaces Options */
#define NG_PPPOEIFO_ETHIF      0x350 /* Pointer to ethernet interface */
/*#define NG_PPPOEIFO_MTU        0x351 /* MTU value  */
#define NG_PPPOEIFO_MAXERR     0x352 /* poeif_maxerror value  */
#define NG_PPPOEIFO_TIMEOUT    0x353 /* poeif_timeout value  */
#define NG_PPPOEIFO_ACNAME     0x354 /* poeif_acname value  */
#define NG_PPPOEIFO_SERVNAME   0x355 /* poeif_servname value  */
#ifdef NG_PPPOE_HOSTUNIQ_SUPPORTED
#define NG_PPPOEIFO_HOSTUNIQ   0x356 /* poeif_hostuniq flag  */
#endif

#define NG_PPPOEIF_DATA( netp, member) (((NGpppoeifnet *)(netp))->member)

typedef struct NGpppoeifnet_S {
  NGpppifnet    poeif_net;       /* generic ppp interface */
  NGethifnet   *poeif_ethif;     /* attached ethernet interface */
  struct NGpppoeifnet_S *poeif_next;      /* next pppoe interface */
  int           poeif_flags;     /* driver options */
#ifdef NG_PPPOE_HOSTUNIQ_SUPPORTED
#define NG_POEIFF_HOSTUNIQ    1
#endif
  int           poeif_state;     /* state of the interface */
  /*  int           poeif_event;     DELETED event of the interface */
#define NG_POEIFS_DOWN       0x0001
#define NG_POEIFS_INIT       0x0002
#define NG_POEIFS_OFFER      0x0004
#define NG_POEIFS_REQUEST    0x0008
#define NG_POEIFS_SESSION    0x0010
#define NG_POEIFS_TERM_SND   0x0020
#define NG_POEIFS_TERM_RCV   0x0040
#define NG_POEIFS_FAIL       0x8000
  /* discovery phase data */
  int           poeif_curtime;   /* current number of ticks */
  int           poeif_timeout;   /* time-out base time in ticks */
  int           poeif_maxerror;  /* maximum number of error before fail */
  int           poeif_nberror;   /* current number of error */
  char         *poeif_acnamereq;
  char         *poeif_servnamereq;
  char          poeif_acname[NG_POEIF_MAXCHAR];
  char          poeif_servname[NG_POEIF_MAXCHAR];
  char          poeif_accookie_used;
  char          poeif_accookie_len;
  char          poeif_accookie[NG_POEIF_MAXCHAR];
  char          poeif_errormsg[NG_POEIF_MAXCHAR];
#ifdef NG_PPPOE_HOSTUNIQ_SUPPORTED
  NGuint        poeif_hostid;
#endif
  /* session phase */
  NGubyte       poeif_acethaddress[6]; /* access concentrator address */
  NGushort      poeif_sessionid;       /* session number from AC */
} NGpppoeifnet;


/* private functions */
extern void ngPppoeLinkProgress( NGifnet *, int);

/* User Interface */
extern int ngPppoeCheckIf(NGifnet *);
extern int ngPppoeStart(NGifnet *);
extern int ngPppoeStop(NGifnet *);
extern int ngPppoeGetState(NGifnet *);
extern int ngPppoeWaitState(NGifnet *, int, int, u_long);

NG_END_DECLS

#endif /* __NG_PPPOE_H_INCLUDED__ */

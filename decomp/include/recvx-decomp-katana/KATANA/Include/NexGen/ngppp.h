/*****************************************************************************
 * $Id: ngppp.h,v 1.8 2000/09/12 19:05:29 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * PPP Interface & Protocol Definitions
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
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
 * 23/06/2000 - Regis Feneon
 *  ppp interface structure
 * 17/07/2000 -
 *  integration of last changes (vj compression)
 *  net_ppp.h and net_pif now merged in this file
 *  AHDLC driver specifics moved to ngappp.h
 * 18/08/2000 - Adrien Felon
 *  MS-DNS option code added
 *  code clean-up
 * 18/08/2000 -
 *  type of pif_net member changed from NGgenifnet to NGifnet
 * 11/09/2000 - Regis Feneon
 *  added DNS1_ADDR and DNS2_ADDR options
 *  added pif_init_dns1_addr and pif_init_dns2_addr members
 *****************************************************************************/

#ifndef __NG_NGPPP_H_INCLUDED__
#define __NG_NGPPP_H_INCLUDED__

#include <ngos.h>
#include <ngnet.h>
#include <ngip.h>

NG_BEGIN_DECLS

/*
 * PPP Protocol
 */

extern NGproto      ngProto_PPP;

#define NG_PPP_MAJOR_VERSION 1
#define NG_PPP_MINOR_VERSION 0

#define NG_PPP_PPPCOMP_SUPPORTED
#define NG_PPP_CHAP_SUPPORTED
#define NG_PPP_PAP_SUPPORTED
#define NG_PPP_VJCOMP_SUPPORTED
#define NG_PPP_MS_DNS_SUPPORTED
/* #define NG_PPP_MAGIC_SUPPORTED */

#ifdef NG_PPP_VJCOMP_SUPPORTED
#include <ngppp/vjcomp.h>
#endif

/* Link fonction */
void ngPppLinkProgress( NGifnet *netp, int event);

/*
 * PPP Network Interface
 */

/* PPP interface options */
#define NG_PPPIFO_DEFAULT_ROUTE 0x301 /* Iternf. def route after connection */
#define NG_PPPIFO_PASSIVE       0x302 /* Wait peer answer instead of stoping */
#define NG_PPPIFO_SILENT        0x303 /* Wait peer initiative */
#define NG_PPPIFO_RESTART       0x304 /* Interface can't die */
#define NG_PPPIFO_TERM_MAX      0x307 /* Max num of terminate request */
#define NG_PPPIFO_CONFIG_MAX    0x308 /* Max num of config request */
#define NG_PPPIFO_FAIL_MAX      0x309 /* Max num of config Nack */
#define NG_PPPIFO_RESTART_MAX   0x310 /* Retrans. timouet */

#define NG_PPPIFO_LCP_ACOMP     0x320 /* Address&ctrl fields compression */
#define NG_PPPIFO_LCP_PCOMP     0x321 /* Protocol fields compression */
#define NG_PPPIFO_LCP_MAGIC     0x322 /* Magic num negociation */
#define NG_PPPIFO_LCP_MRU       0x323 /* Set maximum receive unit */
#define NG_PPPIFO_LCP_ASYNC     0x324 /* Negociate async map table */
#define NG_PPPIFO_LCP_AUTH      0x326 /* Set when peer auth. requested */
#define NG_PPPIFO_LCP_PAP       0x327 /* PAP may be activate (as req by peer)*/
#define NG_PPPIFO_LCP_CHAP      0x328 /* CHAP may be activate (idem) */

#define NG_PPPIFO_CHAP_PERIOD   0x329 /* CHAP challenge periode (in sec) */

#define NG_PPPIFO_IPCP_ADDR     0x330 /* Negociation IP addresses */
#define NG_PPPIFO_IPCP_ACCEPT_LOCAL  0x331 /* Accept peer's addr */
#define NG_PPPIFO_IPCP_ACCEPT_REMOTE 0x332 /* Accept peer's dst. addr */
#define NG_PPPIFO_IPCP_VJCOMP   0x333 /* Negociat VJ IP header compression */
#define NG_PPPIFO_IPCP_VJTABLE  0x334 /* NGpppvjent table for compression */
#define NG_PPPIFO_IPCP_VJMAX    0x335 /* Num of element in prev table */
#define NG_PPPIFO_IPCP_DNS1     0x336 /* Negociate Primary DNS */
#define NG_PPPIFO_IPCP_DNS2     0x337 /* Negociate Secundary DNS */

#define NG_PPPIFO_AUTH_USER     0x338 /* User and password */
#define NG_PPPIFO_AUTH_SECRET   0x339 /* for a CHAP/PAP authentication */
#define NG_PPPIFO_AUTH_HOST     0x340 /* for a CHAP/PAP authentication */

#define NG_PPPIFO_CB_AUTH       0x341 /* Authentication callback */
#define NG_PPPIFO_CB_CONNECT    0x342 /* Connection callback */
#define NG_PPPIFO_CB_DISCONNECT 0x343 /* Disconnection callback */

#define NG_PPPIFO_IPCP_DNS1_ADDR    0x344   /* set/get primary dns address */
#define NG_PPPIFO_IPCP_DNS2_ADDR    0x345   /* set/get secondary dns address */

/* Authentication parameter */

typedef struct {
  int       auth_protocol;      /* Authentication protocol */
#define NG_PPP_PAP 0
#define NG_PPP_CHAP 1
  char *    auth_user;          /* user name */
  int       auth_userlen;       /* user length */
  char *    auth_secret;        /* password/secret */
  int       auth_secretlen;     /* secret length */
} NGpppauth;

/* PPP connection states */
#define NG_PIFS_DEAD       0x0001
#define NG_PIFS_ESTABLISH  0x0002
#define NG_PIFS_NETWORK    0x0004
#define NG_PIFS_AUTH       0x0008
#define NG_PIFS_TERMINATE  0x0010
#define NG_PIFS_IFUP       0x0100

/* PPP user interface functions */
int ngPppStart(NGifnet *netp);
int ngPppStop(NGifnet *netp);
int ngPppGetState(NGifnet *netp);
int ngPppWaitState(
  NGifnet *netp,
  int      statewait,
  int      stateexit,
  u_long   timeoutms
);

#define NG_PPP_PROTOMAX     4

#define NG_PPPIF_DATA( netp, member) (((NGpppifnet *)(netp))->member)

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* Option Negotiation Autotomaton callbacks */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
typedef struct onapfunc_S {
  /* Called when we don't want the lower layer */
  void (*stop_f)    (struct NGifnet_S *netp);
  /* Called when ona reaches OPENED state */
  void (*up_f)      (struct NGifnet_S *netp);
  /* Called when ona leaves OPENED state */
  void (*down_f)    (struct NGifnet_S *netp);
  /* Reset our Configuration Information */
  void (*resetci_f) (struct NGifnet_S *netp);
  /* Request peer's Configuration Information */
  int  (*reqci_f)   (struct NGifnet_S *netp, NGbuf *bufp);
  /* ACK our Configuration Information */
  int  (*ackci_f)   (struct NGifnet_S *netp, NGbuf *bufp);
  /* NAK our Configuration Information */
  int  (*nakci_f)   (struct NGifnet_S *netp, NGbuf *bufp);
  /* Reject our Configuration Information */
  int  (*rejci_f)   (struct NGifnet_S *netp, NGbuf *bufp);
  /* Add our Configuration Information */
  void (*addci_f)   (struct NGifnet_S *netp, NGbuf *bufp);
  /* Called when unknown code received */
  int  (*extcode_f) (struct NGifnet_S *netp, NGbuf *bufp);
} NGonapfunc;

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* Option Negotiation Automaton date structure  */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
typedef struct NGona_S {
  NGushort    ona_protocol;     /* Data Link Layer Protocol field value */
  int         ona_state;        /* State */
  int         ona_flags;        /* Contains option bits */
  int         ona_timeo;        /* Negociation timeout value */
  NGubyte     ona_id;           /* Current id */
  NGubyte     ona_reqid;        /* Current request id */
  NGubyte     ona_seen_ack;     /* Have received valid Ack/Nak/Rej to Req */
  NGubyte     ona_zero;    /* 29/11/99 */
  int         ona_confmax;      /* Maximum Configure-Request transmissions */
  int         ona_retransmits;  /* Number of retransmissions left */
  int         ona_termmax;      /* Maximum Terminate-Request transmissions */
  int         ona_nakloops;     /* Number of nak loops since last ack */
/* int         ona_maxnakloops;*/ /* Maximum number of nak loops tolerated */
  NGonapfunc *ona_pfunc;        /* Associated protocol functions */
} NGona;

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* LCP options structure for negociation purpose */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef struct NGlcpopt_S {
  int      lcpo_flags;
#define NG_LCPO_MRU     0x00000002 /* negociate MRU */
#define NG_LCPO_ASYNC   0x00000004 /* negociate async map */
#define NG_LCPO_AUTH    0x00000008 /* negociate authenticate */
#define NG_LCPO_QUALITY 0x00000010 /* negociate use of link quality prot. */
#define NG_LCPO_MAGIC   0x00000020 /* ask for magic number */
#define NG_LCPO_PCOMP   0x00000080 /* HDLC protocol field compression */
#define NG_LCPO_ACOMP   0x00000100 /* HDLC Addr/Ctrl field compression */
#define NG_LCPO_PAP     0x00001000 /* ask for UPAP authentication */
#define NG_LCPO_CHAP    0x00002000 /* ask for CHAP authentication */
  NGushort lcpo_mru;                 /* Value of MRU */
  NGushort lcpo_zero;   /* 29/11/99 */
  NGuint   lcpo_accm;                /* Value of async map */
#ifdef NG_PPP_MAGIC_SUPPORTED
  NGuint   lcpo_magic;
  int      lcpo_numloops;            /* Num of loops during magic num neg. */
#endif
#ifdef NG_PPP_QUALITY_SUPPORTED
  NGuint   lcpo_lqr_period;          /* Reporting period for LQR 1/100ths sec*/
#endif
} NGlcpopt;


/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* IPCP option structure for negotiation purpose */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
typedef struct NGipcpopt_S {
  int       ipcpo_flags;
#define NG_IPCPO_ADDRS         0x0002
#define NG_IPCPO_VJCOMP        0x0004
#define NG_IPCPO_ADDR          0x0008
#define NG_IPCPO_DSTADDR       0x0010
#define NG_IPCPO_MSDNS1        0x0020
#define NG_IPCPO_MSDNS2        0x0040
#define NG_IPCPO_ACCEPT_REMOTE 0x1000
#define NG_IPCPO_ACCEPT_LOCAL  0x2000
#define NG_IPCPO_DEFAULT_ROUTE 0x4000
} NGipcpopt;

/* =-=-=-=-=-=-=-=-=-=-=-=-=-= */
/*     CHAP authentication     */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-= */
#define NG_MAX_CHALLENGE 20           /* CHAP challenge max size */

/* Challenge data stucture */
typedef struct {
  NGubyte chal_len;
  NGubyte chal_id;
  NGubyte chal_md[16];
  NGubyte chal_zero[2]; /* 29/11/99 */
} NGchal;

/* PPP Generic Interface */
typedef struct {
  NGifnet       pif_net;       /* generic interface */
  int pif_state;                 /* PPP connection states and events */
  int pif_event;                 /* Link progression event */
#define NG_PIFE_UP         0x0001  /* LCP neg. starts */
#define NG_PIFE_OPEN       0x0002  /* LCP neg. success */
#define NG_PIFE_SUCCESS    0x0004  /* Authenticatio neg. success */
#define NG_PIFE_FAIL       0x0008  /* LCP or Authentication neg. failed */
#define NG_PIFE_CLOSE      0x0010  /* the Network closes PPP connection */
#define NG_PIFE_DOWN       0x0020  /* Link terminated */
#define NG_PIFE_DISCONNECT 0x0100  /* Hardware disconnection detected */

  int pif_options;               /* Misc interface option */
#define NG_PIFO_PASSIVE    0x0001  /* Don't die if we don't get a response */
#define NG_PIFO_SILENT     0x0002  /* Wait for the other end to start first */
#define NG_PIFO_RESTART    0x0004  /* restart vs. exit after close */

  int pif_ncpstate;              /* Network control protolols state */
#define NG_PIF_NCPS_IPCP   0x0002  /* IPCP negotiation done */

/* misc parameters */
  struct NGona_S  pif_ona;       /* Finite state machine */
  int         pif_termmax;       /* Max termimate request */
  int         pif_confmax;       /* Max configuration request */
  int         pif_failmax;       /* Max failure message */
  int         pif_restartmax;    /* retransmission timeout */

/* Global authentication parameters */
  int         pif_auth_flags;    /* General authentication flags */
#define NG_PIFAF_INIT     0x0001   /* Set when auth. proto. initialized */
#define NG_PIFAF_LOCAL    0x0002   /* set when our side has been auth. */
#define NG_PIFAF_PEER     0x0004   /* set whe we have authenticate the peer */

  char       *pif_auth_user;     /* Our side CHAP/PAP authentication user */
  char       *pif_auth_secret;   /* Our side CHAP/PAP secret/password */

#ifdef NG_PPP_PAP_SUPPORTED
  NGubyte     pif_pap_reqtimeo;  /* PAP Request authentication timeo */
  NGubyte     pif_pap_reqretrans; /* PAP request retransmission */
  NGubyte     pif_pap_reqid;     /* PAP request id */
  NGubyte     pif_pap_zero;   /* 29/11/99 */
#endif

#ifdef NG_PPP_CHAP_SUPPORTED
  int         pif_chap_chtimeo;   /* Challenge timeout nox200ms */
  int         pif_chap_chretrans; /* Challenge nb restrans */
  int         pif_chap_chperiod;  /* Challenge period nox200ms */
  int         pif_chap_retimeo;   /* response timeout nox200ms */
  int         pif_chap_reretrans; /* response nb restrans */
  NGubyte     pif_chap_rehash[NG_MAX_CHALLENGE];/* CHAP hash table */
  NGubyte     pif_chap_chid;      /* Current challenge ID */
  NGubyte     pif_chap_chlen;     /* Current challenge length */
  NGubyte     pif_chap_zero[2];   /* 29/11/99 */
  NGubyte     pif_chap_chmsg[16]; /* current Challenge itself */
  NGuint      pif_chap_rand;      /* random number */
#endif

/* LCP options */
  struct NGlcpopt_S  pif_lcp_wopt; /* LCP wanted option (set at the init phase) */
  struct NGlcpopt_S  pif_lcp_opt;  /* LCP negiciated option */
  struct NGlcpopt_S  pif_lcp_peerwopt;  /* LCP peer's wanted option */
  struct NGlcpopt_S  pif_lcp_peeropt;   /* LCP peer's negociated option */

/* IPCP options */
  NGuint             pif_init_addr;     /* IPCP local address */
  NGuint             pif_init_dstaddr;  /* IPCP remote address */
  NGuint             pif_ipcp_addr;     /* IPCP local address */
  NGuint             pif_ipcp_dstaddr;  /* IPCP remote address */
#ifdef NG_PPP_MS_DNS_SUPPORTED
  NGuint             pif_init_dns1_addr; /* IP addr of primary DNS server */
  NGuint             pif_init_dns2_addr; /* IP addr of secundary DNS server */
  NGuint             pif_ipcp_dns1_addr; /* IP addr of primary DNS server */
  NGuint             pif_ipcp_dns2_addr; /* IP addr of secundary DNS server */
#endif
  struct NGipcpopt_S pif_ipcp_wopt;     /* IPCP wanted option (init param ) */
  struct NGipcpopt_S pif_ipcp_opt;      /* IPCP negociated option */
  struct NGipcpopt_S pif_ipcp_peeropt;  /* IPCP negociated option */
#ifdef NG_PPP_VJCOMP_SUPPORTED
  NGvjcompdata       pif_ipcp_vjdata;   /* Data for VJ compression */
#endif

/* User call-backs */
  void (*pif_cb_connect_f)(NGifnet *netp);  /* Connect call-back */
  void (*pif_cb_disc_f) (NGifnet *netp);    /* Disconnect call-back */
  int  (*pif_cb_auth_f)(NGifnet *netp, NGpppauth *auth); /* user authentification function */

} NGpppifnet;

/*
 * Generic ppp interface routines
 */

int ngPppifInit( NGifnet *netp);
int ngPppifOpen( NGifnet *netp);
int ngPppifClose( NGifnet *netp);
int ngPppifOutput( NGifnet *netp, NGbuf *bufp, NGuint addr);
int ngPppifCntl( NGifnet *netp, int cmd, int opt, void *arg);

NG_END_DECLS

#endif


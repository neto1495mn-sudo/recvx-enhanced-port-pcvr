/*****************************************************************************
 * $Id: ngdhcpc.h,v 1.3 2000/10/26 10:45:45 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT v1.2
 * Dynamic Host Configuration Protocol client definition
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
 * structure NGdhcpclient
 *----------------------------------------------------------------------------
 * 01/07/1999 - Adrien Felon - Initial Release
 * 13/09/1999 -
 *  adding new members in NGdhcpclient (dc_app_XXX)
 * 10/10/2000 -
 *  adding options to allow application to set client id
 * 26/10/2000 -
 *  adding flags to handle DHCPINFORM
 *****************************************************************************/

#ifndef __NGDHCPC_H_INCLUDED__
#define __NGDHCPC_H_INCLUDED__

#include <ngos.h>
#include <ngboot/dhcp.h>
#include <ngip.h>

NG_BEGIN_DECLS

/* DHCP client structure */
typedef struct NGdhcpclient_S {
  struct NGdhcpclient_S *dc_next; /* Next DHCP client */
  NGubyte        dc_state;     /* Internal DHCP client state */
#define INIT            0       /* While in initialization process */
#define SELECTING       1       /* Client is collecting DHCP server replies */
#define REQUESTING      2       /* Sends a request & wait to complete config */
#define BOUND           3       /* Up and sleeping (everything is fine!) */
#define RENEWING        4       /* Up and trying to extend current lease */
#define REBINDING       5       /* Up and trying to get (another??) lease */
#define INIT_REBOOT     6       /* When remembering IP addr, lease server... */
#define REBOOTING       7       /* Rebinding from INIT_REBOOT state */
  NGubyte        dc_mode;      /* DHCP client mode */
#define DHCPC_RUNNING     0x0   /* Running DHCP client */
#define DHCPC_SLEEPING    0x1   /* Sleeping DHCP client */
#define DHCPC_MATCHFIRST	0<<1  /* Takes first answering lease server */
#define DHCPC_MATCHLEASE  1<<1  /* Selects longest lease time */
#define DHCPC_INFORMSENT  0x4   /* DHCPINFORM sent (for client with IP addr) */
  NGifnet *      dc_netp;      /* Network interface to run DHCP on */
  NGuint         dc_xid;       /* Client transaction ID */
  unsigned int   dc_rmit;      /* Remaining time before retransmit (ticks) */
  unsigned int   dc_boff;      /* Current retransmit timer backoff (ticks) */
  NGuint         dc_saddr;     /* IP address of selected lease server */
  NGuint         dc_caddr;     /* Client IP address suggested */
  unsigned long  dc_T1;        /* Remaining time before renewing (ticks) */
  unsigned long  dc_T2;        /* Remaining time before rebinding (ticks) */
  unsigned long  dc_lease;     /* Lease time for client IP address (ticks) */
  unsigned long  dc_app_saddr; /* IP addr of server suggested by application */
  unsigned long  dc_app_T1;    /* T1 suggested by application */
  unsigned long  dc_app_T2;    /* T2 suggested by application */
  unsigned long  dc_app_lease; /* lease suggested by application */
  NGubyte *      dc_app_cid;   /* client id */
  int            dc_app_cidlen;/* length of client id */
  NGuint *       dc_reject;    /* IP addresses of indesirable lease servers */
  NGdhcpoption * dc_optlist;   /* DHCP formatted list of requested options */
  unsigned int   dc_optcount;  /* number of user requested option */
  NGuint         dc_optflags;  /* flags for incoming option values */
  void         (*dc_cbkf)(     /* User callback function */
    int                    event,    /* callback context (UP, DOWN, INFORM) */
    NGuint                 incoming, /* bit mask for incoming option */
    struct NGdhcpclient_S *client    /* calling DHCP client */
  );
} NGdhcpclient;

/* Macros for user callback function */
/*   Event */
#define UP              0     /* interface has just entered UP state */
#define DOWN            1     /* interface has just entered DOWN state */
#define INFORM          2     /* some user requested opt have just arrived */
/*   To manipulate som DHCP client fields */
#define NG_DHCPC_GET_IF(dcp)          ((dcp)->dc_netp)
#define NG_DHCPC_GET_STATE(dcp)       ((dcp)->dc_state)
#define NG_DHCPC_OPT_IS_SET(dcp,idx) (((dcp)->dc_optflags) & (1<<(idx)))

/* DHCP configurating options (argument type is commented) */
/* Setting timer for selecting state (global DHCP option) */
#define NG_DHCPCO_TIMERSELECT  0x0501    /* unsigned long */
/* User memory space to store a DHCP client */
#define NG_DHCPCO_CLIENTADD    0x0502    /* NGdhcpclient* */
/* Set current DHCP client to configure */
#define NG_DHCPCO_SET          0x0503    /* NGdhcpclient* */
/* Set network interface on which DHCP should run on */
#define NG_DHCPCO_IF           0x0504    /* NGifnet* */
/* Set DHCP client criteria to select lease */ 
#define NG_DHCPCO_MODE         0x0505    /* int */
/* IP address of lease server */
#define NG_DHCPCO_SADDR        0x0506    /* NGuint */
/* Suggested client IP address */
#define NG_DHCPCO_CADDR        0x0507    /* NGuint */
/* Default time before renewing (for a virtual lease time of 1000) */
#define NG_DHCPCO_T1           0x0508    /* int */
/* Default time before rebinding (for a virtual lease time of 1000) */
#define NG_DHCPCO_T2           0x0509    /* int */
/* Suggested lease time for IP address */
#define NG_DHCPCO_LEASE        0x0510    /* unsigned long */
/* Null terminated list of IP address of lease servers to reject */
#define NG_DHCPCO_REJ_SERVERS  0x0511    /* NGuint* */
/* Requested option list (sequence of tag/size/value, ended with TAG_END) */
#define NG_DHCPCO_OPTLIST      0x0512    /* NGdhcpoption* */
/* User callback function */
#define NG_DHCPCO_CALLBACK     0x0513    /*void(*)(int,NGUint,NGdhcpclient*) */
/* Setting DHCP random seed */
#define NG_DHCPCO_RANDOMSEED   0x0514    /* unsigned long */
/* Setting DHCP client id */
#define NG_DHCPCO_CLIENTID     0x0515    /* NGubyte* */
/* Setting DHCP client id length */
#define NG_DHCPCO_CLIENTID_LEN 0x0516    /* int */

/* User functions */
int ngDhcpcStart( NGifnet *netp);
int ngDhcpcRelease( NGifnet *netp);
int ngDhcpcSetOption( NGifnet *netp, int opt, void *optval);
int ngDhcpcSendInform( NGifnet *netp, NGuint saddr, NGdhcpoption * req_opt);

/* Protocol description structure */
extern NGproto ngProto_DHCPC;

NG_END_DECLS

#endif /* __NGDHCPC_H_INCLUDED__ */

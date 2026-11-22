/*****************************************************************************
 * $Id: netif.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Low-Level Network Interface Structures
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 21/07/2000 - Regis Feneon
 *  see ngip/if.h
 *****************************************************************************/

#ifndef __NG_NETIF_H_INCLUDED__
#define __NG_NETIF_H_INCLUDED__

#include <ngos/queue.h>
#include <ngos/netbuf.h>
#include <ngos/netprot.h>

NG_BEGIN_DECLS

#define NG_IF_OUTQ_DFLT     4       /* default interface output queue size */

/* generic interface definition */
typedef struct NGifnet_S {
    struct NGifnet_S   *if_next;    /* link */
    char               *if_name;    /* interface name (ascii) */
    int                 if_type;    /* type of interface */
#define NG_IFT_OTHER        0       /* unspecified */
#define NG_IFT_ETHER        1       /* ethernet */
#define NG_IFT_LOOP         2       /* loopback */
#define NG_IFT_PPP          3       /* ppp */
#define NG_IFT_AUTOSTART    0x8000  /* used only at init time */
    int                 if_flags;   /* flags */
#define NG_IFF_UP           0x0001  /* interface is up */
#define NG_IFF_BROADCAST    0x0002  /* broadcast address valid */
#define NG_IFF_DEBUG        0x0004  /* turn on debugging */
#define NG_IFF_LOOPBACK     0x0008  /* is a loopback net */
#define NG_IFF_POINTOPOINT  0x0010  /* interface is point-to-point link */
#define NG_IFF_EXTNAT       0x0020  /* external NAT interface */
#define NG_IFF_RUNNING      0x0040  /* resources allocated */
#define NG_IFF_NOARP        0x0080  /* no address resolution protocol */
#define NG_IFF_OACTIVE      0x0400  /* transmission in progress */
#define NG_IFF_SIMPLEX      0x0800  /* can't hear own transmissions */
#define NG_IFF_MULTICAST    0x8000  /* supports multicast */
/* flags set internally only: */
#define NG_IFF_CANTCHANGE \
    (NG_IFF_BROADCAST|NG_IFF_POINTOPOINT|NG_IFF_RUNNING|\
      NG_IFF_OACTIVE|NG_IFF_SIMPLEX|NG_IFF_MULTICAST)
/* address */
    NGuint              if_addr;            /* interface address */
    NGuint              if_dstaddr;         /* destination address (point-to-point) */
    NGuint              if_net;             /* network address */
    NGuint              if_netmask;         /* mask of the net part */
    NGuint              if_subnet;          /* subnet number (including net) */
    NGuint              if_subnetmask;      /* mask of subnet part */
    NGuint              if_netbroadcast;    /* network broadcast address */
    NGuint              if_broadaddr;       /* directed broadcast address */
/* driver */
    char               *if_drvname;         /* driver (device) name */
    int                 if_mtu;             /* maximum transmission unit */
    u_long              if_bps;             /* link speed (bit per seconds) */
    NGqueue             if_outq;            /* output queue */
/* driver functions */
    int               (*if_open_f)( struct NGifnet_S *ifnetp);
    int               (*if_close_f)( struct NGifnet_S *ifnetp);
    int               (*if_output_f)( struct NGifnet_S *ifnetp, NGbuf *bufp, NGuint dst_addr);
    void              (*if_start_f)( struct NGifnet_S *ifnetp);
    int               (*if_cntl_f)( struct NGifnet_S *ifnetp, int cmd, int option, void *arg);
/* statistics */
    u_long              if_ibytes;
    u_long              if_ierrors;
    u_long              if_imcasts;
    u_long              if_ipackets;
    u_long              if_iqdrops;
    u_long              if_noproto;
    u_long              if_obytes;
    u_long              if_oerrors;
    u_long              if_omcasts;
    u_long              if_opackets;
    u_long              if_oqdrops;
    NGnode              if_buflist;     /* for internal buffer management */
    int                 if_dev1;        /* special parameter 1 */
    int                 if_dev2;        /* special parameter 2 */
    void               *if_devptr1;     /* special ptr parameter 1 */
    void               *if_devptr2;     /* special ptr parameter 2 */
} NGifnet;


/* network driver definition */
typedef struct NGnetdrv_S {
    char       *netd_name;       /* driver name */
    int         netd_type;       /* adaptator type */
    int         netd_flags;      /* interface flags (at startup) */
    int		    netd_mtu;        /* maximum transmission unit */
	u_long      netd_bps;		 /* link speed (bits per second) */
    int       (*netd_init_f)( NGifnet *ifnetp);
    int       (*netd_open_f)( NGifnet *ifnetp);
    int       (*netd_close_f)( NGifnet *ifnetp);
    int       (*netd_output_f)( NGifnet *ifnetp, NGbuf *bufp, NGuint dst_addr);
    void      (*netd_start_f)( NGifnet *ifnetp);
    int       (*netd_cntl_f)( NGifnet *ifnetp, int cmd, int option, void *arg);
} NGnetdrv;

NG_END_DECLS

#endif


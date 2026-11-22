/*****************************************************************************
 * $Id: ngip.h,v 1.4 2000/08/29 18:46:00 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Internet Protocol Module Definitions
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
 * 07/10/98 - Regis Feneon
 * 29/12/98 -
 *  added configuration options
 *  changes in global variable names
 * 13/01/99 -
 *  standart ip address class definitions added (moved from ip.h)
 *  net_in.h removed
 * 09/02/2000 -
 *  added NG_IN_ADDRXXX() macros in network-byte order
 * 18/02/2000 - Adrien Felon
 *  ngIpOuput() has new argument "flags" for IP forwarding and NAT support
 * 14/04/2000 -
 *  defining IP options for NAT configuration
 * 19/07/2000 - Regis Feneon
 *  added inet and checksum routines prototypes
 *  added host/network byte order macros
 *  added ip and rawip protocols structure
 * 18/08/2000 -
 *  added NG_INADDR() macro
 *****************************************************************************/

#ifndef __NG_NGIP_H_INCLUDED__
#define __NG_NGIP_H_INCLUDED__

#include <ngos.h>
#include <ngnet.h>

NG_BEGIN_DECLS

/* NexGenIP library generation options */
/* Warning: the libraries need to be rebuilt when options are changed */
#define NG_IPFRAG_SUPPORTED
/* #define NG_IPFWD_SUPPORTED */

/*
 * Public
 */

/* Definitions of bits in internet address */
/* (!) addresses are in host-byte order (!) */

#define IN_CLASSA(i)            (((NGuint)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          0x00ffffff
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((NGuint)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          0x0000ffff
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((NGuint)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          0x000000ff

#define	IN_CLASSD(i)		    (((NGuint)(i) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(i)		    IN_CLASSD(i)
#define	IN_MULTICAST_H(i)	    (((NGuint)(i) & 0xf0) == 0xe0)

#define	IN_EXPERIMENTAL(i)	    (((NGuint)(i) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(i)		    (((NGuint)(i) & 0xf0000000) == 0xf0000000)

#define INADDR_ANY              (NGuint)0x00000000
#define INADDR_LOOPBACK         0x7f000001
#define INADDR_BROADCAST        (NGuint)0xffffffff
#define INADDR_NONE             0xffffffff

/* the same in network-byte order */
#if defined (NG_BIG_ENDIAN)
#define NG_IN_CLASSA(a)         IN_CLASSA(a)
#define NG_IN_CLASSA_NET        IN_CLASSA_NET
#define NG_IN_CLASSB(a)         IN_CLASSB(a)
#define NG_IN_CLASSB_NET        IN_CLASSB_NET
#define NG_IN_CLASSC(a)         IN_CLASSC(a)
#define NG_IN_CLASSC_NET        IN_CLASSC_NET
#define NG_IN_CLASSD(a)         IN_CLASSD(a)
#define NG_IN_BADCLASS(a)       IN_BADCLASS(a)
#define NG_INADDR_LOOPBACK      INADDR_LOOPBACK
#define NG_INADDR(a,b,c,d) \
    ((((NGuint)(a))<<24)|(((NGuint)(b))<<16)|(((NGuint)(c))<<8)|((NGuint)(d)))
#elif defined (NG_LITTLE_ENDIAN)
#define NG_IN_CLASSA(a)         (((NGuint)(a) & 0x00000080UL) == 0x00000000UL)
#define NG_IN_CLASSA_NET        (0x000000ffUL)
#define NG_IN_CLASSB(a)         (((NGuint)(a) & 0x000000c0UL) == 0x00000080UL)
#define NG_IN_CLASSB_NET        (0x0000ffffUL)
#define NG_IN_CLASSC(a)         (((NGuint)(a) & 0x000000e0UL) == 0x000000c0UL)
#define NG_IN_CLASSC_NET        (0x00ffffffUL)
#define	NG_IN_CLASSD(a)		    (((NGuint)(a) & 0x000000f0UL) == 0x000000e0UL)
#define	NG_IN_BADCLASS(a)		(((NGuint)(a) & 0x000000f0UL) == 0x000000f0UL)
#define NG_INADDR_LOOPBACK      (0x0100007fUL)
#define NG_INADDR(a,b,c,d) \
    ((((NGuint)(d))<<24)|(((NGuint)(c))<<16)|(((NGuint)(b))<<8)|((NGuint)(a)))
#else
#error "NG_XXX_ENDIAN macro not defined"
#endif
#define NG_INADDR_ANY           INADDR_ANY
#define NG_INADDR_BROADCAST     INADDR_BROADCAST

/* protocols */
#define IPPROTO_IP          0
#define IPPROTO_ICMP        1
#define IPPROTO_IGMP        2
#define IPPROTO_GGP         3
#define IPPROTO_TCP         6
#define IPPROTO_EGP         8
#define IPPROTO_PUP         12
#define IPPROTO_UDP         17
#define IPPROTO_IDP         22
#define IPPROTO_TP          29
#define IPPROTO_EON         80
#define IPPROTO_RAW         255
#define IPPROTO_MAX         256

/* udp and tcp reserved ports */
#define IPPORT_RESERVED         1024
#define IPPORT_USERRESERVED     5000

#define IPPORT_ECHO             7
#define IPPORT_DISCARD          9
#define IPPORT_SYSTAT           11
#define IPPORT_DAYTIME          13
#define IPPORT_NETSTAT          15
#define IPPORT_FTP              21
#define IPPORT_TELNET           23
#define IPPORT_SMTP             25
#define IPPORT_TIMESERVER       37
#define IPPORT_NAMESERVER       42
#define IPPORT_WHOIS            43
#define IPPORT_MTP              57
#define IPPORT_BOOTP            67
#define IPPORT_BOOTPC           68
#define IPPORT_TFTP             69
#define IPPORT_RJE              77
#define IPPORT_FINGER           79
#define IPPORT_TTYLINK          87
#define IPPORT_SUPDUP           95

/* route entry */
typedef struct {
    NGifnet    *rt_ifnetp;
    NGuint      rt_gateway;
    NGuint      rt_addr;
    NGuint      rt_subnetmask;
} NGiprtent;

/* current protocols */
#define NG_IPPROTO_RAW    0       /* SOCK_RAW */
#define NG_IPPROTO_UDP    1       /* SOCK_DGRAM */
#define NG_IPPROTO_TCP    2       /* SOCK_STREAM */
#define NG_IPPROTO_MAX    3

/* IP options */
/* Warning: values from 440 to 448 are used by NAT module (see below) */
#define NG_IPO_TTL              0x0400
#define NG_IPO_TOS              0x0401
#define NG_IPO_FRAG_TIMEO       0x0403
#define NG_IPO_ROUTE_DEFAULT    0x0404
#define NG_IPO_ROUTE_MAX        0x0405
#define NG_IPO_ROUTE_TABLE      0x0406
#define NG_IPO_ROUTE_ADD        0x0407
#define NG_IPO_ROUTE_DELETE     0x0408
#define NG_IPO_FORWARD          0x0481
#define NG_IPO_SENDREDIRECT     0x0482
#define NG_ICMPO_MASKREPLY      0x0484
#define NG_UDPO_CHECKSUM        0x0488
#define NG_IPO_FLAGSMASK        0x007f

/* Following are used only if NAT forwarding is supported */
#define NG_IPO_NAT_TABLEMAX     0x0440
#define NG_IPO_NAT_TABLE        0x0441
#define NG_IPO_NAT_ALGMAX       0x0442
#define NG_IPO_NAT_ALG          0x0443
#define NG_IPO_NAT_ADDEXTIF     0x0444
#define NG_IPO_NAT_TO_TCP       0x0445
#define NG_IPO_NAT_TO_TCP_FIN   0x0446
#define NG_IPO_NAT_TO_UDP       0x0447
#define NG_IPO_NAT_TO_ICMP      0x0448

int      ngRouteAdd( NGuint dest, NGuint subnet, NGuint gateway);
int      ngRouteDelete( NGuint dest);
int      ngRouteDefault( NGuint gateway);

int      ngInetATON( const char *str, NGuint *addr);
int      ngInetNTOA( NGuint addr, char *buffer, int buflen);
NGushort ngInetChecksum( void *ptr, int len);

/* host/network byte order conversions */
#define ngHTONS( val)       ngHTOBE16( val)
#define ngHTONL( val)       ngHTOBE32( val)
#define ngNTOHS( val)       ngBETOH16( val)
#define ngNTOHL( val)       ngBETOH32( val)
#define ngConstHTONS( val)  ngConstHTOBE16( val)
#define ngConstHTONL( val)  ngConstHTOBE32( val)
#define ngconstNTOHS( val)  ngConstBETOH16( val)
#define ngConstNTOHL( val)  ngConstBETOH32( val)

/* IP protocol(s) */
extern NGproto      ngProto_IP;
extern NGproto      ngProto_RAWIP;

/*
 * Private
 */

/* incoming fragments reassembly structure */
typedef struct {
    NGnode  ipf_node;   /* link to fragments */
    int     ipf_ttl;    /* time to live */
    int     ipf_p;      /* ip protocol */
    u_int   ipf_id;     /* fragment id */
    NGuint  ipf_src;    /* source address */
    NGuint  ipf_dst;    /* destination address */
    int     ipf_hlen;   /* first fragment ip header length */
} NGipfrag;

#ifdef NG_IPFRAG_SUPPORTED
#define NG_IPFRAG_NBMAX     4
#define NG_IPFRAG_SIZEMAX   8*1500
#else
#define NG_IPFRAG_NBMAX     1
#define NG_IPFRAG_SIZEMAX   ngBufDataMax
#endif

/* global variables */
extern int          ngIp_ttl;               /* default ttl */
extern int          ngIp_tos;               /* default tos */
extern int          ngIp_flags;             /* global flags */
extern u_int        ngIp_frag_timeo;        /* reassembly fragment timeout */
extern int          ngIp_route_max;         /* size of routing table */
extern NGiprtent   *ngIp_route_table;       /* routing table */
extern NGiprtent    ngIp_route_default;     /* default gateway */

extern NGproto     *ngIpProtoTable[NG_IPPROTO_MAX];

/* IP function prototypes */
int ngIpOutput( NGbuf *bufp, int flags);

/* Defining ngIpOutput() flags */
#define NG_IPOUTPUT_IPFWD      0x1 /* only if IPFWD supported */
#define NG_IPOUTPUT_NATFWD     0x2 /* only if both IPFWD and NATFWD supported */

/* routing internal functions */
NGifnet *ngIpRouteLookup( NGuint addr, NGuint *gateway);
void     ngIpRouteUpdate( void);

/* checksum computation */
NGushort in_cksum( void *ptr, int len, NGiovec *iov, int iovcnt);
void     ngCksumAdjust( NGubyte *sum, NGubyte *optr, int olen,
                        NGubyte *nptr, int nlen);

NG_END_DECLS

#endif


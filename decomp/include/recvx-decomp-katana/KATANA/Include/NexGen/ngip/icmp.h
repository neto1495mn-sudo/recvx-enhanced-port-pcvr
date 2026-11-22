/*****************************************************************************
 * $Id: icmp.h,v 1.2 2000/08/18 16:58:07 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Internet Control Message Protocol Definitions
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
 * 22/09/98 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_ICMP_H_INCLUDED__
#define __NG_ICMP_H_INCLUDED__

#include <ngip/ip.h>

NG_BEGIN_DECLS

/* ICMP header definition */
typedef struct {
    NGubyte     icmp_type;      /* type of message */
    NGubyte     icmp_code;      /* type sub-code */
    NGushort    icmp_cksum;     /* checksum */
    union {
        NGubyte     ih_pptr;    /* ICMP_PARAMPROB */
        NGuint      ih_gwaddr;  /* ICMP_REDIRECT */
        struct ih_idseq {
            NGushort    icd_id;
            NGushort    icd_seq;
        } ih_idseq;
        NGint       ih_void;
        struct ih_pmtu {        /* ICMP_UNREACH_NEEDFRAG */
            NGushort    ipm_void;
            NGushort    ipm_nextmtu;
        } ih_pmtu;
    } icmp_hun;
#define icmp_pptr       icmp_hun.ih_pptr
#define icmp_gwaddr     icmp_hun.ih_gwaddr
#define icmp_id         icmp_hun.ih_idseq.icd_id
#define icmp_seq        icmp_hun.ih_idseq.icd_seq
#define icmp_void       icmp_hun.ih_void
#define icmp_pmvoid     icmp_hun.ih_pmtu.ipm_void
#define icmp_nextmtu    icmp_hun.ih_pmtu.ipm_void
    union {
        struct id_ts {
            NGuint      its_otime;
            NGuint      its_rtime;
            NGuint      its_ttime;
        } id_ts;
        NGiphdr     id_ip;
        NGuint      id_mask;
        NGubyte     id_data[1];
    } icmp_dun;
#define icmp_otime      icmp_dun.id_ts.its_otime
#define icmp_rtime      icmp_dun.id_ts.its_rtime
#define icmp_ttime      icmp_dun.id_ts.its_ttime
#define icmp_ip         icmp_dun.id_ip
#define icmp_mask       icmp_dun.id_mask
#define icmp_data       icmp_dun.data
} NGicmp;


/*
 * Lower bounds on packet lengths for various types.
 * For the error advice packets must first insure that the
 * packet is large enought to contain the returned ip header.
 * Only then can we do the check to see if 64 bits of packet
 * data have been returned, since we need to check the returned
 * ip header length.
 */
#define ICMP_MINLEN 8               /* abs minimum */
#define ICMP_TSLEN  (8 + 3 * sizeof (n_time))   /* timestamp */
#define ICMP_MASKLEN    12              /* address mask */
#define ICMP_ADVLENMIN  (8 + sizeof (struct ip) + 8)    /* min */
#define ICMP_ADVLEN(p)  (8 + ((p)->icmp_ip.ip_hl << 2) + 8)
    /* N.B.: must separately check that ip_hl >= 5 */

/*
 * Definition of type and code field values.
 */
#define ICMP_ECHOREPLY      0       /* echo reply */
#define ICMP_UNREACH        3       /* dest unreachable, codes: */
#define     ICMP_UNREACH_NET    0       /* bad net */
#define     ICMP_UNREACH_HOST   1       /* bad host */
#define     ICMP_UNREACH_PROTOCOL   2       /* bad protocol */
#define     ICMP_UNREACH_PORT   3       /* bad port */
#define     ICMP_UNREACH_NEEDFRAG   4       /* IP_DF caused drop */
#define     ICMP_UNREACH_SRCFAIL    5       /* src route failed */
#define ICMP_SOURCEQUENCH   4       /* packet lost, slow down */
#define ICMP_REDIRECT       5       /* shorter route, codes: */
#define     ICMP_REDIRECT_NET   0       /* for network */
#define     ICMP_REDIRECT_HOST  1       /* for host */
#define     ICMP_REDIRECT_TOSNET    2       /* for tos and net */
#define     ICMP_REDIRECT_TOSHOST   3       /* for tos and host */
#define ICMP_ECHO       8       /* echo service */
#define ICMP_TIMXCEED       11      /* time exceeded, code: */
#define     ICMP_TIMXCEED_INTRANS   0       /* ttl==0 in transit */
#define     ICMP_TIMXCEED_REASS 1       /* ttl==0 in reass */
#define ICMP_PARAMPROB      12      /* ip header bad */
#define ICMP_TSTAMP     13      /* timestamp request */
#define ICMP_TSTAMPREPLY    14      /* timestamp reply */
#define ICMP_IREQ       15      /* information request */
#define ICMP_IREQREPLY      16      /* information reply */
#define ICMP_MASKREQ        17      /* address mask request */
#define ICMP_MASKREPLY      18      /* address mask reply */

#define ICMP_MAXTYPE        18

#define ICMP_INFOTYPE(type) \
    ((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO || \
    (type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY || \
    (type) == ICMP_IREQ || (type) == ICMP_IREQREPLY || \
    (type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)


NG_END_DECLS

#endif


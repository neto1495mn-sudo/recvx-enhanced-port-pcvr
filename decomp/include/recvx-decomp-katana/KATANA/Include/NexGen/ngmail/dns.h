/*****************************************************************************
 * $Id: dns.h,v 1.1.1.1 2000/09/19 09:51:54 af Exp $
 *----------------------------------------------------------------------------
 * NexGenMAIL v1.2
 * Domain Name System Definitions
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
 * 01/06/1999 - Regis Feneon
 * 06/04/2000 -
 *  added test for shc compiler
 * 17/07/2000 - Serge Plagnol - Sega Europe
 *  Added some extra functions declarations
 *****************************************************************************/

#ifndef __NG_DNS_H_INCLUDED__
#define __NG_DNS_H_INCLUDED__

#include <ngos.h>

NG_BEGIN_DECLS

/* opcodes */
#define DNS_QUERY       0x0     /* standart query */
#define DNS_IQUERY      0x1     /* inverse query */
#define DNS_STATUS      0x2     /* nameserver status query */

/* response codes */
#define DNS_NOERROR     0       /* no error */
#define DNS_FORMERR     1       /* format error */
#define DNS_SERVFAIL    2       /* server failure */
#define DNS_NXDOMAIN    3       /* non existent domain */
#define DNS_NOTIMP      4       /* not implemented */
#define DNS_REFUSED     5       /* query refused */

/* type values */
#define DNS_T_A         1       /* host address */
#define DNS_T_NS        2       /* authoritative server */
#define DNS_T_CNAME     5       /* canonical name */
#define DNS_T_PTR       12      /* domain name pointer */

/* class values */
#define DNS_C_IN        1       /* Internet */
#define DNS_C_ANY       255     /* wildcard */

/* DNS query header */
typedef struct {
    NGushort    dns_id;         /* query identifiant value */
#if defined(NG_LITTLE_ENDIAN) && !defined(NG_COMP_SHC)
    NGubyte     dns_rd:1,       /* recursion desired */
                dns_tc:1,       /* truncated messge */
                dns_aa:1,       /* authoritative answer */
                dns_opcode:4,   /* type of query */
                dns_qr:1;       /* response flag */
    NGubyte     dns_rcode:4,    /* response code */
                dns_zero:3,     /* unused */
                dns_ra:1;       /* recursion available */
#else
    NGubyte     dns_qr:1,       /* response flag */
                dns_opcode:4,   /* type of message */
                dns_aa:1,       /* authoritative answer */
                dns_tc:1,       /* truncated messge */
                dns_rd:1;       /* recursion desired */
    NGubyte     dns_ra:1,       /* recursion available */
                dns_zero:3,     /* unused */
                dns_rcode:4;    /* response code */
#endif
    NGushort    dns_qdcount;    /* number of question entries */
    NGushort    dns_ancount;    /* number of answer entries */
    NGushort    dns_nscount;    /* number of authority entries */
    NGushort    dns_arcount;    /* number of resource entries */
} NGdnshdr;

/* internal functions */
int dns_write_name( NGubyte *dst, const char *src);
int dns_read_name( char *dst, int dstlen, NGubyte *src, NGubyte *src_base);
int dns_skip_name( NGubyte *src);

NG_END_DECLS

#endif /* __NG_DNS_H_INCLUDED__ */

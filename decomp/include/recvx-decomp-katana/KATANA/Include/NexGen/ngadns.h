/*****************************************************************************
 * adns.h
 *----------------------------------------------------------------------------
 * NexGenMAIL v1.2
 * Domain Name System Interface
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
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 27/06/00 - Thomas Szirtes - Sega Europe
 * 28/06/00 - Serge Plagnol - Sega Europe
 *  restructured the ngngADnsMem structure to remove uneeded element and
 *  and improve packing
 *  redefined ngADnsTicket to a signed byte
 * 02/08/00 - David Harvey - Sega Europe
 *  added a current server element
 *----------------------------------------------------------------------------
 *****************************************************************************/

#ifndef __NGADNS_H_INCLUDED__
#define __NGADNS_H_INCLUDED__

#include <ngdns.h>
#include <ngmail\dns.h>
#include <ngnetdb.h>

NG_BEGIN_DECLS
typedef	struct {
    NGuint 		ttl;
    NGsock 		*so;
    NGbuf 		*bufp;
    NGdnshdr 	*dp;
    NGubyte 	*ptr;
    NGushort    id, qtype, qclass, reslen;
    NGuint		tm; // Modified by IJO @ VC
    NGsockctrl 	sc;
	NGbyte		iswitch, iretry;
	NGbyte		state;
	NGbyte		curserv;
	char		query[NG_DNS_NAME_MAX+1];
} ngADnsMem;

#define		ADNS_WORKSIZE(x)	(x*sizeof(ngADnsMem))


typedef	NGbyte	ngADnsTicket;

typedef	struct {
	ngADnsTicket	ticket;
	struct hostent  *addr;
} ngADnsAnswer;


// ----- Simple Interface ----- //

int ngADnsInit(unsigned max_queries, void *workmemory);

int	ngADnsGetTicket(ngADnsTicket *pticket, char *query);

int ngADnsPoll( ngADnsAnswer *answer );

int	ngADnsReleaseTicket(ngADnsTicket *pticket);

// ----- Complex Interface ----- //

int ngADnsQuery( ngADnsTicket ticket, int query_type, char *query_str,
                 char *name, int namelen, NGuint *addr_list, int *naddr);

NG_END_DECLS

#endif /* __NGADNS_H_INCLUDED__ */
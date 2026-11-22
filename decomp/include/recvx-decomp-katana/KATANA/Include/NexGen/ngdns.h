/*****************************************************************************
 * $Id: ngdns.h,v 1.1.1.1 2000/09/19 09:51:53 af Exp $
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
 *  agreement.
 *
 *----------------------------------------------------------------------------
 * 01/06/1999 - Regis Feneon
 * 15/07/1999 -
 *	global variables are stored in NGdnsdef structure
 *	macros to change DNS parameters
 * 21/07/2000 - Adrien Felon
 *  support for secondary DNS added
 * 04/09/2000 -
 *  NG_CALL macro removed
 *****************************************************************************/

#ifndef __NGDNS_H_INCLUDED__
#define __NGDNS_H_INCLUDED__

#include <ngos.h>

NG_BEGIN_DECLS

/*
 * Public
 */

/* type of query for ngDnsQuery() */
#define NG_DNS_QUERY_ADDR       0   /* address (A) */
#define NG_DNS_QUERY_NAME       1   /* name query (PTR) */

int ngDnsInit( const char *domain, NGuint dns1, NGuint dns2);
int ngDnsInitExt( int timeout, int retrymax, int switchmax);

int ngDnsQuery( int qr_type, char *qr_str, char *name, int namelen,
                         NGuint *addr_list, int *naddr);
int ngDnsQueryByAddr( NGuint addr, char *name, int namelen,
                         NGuint *addr_list, int *naddr);
/* int  ngDnsQueryByName( const char *hostname, NGuint *addr_list, int *naddr); */
#define ngDnsQueryByName( hostname, addr_list, naddr) \
    ngDnsQuery( NG_DNS_QUERY_ADDR, hostname, NULL, 0, addr_list, naddr)

#define NG_DNS_ARPALEN_MAX  sizeof( "255.255.255.255.in-addr.arpa")
int ngDnsMakePTRQuery( NGuint addr, char *buf, int buflen);

/*
 * Private
 */

/* transmission parameters */
#define NG_DNS_TIMEOUT       4      /* 4 secondes before retransmit */
#define NG_DNS_RETRYMAX      8      /* retransmit 8 times */
#define NG_DNS_SWITCHMAX     1      /* only switching once */

#define NG_DNS_PORT         53      /* UDP server port number */

/* global structure */
typedef struct {
  char       *dns_domain;     /* domain name */
  NGuint      dns_addr1;      /* primary server */
  NGuint		  dns_addr2;      /* secondary server */
  int         dns_timeout;    /* number of second before rmit */
  int         dns_retrymax;   /* number of rmit per server */
  int         dns_switchmax;  /* number of switch between 1st/2nd server */
  NGuint      dns_curserv;    /* current server (0 for 1st, otherwise 2nd) */
} NGdnsdata;

/* variables */
extern NGdnsdata	ngDns_data;

/* Changing DNS module parameters (ngDnsInit/ngDnsInitExt) */
int ngDnsInitMod( const char *domain, NGuint dns1, NGuint dns2, 
                          int timeout, int retrymax, int switchmax);

NG_END_DECLS

#endif /* __NGDNS_H_INCLUDED__ */

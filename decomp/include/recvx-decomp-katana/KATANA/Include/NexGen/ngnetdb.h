/*****************************************************************************
 * $Id: ngnetdb.h,v 1.1.1.1 2000/09/19 09:51:53 af Exp $
 *----------------------------------------------------------------------------
 * NexGenMAIL v1.2
 * BSD Compatible DNS resolver routines
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
 *  This file includes parts which are Copyright (c) 1982-1988 Regents
 *  of the University of California.  All rights reserved.  The
 *  Berkeley Software License Agreement specifies the terms and
 *  conditions for redistribution.
 *----------------------------------------------------------------------------
 * 04/08/1999 - Regis Feneon
 * 04/09/2000 - Adrien Felon
 *  NG_CALL macro removed
 *****************************************************************************/

#ifndef __NG_NETDB_H_INCLUDED__
#define __NG_NETDB_H_INCLUDED__

#include <ngos.h>

NG_BEGIN_DECLS

/*
 * Public
 */

/* host information */
struct hostent {
    char   *h_name;         /* host name */
    char  **h_aliases;      /* list of aliases */
    int     h_addrtype;     /* type of address (always AF_INET) */
    int     h_length;       /* length of address */
    char  **h_addr_list;    /* list of addresses */
};
#define h_addr h_addr_list[0]

/* error codes */
#define HOST_NOT_FOUND  1
#define TRY_AGAIN       2
#define NO_RECOVERY     3
#define NO_DATA         4
#define NO_ADDRESS      NO_DATA
/* global error */
extern int h_errno;

struct hostent * gethostbyname( char *name);
struct hostent * gethostbyaddr( char *addr, int len, int type);

/* threads-safe routines */
struct hostent * gethostbyname_r( char *name, struct hostent *result,
                                 void *buf, int buflen, int *err);
struct hostent * gethostbyaddr_r( char *addr, int len, int type,
                     struct hostent *result, void *buf, int buflen, int *err);

/* macro to define min length of buffer for both prev routines (133 bytes) */
#define NG_DNS_R_MIN_BUFLEN ((sizeof(NGuint*)*(NG_DNS_ADDR_MAX+1)) + \
 (sizeof(NGuint) * NG_DNS_ADDR_MAX) + NG_DNS_NAME_MAX+1)

 /* 9 ptr (0 terminated list) + 8 IP addresses + 65 bytes for hostname */

/*
 * Private
 */

/* static storage for gethostxxx routines */
extern NGuint           ngDns_HostAddr[];
extern NGuint          *ngDns_HostAddrList[];
extern char             ngDns_HostName[];
extern struct hostent   ngDns_HostEntry;

/* maximum number of addressses returned by gethostxxx routines */
#define NG_DNS_ADDR_MAX     8
/* maximum length of host name */
#define NG_DNS_NAME_MAX     64

NG_END_DECLS

#endif

/*****************************************************************************
 * $Id: bootps.h,v 1.1.1.1 2000/09/19 09:50:37 af Exp $
 *----------------------------------------------------------------------------
 * NexGenBOOT v1.2
 * Dynamic Host Configuration Protocol definition
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
 * Definitions needed by Server
 * Strucure:  NGdhcpServer NgdhcpIpInfo NgdhcpRange
 *----------------------------------------------------------------------------
 * 24/05/00 - Abelwahad El Mourabit
 *****************************************************************************/

#ifndef __BOOTS_H_INCLUDED__
#define __BOOTS_H_INCLUDED__

#include <ngos.h>
#include <ngip/ethernet.h> /* ETHERMTU */

#include <ngboot/bp_tags.h>
#include <ngboot/bootp.h>

NG_BEGIN_DECLS

#define BP_GENERAL_FILE_LEN   16        /* max size of general file names  */
#define MAX_NBR_BOOTP_OPTIONS 32        /* max number of options the       */
                                        /* BOOTP server could provide      */

/* The first structure link a gereral file name with a full path file
 * name */
typedef struct {
  NGubyte  bfn_gname[BP_FILE_LEN]; /* general file name like 'LINUX' */
  NGubyte  bfn_file[BP_FILE_LEN];  /* full boot file name associated
                                      to this general file name like
                                      'user/linux/boot' */
} NGbootpsfilename;

/* The second one link a client ( known by its hard address ) with its
 * config */
typedef struct {
  NGubyte  bci_hwadress[BP_CHADDR_LEN]; /* Hw addr of the client */
  NGuint   bci_ip;	                    /* IP addr of the client */
  NGubyte  bci_file[BP_FILE_LEN];       /* full boot file name
                                           associated to tis client */
} NGbootpsclientinfo;


/*****************************************************************************
 * Defining NexGen BOOTPS protocol
 *****************************************************************************/

/* BOOTP server specific configurating options (argument type is commented) */
/* Thanks thoses options, user will be able to give BOOTPS data base */
/* to the server and  also configure the server.*/

/* link a gereral file name with a full path file name */
#define NG_DHCPSO_BOOTP_FILE      200  /* NGbootpsfilename* */
/* 	Number of general file name */
#define NG_DHCPSO_BP_FILE_NBR     210  /* NGuint */
/* configuration of each BOOTP client */
#define NG_DHCPSO_BOOTP_CLIENT    220  /* NGbootpsclientinfo*  */
/* Number of client */
#define NG_DHCPSO_BP_CLI_NBR      230  /* NGuint */

NG_END_DECLS

#endif /* __BOOTS_H_INCLUDED__ */

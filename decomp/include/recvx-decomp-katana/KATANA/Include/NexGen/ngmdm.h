/*****************************************************************************
 * $Id: ngmdm.h,v 1.3 2000/08/31 18:11:36 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * modem user interface implementation
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
 * 31/03/2000 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_NGMDM_H_INCLUDED__
#define __NG_NGMDM_H_INCLUDED__

#include <ngos.h>
#include <ngdev.h>

NG_BEGIN_DECLS

typedef struct{
  char  *mdm_action;    /* string to be sent */
  char  *mdm_answer;    /* expected response */
  char  *mdm_output;    /* string sent when mdm_answer matched */
  int    mdm_goto;      /* next line of script if when mdm_answer matched */
  int    mdm_retval;    /* return value when answer match & mdm_goto is <0 */
  int    mdm_timeout;   /* answer timeout in second */
} NGmdmscript;

#define NG_MODEM_CMD_MAX    16

typedef struct {
    NGdevcb     *mdst_dcb;          /* device control block */
    NGmdmscript *mdst_script;       /* modem script */
    int          mdst_current;      /* script line index (-1 = init, -2 = finished) */
    int          mdst_error;        /* error code (when script is finished) */
    NGmdmscript *mdst_currentp;     /* pointer to current script line */
    char         mdst_buf[NG_MODEM_CMD_MAX];    /* input buffer */
    int          mdst_buflen;                   /* # of chars in input buffer */
    NGuint       mdst_timeo;        /* current timeout */
} NGmdmstate;

int ngModemInit( NGmdmstate *mdm, NGdevcb *dcb, NGmdmscript *script);
int ngModemPoll( NGmdmstate *mdm);
int ngModemScript( NGdevcb *dcb, NGmdmscript *script);
int ngModemDropLines( NGdevcb * dcb,
                      unsigned long droptime,
                      unsigned long timeout);

NG_END_DECLS

#endif


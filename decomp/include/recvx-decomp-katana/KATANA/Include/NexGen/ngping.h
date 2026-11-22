/*****************************************************************************
 * $Id: ngping.h,v 1.4 2000/09/14 14:57:38 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * Ping (ICMP echo request) definition
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
 * 05/01/2000 - herve RUAULT
 * 06/09/2000 - Regis Feneon
 *  added 'data' argument to display callback
 *  added display_data member in config structure
 *  added 'saddr' argument to display callback
 *****************************************************************************/

#ifndef __NG_NGPING_H_INCLUDED__
#define __NG_NGPING_H_INCLUDED__

#include <ngip.h>
#include <ngdsock.h>

NG_BEGIN_DECLS

#define NGPINGDEF_repeat          1 /* default repeat ping value */
#define NGPINGDEF_trepeat      1000 /* default time between 2 pings = 1 sec */
#define NGPINGDEF_rcv_timeo_ms 1000 /* default receive time out = 1 sec */
#define NGPINGDEF_ttl            64 /* default time to live */
#define NGPINGDEF_datalen        20 /* default extra data length */

typedef struct _cfgping {
    int     repeat;                  /* default = 1 ; -1 = infinite */
    NGuint  trepeat;                 /* time between 2 request	default = 1 s */
    NGuint  rcv_timeo_ms;            /* default = 1 second */
    int     ttl;                     /* time to live */
    NGuint  data_len;                /* sizeof frame */
    void  (*display)(int seq, int err, int datasize, int ttl, int time,
                      NGsockaddr *saddr, void *data);
    void   *display_data;
} NGcfgping;

int ngPing(NGsockaddr *addr, NGcfgping *cfgping);

NG_END_DECLS

#endif /* __NG_PING_H_INCLUDED__ */


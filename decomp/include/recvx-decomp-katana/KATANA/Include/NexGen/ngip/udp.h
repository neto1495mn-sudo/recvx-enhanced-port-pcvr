/*****************************************************************************
 * $Id: udp.h,v 1.2 2000/08/18 16:58:07 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenIP v1.2
 * UDP Protocol Low-Level Definitions
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
 * 12/10/98 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_UDP_H_INCLUDED__
#define __NG_UDP_H_INCLUDED__

NG_BEGIN_DECLS

typedef struct {
    NGushort    uh_sport;
    NGushort    uh_dport;
    NGushort    uh_ulen;
    NGushort    uh_sum;
} NGudphdr;

NG_END_DECLS

#endif


/*****************************************************************************
 * $Id: ngmd5.h,v 1.1 2000/09/13 18:36:01 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenPPP v1.2
 * MD5 Message-Digest Algorithm Routines
 *----------------------------------------------------------------------------
 *		Copyright (c) 2000 NexGen Software.
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
 * 28/08/2000 - Regis Feneon
 *  functions and structures renamed
 *****************************************************************************/

#ifndef __NG_NGMD5_H_INCLUDED__
#define __NG_NGMD5_H_INCLUDED__

#include <ngos.h>

typedef struct {
  u_long    buf[4];
  u_long    bits[2];
  NGubyte   in[64];
} NGmd5ctx;

void ngMD5Init( NGmd5ctx *context);
void ngMD5Update( NGmd5ctx *context, const NGubyte *buf, unsigned len);
void ngMD5Final( NGubyte *digest, NGmd5ctx *context);

#endif


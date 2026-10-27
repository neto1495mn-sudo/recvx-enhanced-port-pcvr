/*****************************************************************************
 * $Id: stdlib.h,v 1.4 2000/08/30 16:04:55 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Standard libraries replacement functions
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
 * 19/07/2000 - Regis Feneon
 * 09/08/2000 -
 *  added ngMemChr(),ngStrChr(),ngStrRChr(),ngStrStr()
 *  added ngStrNCat(),ngStrNCmp(),ngStrNCpy()
 *****************************************************************************/

#ifndef __NG_STDLIB_H_INCLUDED__
#define __NG_STDLIB_H_INCLUDED__

/*
 * Standart lib routines
 */

#ifndef NG_STRING

#include <string.h>

/* byte string manipulations */
#define ngMemChr( s, c, l)      memchr( s, c, l)
#define ngMemCmp( d, s, l)      memcmp( d, s, l)
#define ngMemCpy( d, s, l)      memcpy( d, s, l)
#define ngMemMove( d, s, l)     memmove( d, s, l)
#define ngMemSet( d, c, l)      memset( d, c, l)

/* string manipulation routines */
#define ngStrCat( d, s)         strcat( d, s)
#define ngStrChr( s, c)         strchr( s, c)
#define ngStrCmp( s1, s2)       strcmp( s1, s2)
#define ngStrCpy( d, s)         strcpy( d, s)
#define ngStrLen( s)            strlen( s)
#define ngStrNCat( d, s, n)     strncat( d, s, n)
#define ngStrNCmp( s1, s2, n)   strncmp( s1, s2, n)
#define ngStrNCpy( d, s, n)     strncpy( d, s, n)
#define ngStrRChr( s, c)        strrchr( s, c)
#define ngStrStr( s1, s2)       strstr( s1, s2)

#endif

#endif


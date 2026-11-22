/*****************************************************************************
 * $Id: macros.h,v 1.7 2000/09/20 18:11:55 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Hitachi SH3/SH4 Architectures
 * Compiler dependant macros
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998-2000 NexGen Software.
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
 * 09/03/2000 - Regis Feneon
 *  Support for Hitachi SHC compiler
 * 30/05/2000 -
 *  added NG_BIG_ENDIAN_BITFIELD
 * 26/07/2000 -
 *  types.h moved here
 *  ngHTONS()/ngNTOHS() macros replaced by ngBSwap16()/ngBSwap32()
 *  standard libs moved to ngos/stdlib.h
 *  added include of unaligned data handling
 *  added ngIntrCtl() macro
 *****************************************************************************/

#ifndef __NG_MACROS_H_INCLUDED__
#define __NG_MACROS_H_INCLUDED__

/* C++ compatibility */
#if defined(__cplusplus) || defined(__CPLUSPLUS)
#define NG_BEGIN_DECLS  extern "C" {
#define NG_END_DECLS    }
#else
#define NG_BEGIN_DECLS
#define NG_END_DECLS
#endif

NG_BEGIN_DECLS

/* standart c-types */
#ifndef NG_UTYPES
typedef unsigned int    u_int;      /* unsigned >= 16 bits */
typedef unsigned long   u_long;     /* unsigned >= 32 bits */
#endif

/* fixed size types */
typedef unsigned char   NGubyte;    /* 8 bits unsigned */
typedef unsigned short  NGushort;   /* 16 bits unsigned */
typedef unsigned long   NGuint;     /* 32 bits unsigned */
typedef signed char     NGbyte;     /* 8 bits signed */
typedef signed short    NGshort;    /* 16 bits signed */
typedef signed long     NGint;      /* 32 bits signed */

#define NGfar
#define NGcodefar

#define NG_ALIGN32
#define NG_ALIGN_STRICT

#if defined(NG_COMP_SHC)
/*
 * Hitachi SHC Compiler
 */

#define NG_BIG_ENDIAN_BITFIELD

/* byte swapping */
NGushort ngBSwap16( NGushort val);
NGuint   ngBSwap32( NGuint val);

#include <machine.h>

/* disable/enable interrupts - use intrinsic functions */
/* set BL in SR register to mask interrupts */
/* clear BL to unmask */
#define ngDisable()     { set_cr( get_cr() |  0x10000000); }
#define ngEnable()      { set_cr( get_cr() & ~0x10000000); }
#pragma inline(ngIntrCtl)
static int ngIntrCtl( int ctl)
{
    int ret;
    ret = (get_cr() & 0x10000000) ? 0x1001 : 0x1000;
    if( ctl & 0x0001) { ngDisable(); }
    else              { ngEnable();  }
    return( ret);
}

#endif  /* NG_COMP_XXX */

#define NG_INTERRUPT
typedef void (*NGinterruptPTR)( void);

/* NULL pointer */
#ifndef NULL
#define NULL    0
#endif

/* physical->pointer address mapping */
/* "virtual" addressing ? */
#define ngMTOK( addr)   ((void *)(addr))
#define ngKTOM( ptr)    ((NGuint)(ptr))

NG_END_DECLS

/* variable argument list */
#include <stdarg.h>
#define NGva_list               va_list
#define NG_VA_START( ap, lf)    va_start( ap, lf)
#define NG_VA_ARG( ap, tp)      va_arg( ap, tp)
#define NG_VA_END( ap)          va_end( ap)
 
/* big/little endian handling */
#include <ngos/cpu/endian.h>

/* unaligned data handling */
#include <ngos/cpu/unalign.h>

#endif


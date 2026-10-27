/*****************************************************************************
 * $Id: ngos.h,v 1.9 2000/10/03 17:36:56 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Global Definitions
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
 * 18/07/2000 - Regis Feneon
 *  new file for v1.2
 * 24/08/2000 -
 *  new data type NGcfgarg
 * 19/09/2000 -
 *  new ngInit() error flags for device/interface open failures
 *****************************************************************************/

#ifndef __NG_NGOS_H_INCLUDED__
#define __NG_NGOS_H_INCLUDED__

/* cpu-specific includes */
#if defined(NG_CPU_I186S) || defined(NG_CPU_I186M) || defined(NG_CPU_I186C) \
 || defined(NG_CPU_I186L) || defined(NG_CPU_I286)
/* 80x86 16-bits */
#include <ngos/cpu/i186/macros.h>
#elif defined(NG_CPU_I386) || defined(NG_CPU_I486)
/* 80x86 32-bits */
#include <ngos/cpu/i386/macros.h>
#elif defined(NG_CPU_ARM)
/* ARM */
#include <ngos/cpu/arm/macros.h>
#elif defined(NG_CPU_PPC)
/* PowerPC */
#include <ngos/cpu/ppc/macros.h>
#elif defined(NG_CPU_SH3) || defined(NG_CPU_SH4)
/* Hitachi SH3/SH4 */
#include <ngos/cpu/sh7x/macros.h>
#elif defined(NG_CPU_68K)
/* Motorola 680x0 and CPU32 */
#include <ngos/cpu/m68k/macros.h>
#else
#error "Invalid NG_CPU_XXX macro"
#endif

/* rtos definitions */
#include <ngdefs.h>

/* standard includes */
#include <ngos/debug.h>
#include <ngos/errno.h>
#include <ngos/hw.h>
#include <ngos/isr.h>
#include <ngos/queue.h>
#include <ngos/stdio.h>
#include <ngos/stdlib.h>

NG_BEGIN_DECLS

/* Settings */
#define NG_TIMERHZ      5   /* 200ms stack timer */
#define NG_FD_MAX      32   /* maximum number of 'file' descriptors */

/* NexGenOS configuration table entry */
#ifndef NG_CFGARG
typedef unsigned long   NGcfgarg;
#endif
typedef struct {
    u_int       cfg_option;     /* option or command */
    NGcfgarg    cfg_arg;        /* argument */
} NGcfgent;

/* configuration commands */
#define NG_CFG_NOP              0xff00
#define NG_CFG_END              0xffff
#define NG_CFG_PROTOADD         0xff01
#define NG_CFG_IFADD            0xff02
#define NG_CFG_DEVADD           0xff03
#define NG_CFG_DRIVER           0xff04
#define NG_CFG_IFADDWAIT        0xff05
/* global stack options */
#define NG_BUFO_MAX             0xfe00
#define NG_BUFO_HEADER_SIZE     0xfe01
#define NG_BUFO_DATA_SIZE       0xfe02
#define NG_BUFO_ALLOC_F         0xfe03
#define NG_BUFO_INPQ_MAX        0xfe04
#define NG_SOCKO_MAX            0xfe05
#define NG_SOCKO_TABLE          0xfe06
#define NG_SOCKO_SEMTABLE       0xfe07
#define NG_SOCKO_SEMSIZE        0xfe08
#define NG_RTO_CLOCK_FREQ       0xfe09
#define NG_RTO_INPUT_PRIO       0xfe0a
#define NG_RTO_INPUT_STACK_SIZE 0xfe0b
#define NG_RTO_INPUT_STACK      0xfe0c
#define NG_RTO_TIMER_PRIO       0xfe0d
#define NG_RTO_TIMER_STACK      0xfe0e
#define NG_RTO_TIMER_STACK_SIZE 0xfe0f
#define NG_DEBUGO_LEVEL         0xfe10
#define NG_DEBUGO_MODULE        0xfe11
/* device io */
#define NG_DEVCBO_MAX           0xfe12
#define NG_DEVCBO_TABLE         0xfe13
#define NG_DEVCBO_SEMTABLE      0xfe14
/* extensions */
#define NG_BUFO_ALLOC_BUF_F     0xfe15
#define NG_BUFO_POOL_PTR        0xfe16
#define NG_BUFO_POOL_SIZE       0xfe17
/* v1.2 options */
#define NG_RTO_SEMSIZE      NG_SOCKO_SEMSIZE
#define NG_DEVCBO_SEMSIZE   NG_SOCKO_SEMSIZE
#define NG_DEVCBO_PRIO          0xfe18
#define NG_DEVCBO_STACK         0xfe19
#define NG_DEVCBO_STACK_SIZE    0xfe20
#define NG_BUFO_FREE_F          0xfe21
#define NG_BUFO_FREE_BUF_F      0xfe22

/* arguments */
#define NG_CFG_TRUE         ((NGcfgarg) ~0L)
#define NG_CFG_FALSE        ((NGcfgarg) 0L)
#define NG_CFG_LNG( a)      ((NGcfgarg) (a))
#define NG_CFG_INT( a)      ((NGcfgarg) (a))
#define NG_CFG_ADR(a,b,c,d) ((NGcfgarg) NG_INADDR(a,b,c,d))
#define NG_CFG_PTR( a)      ((NGcfgarg) (a))
#define NG_CFG_FNC( a)      ((NGcfgarg) (a))

/* modules control function */
#define NG_CNTL_SET         1
#define NG_CNTL_GET         0

/*
 * Global Data
 */

extern u_int    ngOS_clock_freq;    /* number of system clock ticks per sec */
extern u_long   ngTimer_nticks;     /* number of ticks for the stack timer */

#ifndef NG_RTOS

extern int ngGlobalErrno;           /* global error code */

/* Polling interface */
typedef struct NGpollent_S {
    struct NGpollent_S  *poll_next;                 /* pointer to next entry */
    void               (*poll_handler_f)( void *);  /* polling routine */
    void                *poll_handler_data;         /* routine data */
} NGpollent;

extern NGpollent *ngPoll_list;      /* list of polling routines */

#endif


#ifdef NG_RTOS
/* global synchronization */
extern NGOSmutex    ngGlobalLock;
#define NG_LOCK()   ngOSMutexLock( &ngGlobalLock)
#define NG_UNLOCK() ngOSMutexUnlock( &ngGlobalLock)
#else
#define NG_LOCK()
#define NG_UNLOCK()
#endif


/* Errors returned by ngInit() */
#define NG_EINIT_NOERROR        NG_EOK  /* initialization ok */
#define NG_EINIT_BUFALLOC       0x0001  /* error allocating buffers */
#define NG_EINIT_BUFOPTION      0x0002  /* bad buffer option */
#define NG_EINIT_SOCKTABLE      0x0004  /* bad socket table option */
#define NG_EINIT_DEVCBTABLE     0x0008  /* bad devive control blocks option */
#define NG_EINIT_CLOCKFREQ      0x0010  /* bad clock frequency */
#define NG_EINIT_INPUTTASK      0x0020  /* cannot create input task */
#define NG_EINIT_TIMERTASK      0x0040  /* cannot create timer task */
#define NG_EINIT_DEVIOTASK      0x0080  /* cannot create device task */
#define NG_EINIT_DEVOPEN        0x0100  /* device open failure */
#define NG_EINIT_IFOPEN         0x0200  /* interface open failure */

/*
 * Functions prototypes
 */

/* system interface */
int    ngInit( const NGcfgent *cfg);
void   ngExit( int status);
char  *ngGetVersionString( void);
void   ngSleep( u_long msec);
#ifndef NG_RTOS
void   ngYield( void);
int    ngPollAdd( NGpollent *pent, void (*func)( void *), void *data);
void   ngUserSetWorkProc( int (*func)( void *), void *data);
#endif
#ifndef NG_RTOS_INLINE
u_long ngOSClockGetTime( void);
void   ngOSExit( int status);
void   ngOSInit( void);
void   ngOSSleep( u_long dly);
int    ngOSIsrAttach( int irno, void (*handler_f)(void *), void *data);
int    ngOSIsrDetach( int handle);
int    ngOSIntrCtl( int ctl);
int    ngOSMemKTOM( void *ptr, NGuint *addr);
int    ngOSMemMTOK( NGuint addr, int len, void **ptr);
#endif
#ifndef NG_RTOS
#define ngOSGetErrno()      ngGlobalErrno
#define ngOSSetErrno( err)  ngGlobalErrno = err;
#endif

/* 1.1 compatibility */
#define ngIsrAttach(i,h,d)  ngOSIsrAttach((i),(h),(d))
#define ngIsrDetach(h)      ngOSIsrDetach((h))

/* standart libs replacement functions */
int    ngRawPrintf( void (*out_f)( int, void *), void *data,
                     const char *format, NGva_list args);
void   ngStdOutChar( int c, void *data);
int    ngStdInChar( void *data);
int    ngVSPrintf( char *buf, const char *format, NGva_list args);
int    ngSPrintf( char *buf, const char *format, ...);
int    ngVPrintf( const char *format, NGva_list args);
int    ngPrintf( const char *format, ...);
int    ngGetChar( void);
NGuint ngRandom( NGuint *seed);
#define NG_RAND_MAX     0xffffffffUL
int    ngATOI( const char *s);
long   ngATOL( const char *s);
NGuint ngATOH( const char *s);
char  *ngStrTok( char *s, const char *sep, char **savs);

#ifdef NG_RTOS
/* rtos interface */
#ifndef NG_RTOS_INLINE
int  ngOSGetErrno( void);
int  ngOSIsrEventDelete( NGOSievent *event);
int  ngOSIsrEventInit( NGOSievent *event);
int  ngOSIsrEventPost( NGOSievent *event);
int  ngOSIsrEventTaskPost( NGOSievent *event);
int  ngOSIsrEventWait( NGOSievent *event);
int  ngOSMutexDelete( NGOSmutex *mutex);
int  ngOSMutexInit( NGOSmutex *mutex);
int  ngOSMutexLock( NGOSmutex *mutex);
int  ngOSMutexUnlock( NGOSmutex *mutex);
int  ngOSSemClear( NGOSsem *sem);
int  ngOSSemDelete( NGOSsem *sem);
int  ngOSSemInit( NGOSsem *sem, int value);
int  ngOSSemPost( NGOSsem *sem);
int  ngOSSemTimedWait( NGOSsem *sem, u_long dly);
int  ngOSSemWait( NGOSsem *sem);
void ngOSSetErrno( int errno);
int  ngOSTaskCreate( NGOStask *th, void (*task)( void *data), void *data,
                        int prio, void *stack_ptr, int stack_size);
void ngOSTaskExit( void);
int  ngOSTaskDelete( NGOStask *th);
#endif
#endif

/* standart macros */
#define NG_ABS( a)      ((a)>=0?(a):-(a))
#define NG_MAX( a, b)   ((a)>(b)?(a):(b))
#define NG_MIN( a, b)   ((a)<(b)?(a):(b))

NG_END_DECLS

#endif


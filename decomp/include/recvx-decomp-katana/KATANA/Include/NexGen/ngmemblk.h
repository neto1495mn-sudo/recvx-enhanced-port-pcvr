/*****************************************************************************
 * $Id: ngmemblk.h,v 1.1 2000/08/21 09:33:34 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Memory Blocks Management
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
 * 21/08/2000 - Regis Feneon
 *****************************************************************************/

#ifndef __NG_NGMEMBLK_H_INCLUDED__
#define __NG_NGMEMBLK_H_INCLUDED__

#include <ngos.h>

NG_BEGIN_DECLS

/* data structures */
typedef struct {
    int     mem_total;  /* initial number of blocks */
    int     mem_nb;     /* number of free blocks */
    void   *mem_list;   /* pointer to next free block */
} NGmemblk;

/* prototypes */
int   ngMemBlkInit( NGmemblk *memp, void *addr, int blk_nb, int blk_size);
void *ngMemBlkAlloc( NGmemblk *memp);
void  ngMemBlkFree( NGmemblk *memp, void *blkp);

/* macros */
/* allocate memory block */
#define NG_MEMBLK_ALLOC( memp, ptr) { \
    (ptr) = (memp)->mem_list; \
    if( (ptr) != NULL) { \
        (memp)->mem_list = *((void **) (ptr)); \
        (memp)->mem_nb--; \
    } \
}
/* release memory block */
#define NG_MEMBLK_FREE( memp, ptr) { \
    *((void **) (ptr)) = (memp)->mem_list; \
    (memp)->mem_list = (ptr); \
    (memp)->mem_nb++; \
}

NG_END_DECLS

#endif


/*
 *  Shinobi Library
 *  Copyright (c) 1998 SEGA
 *  
 *       Memory Management
 *
 *  Module  : Library Header File
 *  File    : sg_Maloc.h
 *  Create  : 1998-03-25
 *  Modify  : 1998-09-16 malloc and syMalloc is unified.
 *  Version : 1.00
 *  Note    :
 */

#ifndef _SG_MALOC_H_
#define _SG_MALOC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void syMallocInit(Void *heap, Uint32 size);

Void syMallocFinish(Void);

#define syMalloc(__nbytes__) malloc(__nbytes__)

#define syFree(__ap__) free(__ap__)

Void syMallocStat(Uint32 *wholeFreeSize, Uint32 *biggestFreeBlockSize);

#ifdef __cplusplus
	   }
#endif /* __cplusplus */

#endif /* _SG_MALOC_H_ */

/******************************* end of file *******************************/

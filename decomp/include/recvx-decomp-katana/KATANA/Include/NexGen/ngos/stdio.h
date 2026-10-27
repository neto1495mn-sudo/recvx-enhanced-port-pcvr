/*****************************************************************************
 * $Id: stdio.h,v 1.2 2000/08/18 16:59:16 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Common IO Interface
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
 * 11/10/99 - Regis Feneon
 * 18/07/2000 -
 *  added NGiovec definition
 *****************************************************************************/

#ifndef __NG_STDIO_H_INCLUDED__
#define __NG_STDIO_H_INCLUDED__

NG_BEGIN_DECLS

/*
 * Type of control block
 */

#define NG_IOCB_NONE    0   /* free control block */
#define NG_IOCB_SOCK    1   /* socket */
#define NG_IOCB_DEV     2   /* character device */
#define NG_IOCB_BLK     3   /* block device */
#define NG_IOCB_FILE    4   /* file */

/*
 * file status flags for open() and fcntl()
 */

#define NG_O_RDONLY         0x0000
#define NG_O_WRONLY         0x0001
#define NG_O_RDWR           0x0002
#define NG_O_ACCMODE        0x0003
#define NG_O_NONBLOCK       0x0004
#define NG_O_APPEND         0x0008
#define NG_O_CREAT          0x0010
#define NG_O_TRUNC          0x0020
#define NG_O_EXCL           0x0040
#define NG_O_DSYNC          0x0080
#define NG_O_SYNC           0x0100

/*
 * IO state flags
 */

#define NG_IOSTATE_CANREAD          0x0001
#define NG_IOSTATE_CANWRITE         0x0002
#define NG_IOSTATE_CANREADOOB       0x0004
#define NG_IOSTATE_CANACCEPT        0x0008
#define NG_IOSTATE_ISCONNECTED      0x0010
#define NG_IOSTATE_ISDISCONNECTED   0x0020
#define NG_IOSTATE_HASERROR         0x0040

/*
 * Flags for read/write operations
 */

#define NG_IO_DONTWAIT   0x0001
#define NG_IO_NONBLOCK   NG_IO_DONTWAIT
#define NG_IO_WAITALL    0x0002
#define NG_IO_OOB        0x0004
#define NG_IO_PEEK       0x0008

/*
 * common ioctl commands (fcntl level)
 */

#define NG_IOCTL_FCNTL      0x0100
#define NG_FCNTL_GFDFLAGS   1
#define NG_FCNTL_SFDFLAGS   2
#define NG_FCNTL_GFLAGS     3
#define NG_FCNTL_SFLAGS     4
#define NG_FCNTL_GIOSTATE   5
#define NG_FCNTL_GNREAD     6

/* scatering/gathering */
typedef struct {
    void *iov_base;     /* pointer to data block */
    int   iov_len;      /* size of data block */
} NGiovec;

/* Structure used to set read/write timeouts */
typedef struct {
    long tv_sec;        /* seconds */
    long tv_usec;       /* microseconds */
} NGtimeval;

/* usefull memory copy routine */
void ngIovCpy( NGubyte *dst, NGiovec *iov, int iovcnt, int iov_off, int len);

NG_END_DECLS

#endif


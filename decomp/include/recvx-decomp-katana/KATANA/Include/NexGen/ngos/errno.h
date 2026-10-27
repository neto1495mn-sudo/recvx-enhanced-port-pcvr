/*****************************************************************************
 * $Id: errno.h,v 1.1.1.1 2000/07/26 09:53:23 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Error Codes
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
 * 29/09/98 - Regis Feneon
 * 04/06/99 -
 *  added standart UNIX codes
 *****************************************************************************/

#ifndef __NG_ERRNO_H_INCLUDED__
#define __NG_ERRNO_H_INCLUDED__

#define NG_EOK               0  /* no error */
#define NG_E2BIG            -1  /* argument list too long */
#define NG_EACCES           -2  /* permission denied */
#define NG_EADDRINUSE       -3  /* address already in use */
#define NG_EADDRNOTAVAIL    -4  /* address not available from this host */
#define NG_EAFNOSUPPORT     -5  /* address family not supported */
#define NG_EAGAIN           -6  /* resource temporarily unavailable */
#define NG_EALREADY         -7  /* connection not yet completed */
#define NG_EAUTH            -8  /* authentication error */
#define NG_EBADF            -9  /* bad descriptor */
#define NG_EBUSY            -10 /* resource busy */
#define NG_ECHILD           -11 /* no child process */
#define NG_ECONNABORTED     -12 /* connection aborted by user */
#define NG_ECONNREFUSED     -13 /* connection refused */
#define NG_ECONNRESET       -14 /* connection reseted by peer */
#define NG_EDEADLK          -15 /* resource deadlock avoided */
#define NG_EDESTADDRREQ     -16 /* destination address required */
#define NG_EDOM             -17 /* value out of range */
#define NG_EEXIST           -18 /* file exists */
#define NG_EFAULT           -19 /* bad address */
#define NG_EFBIG            -20 /* file too big */
#define NG_EHOSTDOWN        -21 /* host down */
#define NG_EHOSTUNREACH     -22 /* host unreachable */
#define NG_EIDRM            -23 /* identifier removed */
#define NG_EINPROGRESS      -24 /* operation in progress */
#define NG_EINTR            -25 /* interrupted function call */
#define NG_EINVAL           -26 /* invalid argument */
#define NG_EIO              -27 /* input/output error */
#define NG_EISCONN          -28 /* already connected */
#define NG_EISDIR           -29 /* is a directory */
#define NG_ELOOP            -30 /* too many levels of symbolic links */
#define NG_EMFILE           -31 /* out of descriptors */
#define NG_EMLINK           -32 /* too many links */
#define NG_EMSGSIZE         -33 /* message too big */
#define NG_ENEEDAUTH        -34 /* need authenticator */
#define NG_ENETDOWN         -35 /* network down */
#define NG_ENETRESET        -36 /* network dropped connection on reset */
#define NG_ENETUNREACH      -37 /* network unreachable */
#define NG_ENFILE           -38 /* too many open files in system */
#define NG_ENOBUFS          -39 /* out of resources */
#define NG_ENODEV           -40 /* device not found */
#define NG_ENOENT           -41 /* no such file or directory */
#define NG_ENOEXEC          -42 /* exec format error */
#define NG_ENOLCK           -43 /* no locks available */
#define NG_ENOMEM           -44 /* cannot allocate memory */
#define NG_ENOMSG           -45 /* no message of desired type */
#define NG_ENOPROTOOPT      -46 /* not a valid protocol option */
#define NG_ENOSPC           -47 /* not enougth space to create object */
#define NG_ENOSYS           -48 /* function not implemented */
#define NG_ENOTBLK          -49 /* block device required */
#define NG_ENOTCONN         -50 /* socket not connected */
#define NG_ENOTDIR          -51 /* not a directory */
#define NG_ENOTEMPTY        -52 /* directory not empty */
#define NG_ENOTSOCK         -53 /* not a socket descriptor */
#define NG_ENOTTY           -54 /* not a tty */
#define NG_ENXIO            -55 /* device not configured */
#define NG_EOPNOTSUPP       -56 /* operation not supported */
#define NG_EPERM            -57 /* operation not permitted */
#define NG_EPFNOSUPPORT     -58 /* protocol family not supported */
#define NG_EPIPE            -59 /* pipe broken */
#define NG_EPROTONOSUPPORT  -60 /* protocol not supported */
#define NG_EPROTOTYPE       -61 /* wrong protocol type */
#define NG_ERANGE           -62 /* result too large */
#define NG_EROFS            -63 /* read-only file system */
#define NG_ESHUTDOWN        -64 /* can't send after shutdown */
#define NG_ESOCKTNOSUPPORT  -65 /* socket type not supported */
#define NG_ESPIPE           -66 /* illegal seek */
#define NG_ESRCH            -67 /* no such process */
#define NG_ETIMEDOUT        -68 /* connection timed out */
#define NG_ETOOMANYREFS     -69 /* too many references */
#define NG_EWOULDBLOCK      NG_EAGAIN /* operation would block */
#define NG_EXDEV            -70 /* cross-device link */

#endif


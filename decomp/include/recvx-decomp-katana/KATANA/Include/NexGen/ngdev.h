/*****************************************************************************
 * $Id: ngdev.h,v 1.6 2000/08/31 16:50:05 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Character device management
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
 *    	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 21/06/2000 - Regis Feneon
 * 17/08/2000 -
 *  added ngDevioOutChar()/ngDevioInChar() prototypes
 * 22/08/2000 -
 *  new driver functions
 *  ngDevCntl() prototype
 * 16/01/2001 - Elliott Martin
 *  Fixed a couple of macro spelling mistakes
 *****************************************************************************/

#ifndef __NG_NGDEV_H_INCLUDED__
#define __NG_NGDEV_H_INCLUDED__

#include <ngos.h>
#include <ngos/queue.h>
#include <ngos/stdio.h>

NG_BEGIN_DECLS

/*****************************************************************************
 * Public
 *****************************************************************************/

/* input/output buffer */
typedef struct {
    NGubyte        *devbuf_ptr;     /* start of buffer */
    int             devbuf_len;     /* length of buffer */
    volatile int    devbuf_cc;      /* # of bytes in buffer */
    volatile int    devbuf_ptr_r;   /* read pointer */
    volatile int    devbuf_ptr_w;   /* write pointer */
} NGdevbuf;

/* new device structure */
typedef struct NGdev_S {
    struct NGdev_S *dev_next;       /* link */
    char           *dev_name;       /* interface name */
    int             dev_type;       /* type of interface: */
#define NG_DEVT_OTHER           0       /* unspecified device */
#define NG_DEVT_SERIAL          1       /* serial device */
    int             dev_flags;      /* device flags: */
#define NG_DEVF_UP              0x0001  /* device is up */
#define NG_DEVF_DEBUG           0x0004  /* turn on debugging */
#define NG_DEVF_RUNNING         0x0040  /* resources allocated */
#define NG_DEVF_OACTIVE         0x0400  /* Ouput in progress */
    int             dev_cflags;     /* control and status flags: */
/* serial device flags: */
#define NG_DEVCF_SER_LOCAL      0x0001  /* ignore modem status line */
#define NG_DEVCF_SER_DTR        0x0002  /* DTR line control */
#define NG_DEVCF_SER_RTS        0x0004  /* RTS line control */
#define NG_DEVCF_SER_CTS        0x0008  /* CTS line status (ro) */
#define NG_DEVCF_SER_DSR        0x0010  /* DSR line status (ro) */
#define NG_DEVCF_SER_RI         0x0020  /* RI line status (ro) */
#define NG_DEVCF_SER_CD         0x0040  /* CD line status (ro) */
#define NG_DEVCF_SER_MODEM_RO \
    (NG_DEVCF_SER_CTS|NG_DEVCF_SER_DSR|NG_DEVCF_SER_RI|NG_DEVCF_SER_CD)
                                        /* read-only modem status flags */
#define NG_DEVCF_SER_MODEM_RW \
    (NG_DEVCF_SER_DTR|NG_DEVCF_SER_RTS) /* read-write modem control flags */
#define NG_DEVCF_SER_PARITY     0x0380  /* parity control bits */
#define NG_DEVCF_SER_PARENB     0x0080  /* enable parity */
#define NG_DEVCF_SER_PARODD     0x0180  /* odd (1) or even (0) parity */
#define NG_DEVCF_SER_PARSTK     0x0200  /* mark parity if PARODD, else space parity */
#define NG_DEVCF_SER_PARMRK \
    (NG_DEVCF_SER_PARENB|NG_DEVCF_PAR_STK|NG_DEVCF_SER_PARODD)
                                        /* mark parity */
#define NG_DEVCF_SER_PARSPC \
    (NG_DEVCF_SER_PARENB|NG_DEVCF_PAR_STK)
                                        /* space parity */
#define NG_DEVCF_SER_PARDIS     0x0000  /* disable parity */
#define NG_DEVCF_SER_PAREVN     NG_DEVCF_SER_PARENB     /* even parity - Spelling fixed by Elliott Martin */
#define NG_DEVCF_SER_CSIZE      0x0c00  /* character size bits */
#define NG_DEVCF_SER_CS8        0x0000  /* 8 bits */
#define NG_DEVCF_SER_CS7        0x0400  /* 7 bits */
#define NG_DEVCF_SER_CS6        0x0800  /* 6 bits */
#define NG_DEVCF_SER_CS5        0x0c00  /* 5 bits */
#define NG_DEVCF_SER_CSTOPB     0x1000  /* 2 stop bits (1) or 1 stop bit (0) */
#define NG_DEVCF_SER_CSTOPB1    0x0000              /* 1 stop bit */
#define NG_DEVCF_SER_CSTOPB2    NG_DEVCF_SER_CSTOPB     /* 2 stop bits - Spelling fixed by Elliott Martin */
#define NG_DEVCF_SER_DATA \
    (NG_DEVCF_SER_PARITY|NG_DEVCF_SER_CSIZE|NG_DEVCF_SER_CSTOPB)
                                        /* data format */
#define NG_DEVCF_SER_NOHUPCL    0x2000  /* no hang-up on last close */
    int             dev_iflags;     /* input processing flags: */
#define NG_DEVIF_SFLOW          0x0001  /* set input soft flow control */
#define NG_DEVIF_HFLOW          0x0002  /* set input hard flow control */
#define NG_DEVIF_SPAGED         0x0004  /* input paged by soft */
#define NG_DEVIF_HPAGED         0x0008  /* input paged by hard */
    int             dev_oflags;     /* output processing flags: */
#define NG_DEVOF_SFLOW          0x0001  /* set output soft flow control */
#define NG_DEVOF_HFLOW          0x0002  /* set output hard flow control */
#define NG_DEVOF_SPAGED         0x0004  /* output paged by soft */
#define NG_DEVOF_HPAGED         0x0008  /* output paged by hard */
    NGubyte         dev_vstart;     /* start character */
    NGubyte         dev_vstop;      /* stop character */
    NGushort        dev_vzero;      /* unused */
    NGuint          dev_bps;        /* link speed (bit per seconds) */
/* driver data */
    char           *dev_devname;    /* driver (device) name */
    int             dev_devirno;    /* irq level */
    int             dev_devport;    /* io port address */
    void NGfar     *dev_devbase;    /* base address pointer */
    u_long          dev_devclk;     /* source clock frequency */
    int             dev_spec1;      /* driver specific variable 1 */
    int             dev_spec2;      /* driver specific variable 2 */
    void           *dev_data;       /* pointer to driver specific data */
/* driver functions */
    int           (*dev_open_f)( struct NGdev_S *devp);
    int           (*dev_close_f)( struct NGdev_S *devp);
    int           (*dev_cntl_f)( struct NGdev_S *devp, int cmd,
                                 int option, void *arg);
    void          (*dev_updown_f)( struct NGdev_S *devp);
    void          (*dev_sbaud_f)( struct NGdev_S *devp);
    void          (*dev_sdata_f)( struct NGdev_S *devp);
    void          (*dev_smodem_f)( struct NGdev_S *devp);
    void          (*dev_gmodem_f)( struct NGdev_S *devp);
    void          (*dev_ibuffer_f)( struct NGdev_S *devp);
    void          (*dev_obuffer_f)( struct NGdev_S *devp);
    volatile int    dev_isrflags;   /* interrupt flags: */
#define NG_DEVIEF_SIGNAL   0x0001      /* signal interrupt (RTOS only) */
#define NG_DEVIEF_IDATA    0x0002      /* input buffer has changed */
#define NG_DEVIEF_ODATA    0x0004      /* output buffer has changed */
#define NG_DEVIEF_MODEM    0x0008      /* modem lines have changed */
    NGdevbuf        dev_ibuf;       /* input buffer */
    NGdevbuf        dev_obuf;       /* output buffer */
    int             dev_ibuflo;     /* low-water mark */
    int             dev_ibufhi;     /* high-water mark */
    volatile u_long dev_ibytes;     /* # of bytes received */
    volatile u_long dev_ierrors;    /* # of input errors */
    volatile u_long dev_iqdrops;    /* # of input bytes dropped (buffer full) */
    volatile u_long dev_obytes;     /* # of bytes transmitted */
    NGnode          dev_iocb_openq; /* opened iocbs */
} NGdev;

/* Serial device options */
#define NG_DEVO_NAME        0x0001
#define NG_DEVO_FLAGS       0x0002
#define NG_DEVO_IRQ         0x0003
#define NG_DEVO_IOBASE      0x0004
#define NG_DEVO_MEMBASE     0x0005
#define NG_DEVO_SPEC1       0x0006
#define NG_DEVO_SPEC2       0x0007
#define NG_DEVO_DATA        0x0008
#define NG_DEVO_CFLAGS      0x0009
#define NG_DEVO_IFLAGS      0x000a
#define NG_DEVO_OFLAGS      0x000b
#define NG_DEVO_BAUDS       0x000c
#define NG_DEVO_CLKBASE     0x000d
/* new commands & options */
#define NG_DEVO_IBUF_PTR    0x0010  /* input buffer */
#define NG_DEVO_IBUF_LEN    0x0011  /* input buffer length */
#define NG_DEVO_OBUF_PTR    0x0012  /* output buffer */
#define NG_DEVO_OBUF_LEN    0x0013  /* output buffer length */
#define NG_DEVO_IBUF_GLEN   0x0016  /* get # of bytes in input buffer */
#define NG_DEVO_IBUF_GPTR   0x0017  /* get direct pointer to input buffer */
#define NG_DEVO_IBUF_UPDT   0x0018  /* data has been read in input buffer */
#define NG_DEVO_OBUF_GLEN   0x0019  /* get # of available bytes in output buffer */
#define NG_DEVO_OBUF_GPTR   0x0020  /* get direct pointer to output buffer */
#define NG_DEVO_OBUF_UPDT   0x0021  /* data has been written in output buffer */


/* device driver definition */
typedef struct NGdevdrv_S {
    char       *devd_name;       /* driver name */
    int         devd_type;       /* adaptator type */
    int         devd_flags;      /* interface flags (at startup) */
    int       (*devd_init_f)( NGdev *devp);
    int       (*devd_open_f)( NGdev *devp);
    int       (*devd_close_f)( NGdev *devp);
    int       (*devd_cntl_f)( NGdev *devp, int cmd, int option, void *arg);
    void      (*devd_updown_f)( NGdev *devp);
    void      (*devd_sbaud_f)( NGdev *devp);
    void      (*devd_sdata_f)( NGdev *devp);
    void      (*devd_smodem_f)( NGdev *devp);
    void      (*devd_gmodem_f)( NGdev *devp);
    void      (*devd_ibuffer_f)( NGdev *devp);
    void      (*devd_obuffer_f)( NGdev *devp);
} NGdevdrv;


/* generic device control function */
int ngDevCntl( NGdev *devp, int cld, int option, void *arg);


/*
 * Device Control Block Definition
 */

typedef struct NGdevcb_S {
    NGnode      dcb_node;           /* structure link */
    int         dcb_handle;         /* type of descriptor (NG_FD_DEV) */
    NGdev      *dcb_devp;           /* back pointer to device structure */
    int         dcb_flags;          /* open flags: see ng_stdio.h */
#ifdef NG_RTOS
    int         dcb_insleep;        /* # of tasks waiting on input */
    int         dcb_onsleep;        /* # of tasks waiting on output */
    void       *dcb_iosem;          /* semaphore used to block tasks */
#else
    int         dcb_sleep;
#define NG_DIO_SLEEP_READ       0x0001
#define NG_DIO_SLEEP_WRITE      0x0002
#define NG_DIO_SLEEP_EXCEPT     0x0004
#endif
    int         dcb_vmin;           /* minimum # of characters to read */
    int         dcb_vtimeo;         /* timeout on read operations */
    void      (*dcb_upcall_func)(   /* direct io upcall routine: */
        struct NGdevcb_S   *dcb,        /* control block */
        void               *data,       /* dcb_upcall_data */
        int                 flags);     /* flags below: */
#define NG_DIO_UPCALL_IDATA     0x0001     /* input buffer changed */
#define NG_DIO_UPCALL_ODATA     0x0002     /* output buffer changed */
#define NG_DIO_UPCALL_MODEM     0x0004     /* modem lines changed */
    void       *dcb_upcall_data;    /* data passed to upcall routine */
} NGdevcb;

/*
 * Device io user interface
 */

int  ngDevioOpen( NGdev *devp, int flags, NGdevcb **dcb);
int  ngDevioClose( NGdevcb *dcb);
int  ngDevioRead( NGdevcb *dcb, void *buf, int buflen, int flags);
int  ngDevioReadByte( NGdevcb *dcb, int flags);
int  ngDevioReadEx( NGdevcb *dcb, void *buf, int buflen, int min, u_long timeo);
int  ngDevioWrite( NGdevcb *dcb, void *buf, int buflen, int flags);
int  ngDevioIoctl( NGdevcb *dcb, int level, int cmd, void *arg, int *arglen);
int  ngDevioGetState( NGdevcb *dcb);
int  ngDevioGetNRead( NGdevcb *dcb);
int  ngDevioPrintf( NGdevcb *dcb, const char *fmt,...);
int  ngDevioVPrintf( NGdevcb *dcb, const char *fmt, NGva_list args);

/* device input/output routines */
void ngDevioOutChar( int c, void *data);
int  ngDevioInChar( void *data);

/*
 * Device level ioctl commands
 */

#define NG_IOCTL_DEVCTL         0x0101
#define NG_DEVCTL_SVMIN         1
#define NG_DEVCTL_GVMIN         2
#define NG_DEVCTL_SVTIMEO       3
#define NG_DEVCTL_GVTIMEO       4
#define NG_DEVCTL_SIFLAGS       5
#define NG_DEVCTL_GIFLAGS       6
#define NG_DEVCTL_SOFLAGS       7
#define NG_DEVCTL_GOFLAGS       8
#define NG_DEVCTL_SCFLAGS       9
#define NG_DEVCTL_GCFLAGS       10
#define NG_DEVCTL_SIOSPEED      11
#define NG_DEVCTL_GIOSPEED      12


/*****************************************************************************
 * Private
 *****************************************************************************/

/*
 * Global Data
 */

typedef struct {
    NGdev      *dev_list;       /* chained list of devices */
    NGqueue     iocb_freeq;     /* queue of free device control block */
} NGdeviodata;

extern NGdeviodata  ngDevioData;
#ifdef NG_RTOS
/* internal task */
void                ngDevioFunc( void *data);
extern NGOStask     ngDevioTask;
extern NGOSievent   ngDevioEvent;
#endif

/* internal io event processing */
void ngDevioIntr( NGdev *devp, int iflags);

/*
 * Internal io functions
 */

int ngIofDevOpen( NGdevcb **dcb, NGdev *devp, int flags,...);
int ngIofDevClose( NGdevcb *dcb);
int ngIofDevIoctl( NGdevcb *dcb, int level, int cmd, void *arg, int *arglen);
int ngIofDevRead( NGdevcb *dcb, void *buf, int buflen, int flags);
int ngIofDevWrite( NGdevcb *dcb, void *buf, int buflen, int flags);

/*
 * Device Control Block management routines
 */

NGdevcb  *ngDevcbAlloc( void);
void      ngDevcbFree( NGdevcb *dcb);
int       ngDevcbSleepOnInput( NGdevcb *dcb, u_long timeo);
int       ngDevcbSleepOnOutput( NGdevcb *dcb, u_long timeo);
#ifdef NG_RTOS
void      ngDevcbWakeUp( NGdevcb *dcb);
#define   ngDevcbWakeUpInput( dcb)  { if( (dcb)->dcb_insleep) ngDevcbWakeUp( dcb); }
#define   ngDevcbWakeUpOutput( dcb) { if( (dcb)->dcb_onsleep) ngDevcbWakeUp( dcb); }
#else
#define   ngDevcbWakeUp( dcb)       { (dcb)->dcb_sleep = 0; }
#define   ngDevcbWakeUpInput( dcb)  { (dcb)->dcb_sleep &= ~NG_DIO_SLEEP_READ; }
#define   ngDevcbWakeUpOutput( dcb) { (dcb)->dcb_sleep &= ~NG_DIO_SLEEP_WRITE; }
#endif

NG_END_DECLS

#endif


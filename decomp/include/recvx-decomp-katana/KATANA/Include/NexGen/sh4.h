/*****************************************************************************
 * $Id: sh4.h,v 1.2 2000/09/01 18:35:35 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Hitachi SH-4 Processors: SH7750/SH7091
 * Peripheral registers description
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
 * 13/03/2000 - Regis Feneon
 * 01/09/2000 -
 *  v1.2 update
 *****************************************************************************/

#ifndef __NG_SH4_H_INCLUDED__
#define __NG_SH4_H_INCLUDED__

#include <ngos.h>
#include <ngdev.h>

NG_BEGIN_DECLS

/* serial driver */
extern const NGdevdrv ngSerDrv_SH4SCIF;

/* standard input/output */
int ngStdIOSH4SCIFInit( u_long baud, u_long pphifreq);

NG_END_DECLS

#endif

#ifdef NG_SH4_REGS

#ifndef __NG_VTYPES__
#define __NG_VTYPES__
/* register data types */
typedef volatile unsigned char  NGvubyte;
typedef volatile unsigned short NGvushort;
typedef volatile unsigned int   NGvuint;
#endif

/*---------------------------------------------------------------------------*
 * CPG: Clock Pulse Generator                                                *
 *---------------------------------------------------------------------------*/

#define CPG_BASE    0xffc00000

#define FRQCR   (*(NGvushort *)(CPG_BASE+0x00)) /* Frequency control register */
#define STBCR   (*(NGvubyte *) (CPG_BASE+0x04)) /* Standby control register */
#define WTCNT_r (*(NGvubyte *) (CPG_BASE+0x08)) /* Watchdog timer counter (read) */
#define WTCNT_w (*(NGvushort *)(CPG_BASE+0x08)) /* Watchdog timer counter (write) */
#define WTCSR_r (*(NGvubyte *) (CPG_BASE+0x0c)) /* Watchdog timer control/status register (read) */
#define WTCSR_w (*(NGvushort *)(CPG_BASE+0x0c)) /* Watchdog timer control/status register (write) */
#define STBCR2  (*(NGvubyte *) (CPG_BASE+0x10)) /* Standby control register 2 */

#define FRQCR_CKOEN     0x0800  /* Clock output enable */
#define FRQCR_PLL1EN    0x0400  /* PLL circuit 1 enable */
#define FRQCR_PLL2EN    0x0200  /* PLL circuit 2 enable */
#define FRQCR_IFC       0x01c0  /* CPU clock freq division bits: */
#define FRQCR_IFC_1     0x0000  /*   x 1   */
#define FRQCR_IFC_2     0x0040  /*   x 1/2 */
#define FRQCR_IFC_3     0x0080  /*   x 1/3 */
#define FRQCR_IFC_4     0x00c0  /*   x 1/4 */
#define FRQCR_IFC_6     0x0100  /*   x 1/6 */
#define FRQCR_IFC_8     0x0140  /*   x 1/8 */
#define FRQCR_BFC       0x0038  /* Bus clock freq division ratio bits: */
#define FRQCR_BFC_1     0x0000  /*   x 1   */
#define FRQCR_BFC_2     0x0008  /*   x 1/2 */
#define FRQCR_BFC_3     0x0010  /*   x 1/3 */
#define FRQCR_BFC_4     0x0018  /*   x 1/4 */
#define FRQCR_BFC_6     0x0020  /*   x 1/6 */
#define FRQCR_BFC_8     0x0028  /*   x 1/8 */
#define FRQCR_PFC       0x0007  /* Peripheral clock freq division ratio bits: */
#define FRQCR_PFC_2     0x0000  /*   x 1/2 */
#define FRQCR_PFC_3     0x0001  /*   x 1/3 */
#define FRQCR_PFC_4     0x0002  /*   x 1/4 */
#define FRQCR_PFC_6     0x0003  /*   x 1/6 */
#define FRQCR_PFC_8     0x0004  /*   x 1/8 */

/*---------------------------------------------------------------------------*
 * INTC: Interrupt Controller                                                *
 *---------------------------------------------------------------------------*/

#define INTC_BASE   0xffd00000

/* external interrupts */
#define INTC_IRQ_NMI    0x1c0
#define INTC_IRQ_IRL0   0x200
#define INTC_IRQ_IRL1   0x220
#define INTC_IRQ_IRL2   0x240
#define INTC_IRQ_IRL3   0x260
#define INTC_IRQ_IRL4   0x280
#define INTC_IRQ_IRL5   0x2a0
#define INTC_IRQ_IRL6   0x2c0
#define INTC_IRQ_IRL7   0x2e0
#define INTC_IRQ_IRL8   0x300
#define INTC_IRQ_IRL9   0x320
#define INTC_IRQ_IRLA   0x340
#define INTC_IRQ_IRLB   0x360
#define INTC_IRQ_IRLC   0x380
#define INTC_IRQ_IRLD   0x3a0
#define INTC_IRQ_IRLE   0x3c0

#define ICR   (*(NGvushort *)(INTC_BASE+0x00)) /* Interrupt control register */
#define IPRA  (*(NGvushort *)(INTC_BASE+0x04)) /* Interrupt priority register A */
#define IPRB  (*(NGvushort *)(INTC_BASE+0x08)) /* Interrupt priority register B */
#define IPRC  (*(NGvushort *)(INTC_BASE+0x0c)) /* Interrupt priority register C */

#define ICR_NMIL        0x8000  /* NMI input level */
#define ICR_MAI         0x4000  /* NMI interrupt mask */
#define ICR_NMIB        0x0200  /* NMI block mode */
#define ICR_NMIE        0x0100  /* NMI edge select */
#define ICR_IRLM        0x0080  /* IRL pin mode */

#define IPRA_TMU0       0xf000  /* TMU0 interrupt priority level */
#define IPRA_TMU1       0x0f00  /* TMU1 interrupt priority level */
#define IPRA_TMU2       0x00f0  /* TMU2 interrupt priority level */
#define IPRA_RTC        0x000f  /* RTC interrupt priority level */

#define IPRB_WDT        0xf000  /* WDT interrupt priority level */
#define IPRB_REF        0x0f00  /* REF interrupt priority level */
#define IPRB_SCI1       0x00f0  /* SCI interrupt priority level */

#define IPRC_GPIO       0xf000  /* GPIO interrupt priority level */
#define IPRC_DMAC       0x0f00  /* DMAC interrupt priority level */
#define IPRC_SCIF       0x00f0  /* SCIF interrupt priority level */
#define IPRC_UDI        0x000f  /* Hitachi-UDI interrupt priority level */

/*---------------------------------------------------------------------------*
 * TMU: Timer Unit                                                           *
 *---------------------------------------------------------------------------*/

#define TMU_BASE    0xffd80000

#define TMU_IRQ_TUNI0   0x400
#define TMU_IRQ_TUNI1   0x420
#define TMU_IRQ_TUNI2   0x440
#define TMU_IRQ_TICPI2  0x460

#define TOCR    (*(NGvubyte *) (TMU_BASE+0x00)) /* Timer output control register */
#define TSTR    (*(NGvubyte *) (TMU_BASE+0x04)) /* Timer start register */
#define TCOR0   (*(NGvuint *)  (TMU_BASE+0x08)) /* Timer constant register 0 */
#define TCNT0   (*(NGvuint *)  (TMU_BASE+0x0c)) /* Timer counter 0 */
#define TCR0    (*(NGvushort *)(TMU_BASE+0x10)) /* Timer control register 0 */
#define TCOR1   (*(NGvuint *)  (TMU_BASE+0x14)) /* Timer constant register 1 */
#define TCNT1   (*(NGvuint *)  (TMU_BASE+0x18)) /* Timer counter 1 */
#define TCR1    (*(NGvushort *)(TMU_BASE+0x1c)) /* Timer control register 1 */
#define TCOR2   (*(NGvuint *)  (TMU_BASE+0x20)) /* Timer constant register 2 */
#define TCNT2   (*(NGvuint *)  (TMU_BASE+0x24)) /* Timer counter 2 */
#define TCR2    (*(NGvushort *)(TMU_BASE+0x28)) /* Timer control register 2 */
#define TCPR2   (*(NGvuint *)  (TMU_BASE+0x2c)) /* Input capture register */

#define TOCR_TCOE       0x01    /* Timer clock pin control */

#define TSTR_STR2       0x04    /* Counter start 2 */
#define TSTR_STR1       0x02    /* Counter start 1 */
#define TSTR_STR0       0x01    /* Counter start 0 */

#define TCR_ICPF        0x0200  /* Input capture interrupt flag (channel 2 only) */
#define TCR_UNF         0x0100  /* Underflow flag */
#define TCR_ICPE1       0x0080  /* Input capture control 1 (channel 2 only) */
#define TCR_ICPE0       0x0040  /* Input capture control 0 (channel 2 only) */
#define TCR_UNIE        0x0020  /* Underflow interrupt control */
#define TCR_CKEG1       0x0010  /* Clock edge 1 */
#define TCR_CKEG0       0x0008  /* Clock edge 0 */
#define TCR_TPSC        0x0007  /* Timer prescaler bits: */
#define TCR_TPSC_4      0x0000  /*   Pphi/4 */
#define TCR_TPSC_16     0x0001  /*   Pphi/16 */
#define TCR_TPSC_64     0x0002  /*   Pphi/64 */
#define TCR_TPSC_256    0x0003  /*   Pphi/256 */
#define TCR_TPSC_1024   0x0004  /*   Pphi/1024 */
#define TCR_TPSC_RTC    0x0006  /*   on-chip RTC output clock */
#define TCR_TPSC_EXT    0x0007  /*   external clock */

/*---------------------------------------------------------------------------*
 * SCI: Serial Communication Interface                                       *
 *---------------------------------------------------------------------------*/

#define SCI_BASE    0xffe00000

#define SCI_IRQ_ERI     0x4e0
#define SCI_IRQ_RXI     0x500
#define SCI_IRQ_TXI     0x520
#define SCI_IRQ_TEI     0x540

#define SCSMR1  (*(NGvubyte *) (SCI_BASE+0x00)) /* Serial mode register */
#define SCBRR1  (*(NGvubyte *) (SCI_BASE+0x04)) /* Bit rate register */
#define SCSCR1  (*(NGvubyte *) (SCI_BASE+0x08)) /* Serial control register */
#define SCTDR1  (*(NGvubyte *) (SCI_BASE+0x0c)) /* Transmit data register */
#define SCSSR1  (*(NGvubyte *) (SCI_BASE+0x10)) /* Serial status register */
#define SCRDR1  (*(NGvubyte *) (SCI_BASE+0x14)) /* Receive data register */
#define SCSCMR1 (*(NGvubyte *) (SCI_BASE+0x18)) /* Smart card mode register */
#define SCSPTR1 (*(NGvubyte *) (SCI_BASE+0x1c)) /* Serial port register */

#define SCSMR1_CA       0x80    /* Communication mode */
#define SCSMR1_CHR      0x40    /* Character length */
#define SCSMR1_PE       0x20    /* Parity enable */
#define SCSMR1_OE       0x10    /* Parity mode */
#define SCSMR1_STOP     0x08    /* Stop bit length */
#define SCSMR1_MP       0x04    /* Multiprocessor mode */
#define SCSMR1_CKS      0x03    /* Clock select bits: */
#define SCSMR1_CKS_1    0x00    /*   Pphi */
#define SCSMR1_CKS_4    0x01    /*   Pphi/4 */
#define SCSMR1_CKS_16   0x02    /*   Pphi/16 */
#define SCSMR1_CKS_64   0x03    /*   Pphi/64 */

#define SCSCR1_TIE      0x80    /* Transmit interrupt enable */
#define SCSCR1_RIE      0x40    /* Receive interrupt enable */
#define SCSCR1_TE       0x20    /* Transmit enable */
#define SCSCR1_RE       0x10    /* Receive enable */
#define SCSCR1_MPIE     0x08    /* Multiprocessor interrupt enable */
#define SCSCR1_TEIE     0x04    /* Transmit-end interrupt enable */
#define SCSCR1_CKE1     0x02    /* Clock enable 1 */
#define SCSCR1_CKE0     0x01    /* Clock enable 0 */

#define SCSSR1_TDRE     0x80    /* Transmit data register empty */
#define SCSSR1_RDRF     0x40    /* Receive data register full */
#define SCSSR1_ORER     0x20    /* Overrun error */
#define SCSSR1_FER      0x10    /* Framing error */
#define SCSSR1_PER      0x08    /* Parity error */
#define SCSSR1_TEND     0x04    /* Transmit end */
#define SCSSR1_MPB      0x02    /* Multiprocessor bit */
#define SCSSR1_MPBT     0x01    /* Multiprocessor bit transfert */

#define SCSPTR1_EIO     0x80    /* Error interrupt only */
#define SCSPTR1_SPB1IO  0x08    /* Serial port clock port IO */
#define SCSPTR1_SPB1DT  0x04    /* Serial port clock port data */
#define SCSPTR1_SPB0IO  0x02    /* Serial port break IO */
#define SCSPTR1_SPB0DT  0x01    /* Serial port break data */

/*---------------------------------------------------------------------------*
 * SCIF: Serial Communication Interface with FIFO                            *
 *---------------------------------------------------------------------------*/

#define SCIF_BASE   0xffe80000

#define SCIF_IRQ_ERI    0x700
#define SCIF_IRQ_RXI    0x720
#define SCIF_IRQ_BRI    0x740
#define SCIF_IRQ_TXI    0x760

#define SCSMR2  (*(NGvushort *)(SCIF_BASE+0x00)) /* Serial mode register */
#define SCBRR2  (*(NGvubyte *) (SCIF_BASE+0x04)) /* Bit rate register */
#define SCSCR2  (*(NGvushort *)(SCIF_BASE+0x08)) /* Serial control register */
#define SCFTDR2 (*(NGvubyte *) (SCIF_BASE+0x0c)) /* Transmit FIFO data register */
#define SCFSR2  (*(NGvushort *)(SCIF_BASE+0x10)) /* Serial status register */
#define SCFRDR2 (*(NGvubyte *) (SCIF_BASE+0x14)) /* Receive FIFO data register */
#define SCFCR2  (*(NGvushort *)(SCIF_BASE+0x18)) /* FIFO control register */
#define SCFDR2  (*(NGvushort *)(SCIF_BASE+0x1c)) /* FIFO data count register */
#define SCSPTR2 (*(NGvushort *)(SCIF_BASE+0x20)) /* Serial port register */
#define SCLSR2  (*(NGvushort *)(SCIF_BASE+0x24)) /* Line status register */

#define SCSMR2_CHR      0x0040  /* Character length */
#define SCSMR2_PE       0x0020  /* Parity enable */
#define SCSMR2_OE       0x0010  /* Parity mode */
#define SCSMR2_STOP     0x0008  /* Stop bit length */
#define SCSMR2_CKS      0x0003  /* Clock select bits: */
#define SCSMR2_CKS_1    0x0000  /*   Pphi */
#define SCSMR2_CKS_4    0x0001  /*   Pphi/4 */
#define SCSMR2_CKS_16   0x0002  /*   Pphi/16 */
#define SCSMR2_CKS_64   0x0003  /*   Pphi/64 */

#define SCSCR2_TIE      0x0080  /* Transmit interrupt enable */
#define SCSCR2_RIE      0x0040  /* Receive interrupt enable */
#define SCSCR2_TE       0x0020  /* Transmit enable */
#define SCSCR2_RE       0x0010  /* Receive enable */
#define SCSCR2_REIE     0x0008  /* Receive error interrupt enable */
#define SCSCR2_CKE1     0x0002  /* Clock enable 1 */

#define SCFSR2_PERN     0xf000  /* Number of parity errors */
#define SCFSR2_FERN     0x0f00  /* Number of framing errors */
#define SCFSR2_ER       0x0080  /* Receive error */
#define SCFSR2_TEND     0x0040  /* Transmit end */
#define SCFSR2_TDFE     0x0020  /* Transmit FIFO data empty */
#define SCFSR2_BRK      0x0010  /* Break detect */
#define SCFSR2_FER      0x0008  /* Framing error */
#define SCFSR2_PER      0x0004  /* Parity error */
#define SCFSR2_RDF      0x0002  /* Receive FIFO data full */
#define SCFSR2_DR       0x0001  /* Receive data ready */

#define SCFCR2_RTRG     0x00c0  /* Receive FIFO data number trigger bits: */
#define SCFCR2_RTRG_1   0x0000  /*   1 byte */
#define SCFCR2_RTRG_4   0x0040  /*   4 bytes */
#define SCFCR2_RTRG_8   0x0080  /*   8 bytes */
#define SCFCR2_RTRG_14  0x00c0  /*   14 bytes */
#define SCFCR2_TTRG     0x0030  /* Transmit FIFO data number trigger bits: */
#define SCFCR2_TTRG_8   0x0000  /*   8 bytes */
#define SCFCR2_TTRG_4   0x0010  /*   4 bytes */
#define SCFCR2_TTRG_2   0x0020  /*   2 bytes */
#define SCFCR2_TTRG_1   0x0030  /*   1 byte */
#define SCFCR2_MCE      0x0008  /* Modem control enable */
#define SCFCR2_TFRST    0x0004  /* Transmit FIFO data register reset */
#define SCFCR2_RFRST    0x0002  /* Receive FIFO data register reset */
#define SCFCR2_LOOP     0x0001  /* Loopback test */

#define SCFDR2_TN       0xff00  /* Number of untransmitted data bytes */
#define SCFDR2_RN       0x00ff  /* Number of receive data bytes */

#define SCSPTR2_RTSIO   0x0080  /* Serial port RTS port IO */
#define SCSPTR2_RTSDT   0x0040  /* Serial port RTS port data */
#define SCSPTR2_CTSIO   0x0020  /* Serial port CTS port IO */
#define SCSPTR2_CTSDT   0x0010  /* Serial port CTS port data */
#define SCSPTR2_SPB2IO  0x0002  /* Serial port break IO */
#define SCSPTR2_SPB2DT  0x0001  /* Serial port beak data */

#define SCLSR2_ORER     0x0001  /* Overrun error */

#endif /* NG_SH4_REGS */


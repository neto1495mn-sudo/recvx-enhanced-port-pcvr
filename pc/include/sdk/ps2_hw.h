/*
 * PS2 hardware constants and register-packing macros (GS, GIF, VIF, DMA) for
 * the PC build, written from the GS/EE hardware register layouts. The values
 * feed the GIF/VIF packet interpreter in pc/src/ps2/, which consumes the packets
 * the original platform code builds.
 */
#pragma once
#include <stdint.h>

#define PS2_U64(x) ((uint64_t)(x))

/* ---------------- GIF ---------------- */
#define SCE_GIF_PACKED      0
#define SCE_GIF_REGLIST     1
#define SCE_GIF_IMAGE       2
#define SCE_GIF_PACKED_AD   0x0e
#define SCE_GIF_SET_TAG(nloop, eop, pre, prim, flg, nreg) \
    (PS2_U64(nloop) | (PS2_U64(eop) << 15) | (PS2_U64(pre) << 46) | (PS2_U64(prim) << 47) | \
     (PS2_U64(flg) << 58) | (PS2_U64(nreg) << 60))

/* ---------------- GS registers ---------------- */
#define SCE_GS_PRIM        0x00
#define SCE_GS_RGBAQ       0x01
#define SCE_GS_ST          0x02
#define SCE_GS_UV          0x03
#define SCE_GS_XYZF2       0x04
#define SCE_GS_XYZ2        0x05
#define SCE_GS_TEX0_1      0x06
#define SCE_GS_TEX0_2      0x07
#define SCE_GS_CLAMP_1     0x08
#define SCE_GS_CLAMP_2     0x09
#define SCE_GS_FOG         0x0a
#define SCE_GS_XYZF3       0x0c
#define SCE_GS_XYZ3        0x0d
#define SCE_GS_TEX1_1      0x14
#define SCE_GS_TEX1_2      0x15
#define SCE_GS_TEX2_1      0x16
#define SCE_GS_TEX2_2      0x17
#define SCE_GS_XYOFFSET_1  0x18
#define SCE_GS_XYOFFSET_2  0x19
#define SCE_GS_PRMODECONT  0x1a
#define SCE_GS_PRMODE      0x1b
#define SCE_GS_TEXCLUT     0x1c
#define SCE_GS_SCANMSK     0x22
#define SCE_GS_MIPTBP1_1   0x34
#define SCE_GS_MIPTBP1_2   0x35
#define SCE_GS_MIPTBP2_1   0x36
#define SCE_GS_MIPTBP2_2   0x37
#define SCE_GS_TEXA        0x3b
#define SCE_GS_FOGCOL      0x3d
#define SCE_GS_TEXFLUSH    0x3f
#define SCE_GS_SCISSOR_1   0x40
#define SCE_GS_SCISSOR_2   0x41
#define SCE_GS_ALPHA_1     0x42
#define SCE_GS_ALPHA_2     0x43
#define SCE_GS_DIMX        0x44
#define SCE_GS_DTHE        0x45
#define SCE_GS_COLCLAMP    0x46
#define SCE_GS_TEST_1      0x47
#define SCE_GS_TEST_2      0x48
#define SCE_GS_PABE        0x49
#define SCE_GS_FBA_1       0x4a
#define SCE_GS_FBA_2       0x4b
#define SCE_GS_FRAME_1     0x4c
#define SCE_GS_FRAME_2     0x4d
#define SCE_GS_ZBUF_1      0x4e
#define SCE_GS_ZBUF_2      0x4f
#define SCE_GS_BITBLTBUF   0x50
#define SCE_GS_TRXPOS      0x51
#define SCE_GS_TRXREG      0x52
#define SCE_GS_TRXDIR      0x53
#define SCE_GS_HWREG       0x54
#define SCE_GS_SIGNAL      0x60
#define SCE_GS_FINISH      0x61
#define SCE_GS_LABEL       0x62

/* pixel storage modes */
#define SCE_GS_PSMCT32   0x00
#define SCE_GS_PSMCT24   0x01
#define SCE_GS_PSMCT16   0x02
#define SCE_GS_PSMCT16S  0x0a
#define SCE_GS_PSMT8     0x13
#define SCE_GS_PSMT4     0x14
#define SCE_GS_PSMT8H    0x1b
#define SCE_GS_PSMT4HL   0x24
#define SCE_GS_PSMT4HH   0x2c
#define SCE_GS_PSMZ32    0x30
#define SCE_GS_PSMZ24    0x31
#define SCE_GS_PSMZ16    0x32
#define SCE_GS_PSMZ16S   0x3a

/* primitive types */
#define SCE_GS_PRIM_POINT     0
#define SCE_GS_PRIM_LINE      1
#define SCE_GS_PRIM_LINESTRIP 2
#define SCE_GS_PRIM_TRI       3
#define SCE_GS_PRIM_TRISTRIP  4
#define SCE_GS_PRIM_TRIFAN    5
#define SCE_GS_PRIM_SPRITE    6

/* misc field values (some also defined by the decomp's types.h) */
#ifndef SCE_GS_FALSE
#define SCE_GS_FALSE (0)
#endif
#ifndef SCE_GS_TRUE
#define SCE_GS_TRUE (1)
#endif
#define SCE_GS_MODULATE   0
#define SCE_GS_DECAL      1
#define SCE_GS_HIGHLIGHT  2
#define SCE_GS_HIGHLIGHT2 3
#define SCE_GS_NEAREST    0
#define SCE_GS_LINEAR     1
#define SCE_GS_ALPHA_ZERO 2
#define SCE_GS_ALPHA_AS         (0)   /* spelled as in the decomp's types.h: no redefinition warning */
#define SCE_GS_ALPHA_AD         (1)
#define SCE_GS_ALPHA_ALWAYS   1
#define SCE_GS_ALPHA_LESS     2
#define SCE_GS_ALPHA_LEQUAL   3
#define SCE_GS_ALPHA_EQUAL    4
#define SCE_GS_ALPHA_NOTEQUAL 7
#define SCE_GS_AFAIL_ZB_ONLY  2
#define SCE_GS_AFAIL_RGB_ONLY 3
#define SCE_GS_DEPTH_NEVER    0
#define SCE_GS_DEPTH_GREATER  3
#define SCE_GS_ZNEVER   0
#define SCE_GS_ZALWAYS  1
#define SCE_GS_ZGEQUAL  2
#define SCE_GS_ZGREATER 3
#define SCE_GS_NOINTERLACE 0
#define SCE_GS_INTERLACE   1
#define SCE_GS_FIELD       1
#define SCE_GS_FRAME       0
#define SCE_GS_NTSC        2
#define SCE_GS_PAL         3

/* register packing */
#define SCE_GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
    (PS2_U64(prim) | (PS2_U64(iip) << 3) | (PS2_U64(tme) << 4) | (PS2_U64(fge) << 5) | (PS2_U64(abe) << 6) | \
     (PS2_U64(aa1) << 7) | (PS2_U64(fst) << 8) | (PS2_U64(ctxt) << 9) | (PS2_U64(fix) << 10))
#define SCE_GS_SET_RGBAQ(r, g, b, a, q) \
    (PS2_U64(r) | (PS2_U64(g) << 8) | (PS2_U64(b) << 16) | (PS2_U64(a) << 24) | (PS2_U64(q) << 32))
#define SCE_GS_SET_ST(s, t)  (PS2_U64(s) | (PS2_U64(t) << 32))
#define SCE_GS_SET_UV(u, v)  (PS2_U64(u) | (PS2_U64(v) << 16))
#define SCE_GS_SET_XYZF(x, y, z, f) (PS2_U64(x) | (PS2_U64(y) << 16) | ((PS2_U64(z) & 0xffffff) << 32) | (PS2_U64(f) << 56))
#define SCE_GS_SET_XYZ(x, y, z)     (PS2_U64(x) | (PS2_U64(y) << 16) | (PS2_U64(z) << 32))
#define SCE_GS_SET_XYZF2 SCE_GS_SET_XYZF
#define SCE_GS_SET_XYZF3 SCE_GS_SET_XYZF
#define SCE_GS_SET_XYZ2  SCE_GS_SET_XYZ
#define SCE_GS_SET_XYZ3  SCE_GS_SET_XYZ
#define SCE_GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    (PS2_U64(tbp) | (PS2_U64(tbw) << 14) | (PS2_U64(psm) << 20) | (PS2_U64(tw) << 26) | (PS2_U64(th) << 30) | \
     (PS2_U64(tcc) << 34) | (PS2_U64(tfx) << 35) | (PS2_U64(cbp) << 37) | (PS2_U64(cpsm) << 51) | \
     (PS2_U64(csm) << 55) | (PS2_U64(csa) << 56) | (PS2_U64(cld) << 61))
#define SCE_GS_SET_TEX0_1 SCE_GS_SET_TEX0
#define SCE_GS_SET_TEX0_2 SCE_GS_SET_TEX0
#define SCE_GS_SET_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
    (PS2_U64(lcm) | (PS2_U64(mxl) << 2) | (PS2_U64(mmag) << 5) | (PS2_U64(mmin) << 6) | (PS2_U64(mtba) << 9) | \
     (PS2_U64(l) << 19) | (PS2_U64(k) << 32))
#define SCE_GS_SET_TEX1_1 SCE_GS_SET_TEX1
#define SCE_GS_SET_TEX1_2 SCE_GS_SET_TEX1
#define SCE_GS_SET_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
    (PS2_U64(wms) | (PS2_U64(wmt) << 2) | (PS2_U64(minu) << 4) | (PS2_U64(maxu) << 14) | (PS2_U64(minv) << 24) | (PS2_U64(maxv) << 34))
#define SCE_GS_SET_CLAMP_1 SCE_GS_SET_CLAMP
#define SCE_GS_SET_CLAMP_2 SCE_GS_SET_CLAMP
#define SCE_GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
    (PS2_U64(ate) | (PS2_U64(atst) << 1) | (PS2_U64(aref) << 4) | (PS2_U64(afail) << 12) | (PS2_U64(date) << 14) | \
     (PS2_U64(datm) << 15) | (PS2_U64(zte) << 16) | (PS2_U64(ztst) << 17))
#define SCE_GS_SET_TEST_1 SCE_GS_SET_TEST
#define SCE_GS_SET_TEST_2 SCE_GS_SET_TEST
#define SCE_GS_SET_ALPHA(a, b, c, d, fix) \
    (PS2_U64(a) | (PS2_U64(b) << 2) | (PS2_U64(c) << 4) | (PS2_U64(d) << 6) | (PS2_U64(fix) << 32))
#define SCE_GS_SET_ALPHA_1 SCE_GS_SET_ALPHA
#define SCE_GS_SET_ALPHA_2 SCE_GS_SET_ALPHA
#define SCE_GS_SET_FRAME(fbp, fbw, psm, fbmsk) (PS2_U64(fbp) | (PS2_U64(fbw) << 16) | (PS2_U64(psm) << 24) | (PS2_U64(fbmsk) << 32))
#define SCE_GS_SET_FRAME_1 SCE_GS_SET_FRAME
#define SCE_GS_SET_FRAME_2 SCE_GS_SET_FRAME
#define SCE_GS_SET_ZBUF(zbp, psm, zmsk) (PS2_U64(zbp) | (PS2_U64(psm) << 24) | (PS2_U64(zmsk) << 32))
#define SCE_GS_SET_ZBUF_1 SCE_GS_SET_ZBUF
#define SCE_GS_SET_ZBUF_2 SCE_GS_SET_ZBUF
#define SCE_GS_SET_SCISSOR(x0, x1, y0, y1) (PS2_U64(x0) | (PS2_U64(x1) << 16) | (PS2_U64(y0) << 32) | (PS2_U64(y1) << 48))
#define SCE_GS_SET_SCISSOR_1 SCE_GS_SET_SCISSOR
#define SCE_GS_SET_SCISSOR_2 SCE_GS_SET_SCISSOR
#define SCE_GS_SET_XYOFFSET(x, y) (PS2_U64(x) | (PS2_U64(y) << 32))
#define SCE_GS_SET_XYOFFSET_1 SCE_GS_SET_XYOFFSET
#define SCE_GS_SET_XYOFFSET_2 SCE_GS_SET_XYOFFSET
#define SCE_GS_SET_PABE(pabe) PS2_U64(pabe)
#define SCE_GS_SET_FBA(fba) PS2_U64(fba)
#define SCE_GS_SET_FBA_1 SCE_GS_SET_FBA
#define SCE_GS_SET_FBA_2 SCE_GS_SET_FBA
#define SCE_GS_SET_TEXA(ta0, aem, ta1) (PS2_U64(ta0) | (PS2_U64(aem) << 15) | (PS2_U64(ta1) << 32))
#define SCE_GS_SET_FOGCOL(r, g, b) (PS2_U64(r) | (PS2_U64(g) << 8) | (PS2_U64(b) << 16))
#define SCE_GS_SET_FOG(f) (PS2_U64(f) << 56)
#define SCE_GS_SET_TEXFLUSH(x) PS2_U64(x)
#define SCE_GS_SET_COLCLAMP(c) PS2_U64(c)
#define SCE_GS_SET_DTHE(d) PS2_U64(d)
#define SCE_GS_SET_PRMODECONT(ac) PS2_U64(ac)
#define SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
    (PS2_U64(sbp) | (PS2_U64(sbw) << 16) | (PS2_U64(spsm) << 24) | (PS2_U64(dbp) << 32) | (PS2_U64(dbw) << 48) | (PS2_U64(dpsm) << 56))
#define SCE_GS_SET_TRXPOS(ssax, ssay, dsax, dsay, dir) \
    (PS2_U64(ssax) | (PS2_U64(ssay) << 16) | (PS2_U64(dsax) << 32) | (PS2_U64(dsay) << 48) | (PS2_U64(dir) << 59))
#define SCE_GS_SET_TRXREG(rrw, rrh) (PS2_U64(rrw) | (PS2_U64(rrh) << 32))
#define SCE_GS_SET_TRXDIR(xdr) PS2_U64(xdr)

/* ---------------- VIF ---------------- */
#define SCE_VIF1_SET_NOP(irq)            ((uint32_t)(irq) << 31)
#define SCE_VIF1_SET_OFFSET(off, irq)    ((uint32_t)(off) | (0x02u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_BASE(base, irq)     ((uint32_t)(base) | (0x03u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_ITOP(itop, irq)     ((uint32_t)(itop) | (0x04u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_STCYCL(wl, cl, irq) ((uint32_t)(cl) | ((uint32_t)(wl) << 8) | (0x01u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_FLUSH(irq)          ((0x11u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_FLUSHA(irq)         ((0x13u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_MSCAL(addr, irq)    ((uint32_t)(addr) | (0x14u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_MSCNT(irq)          ((0x17u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_DIRECT(cnt, irq)    ((uint32_t)(cnt) | (0x50u << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF1_SET_UNPACK(addr, num, cmd, irq) \
    ((uint32_t)(addr) | ((uint32_t)(num) << 16) | ((uint32_t)(0x60 | (cmd)) << 24) | ((uint32_t)(irq) << 31))
#define SCE_VIF0_SET_NOP    SCE_VIF1_SET_NOP
#define SCE_VIF0_SET_MSCAL  SCE_VIF1_SET_MSCAL
#define SCE_VIF0_SET_UNPACK SCE_VIF1_SET_UNPACK

/* ---------------- DMA channels (used as identifiers, never dereferenced on PC) ---------------- */
#define PS2_HWREG(a) ((volatile uint32_t*)(uintptr_t)(a))
#define D0_CHCR PS2_HWREG(0x10008000)
#define D1_CHCR PS2_HWREG(0x10009000)
#define D1_MADR PS2_HWREG(0x10009010)
#define D1_QWC  PS2_HWREG(0x10009020)
#define D1_TADR PS2_HWREG(0x10009030)
#define D2_CHCR PS2_HWREG(0x1000a000)
#define D2_MADR PS2_HWREG(0x1000a010)
#define D2_QWC  PS2_HWREG(0x1000a020)
#define D2_TADR PS2_HWREG(0x1000a030)
#define D3_CHCR PS2_HWREG(0x1000b000)
#define D3_MADR PS2_HWREG(0x1000b010)
#define D3_QWC  PS2_HWREG(0x1000b020)
#define D4_CHCR PS2_HWREG(0x1000b400)
#define D4_MADR PS2_HWREG(0x1000b410)
#define D4_QWC  PS2_HWREG(0x1000b420)
#define D4_TADR PS2_HWREG(0x1000b430)
#define D8_CHCR PS2_HWREG(0x1000d000)
#define D9_CHCR PS2_HWREG(0x1000d400)

/* ---------------- other EE registers: backed by a fake register file on PC ---------------- */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t t0_mode, t0_count, t0_comp, t0_hold;
    uint32_t d_ctrl, d_stat, d_pcr;
    uint32_t vif1_stat, gif_stat;
    uint32_t ipu_ctrl, ipu_bp;
    uint32_t dma_tadr[10], dma_madr[10], dma_qwc[10];
} Ps2FakeRegs;
extern Ps2FakeRegs ps2_regs;
void ps2_dma_put_chcr(int ch, uint32_t value);
void ps2_wait_vblank(void);

#define T0_MODE  (&ps2_regs.t0_mode)
#define T0_COUNT (&ps2_regs.t0_count)
#define DGET_D_PCR()      (ps2_regs.d_pcr)
#define DPUT_D_PCR(v)     (ps2_regs.d_pcr = (uint32_t)(v))
#define DGET_D_STAT()     (ps2_regs.d_stat)
#define DPUT_D_STAT(v)    (ps2_regs.d_stat = (uint32_t)(v))
#define DGET_VIF1_STAT()  (ps2_regs.vif1_stat)
#define DGET_GIF_STAT()   (ps2_regs.gif_stat)
#define DGET_IPU_CTRL()   (ps2_regs.ipu_ctrl)
#define DGET_IPU_BP()     (ps2_regs.ipu_bp)
#define DPUT_D2_TADR(v)   (ps2_regs.dma_tadr[2] = (uint32_t)(uintptr_t)(v))
#define DPUT_D2_CHCR(v)   ps2_dma_put_chcr(2, (uint32_t)(v))

/* libdma: the channel pointer selects the destination (VIF1 / GIF / ...) */
struct _sceDmaChan;
void sceDmaReset(int mode);
int  sceDmaSend(void* chan, void* tag);
int  sceDmaSendN(void* chan, void* addr, int qwc);
int  sceDmaSync(void* chan, int mode, int timeout);
void* sceDmaGetChan(int id);
#ifdef __cplusplus
}
#endif

/*
 * VIF1: unpacks data into VU1 memory, starts VU1 microprograms, forwards DIRECT
 * data to the GIF (PATH2). VU1 programs come from the decomp's microcode listing
 * (vu1_microcode.inc); MPG uploads are therefore accepted but not needed.
 */
#include "ps2_platform.h"
#include "vu0.h"
#include "../gs/gs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static struct {
    uint32_t cl, wl;          /* STCYCL */
    uint32_t mode;            /* STMOD */
    uint32_t mask;            /* STMASK */
    uint32_t row[4], col[4];  /* STROW / STCOL */
    uint32_t base, ofst, tops, top, itops, itop, dbf;
    /* partial command state across DMA chunks */
    uint32_t cmd;             /* current multi-word command, 0 if none */
    uint32_t code;
    uint32_t remaining;       /* words still expected */
    uint32_t addr, num_done;  /* UNPACK progress */
    uint32_t cycle;           /* write cycle counter within CL/WL */
    uint32_t tmp[4]; int tmp_n;
    uint32_t mpg_addr;
} vif;

static void vu1_start(uint32_t addr)
{
    static int dbg = -1, n;
    if (dbg < 0) dbg = getenv("RECVX_DUMP_VU1") ? atoi(getenv("RECVX_DUMP_VU1")) : 0;
    if (dbg && ++n == dbg) {
        fprintf(stderr, "VU1 MSCAL %u: TOPS=%u BASE=%u OFST=%u DBF=%u\n", addr, vif.tops, vif.base, vif.ofst, vif.dbf);
        for (int q = 884; q <= 898; q++) fprintf(stderr, "  mem[%d] = %10.4f %10.4f %10.4f %10.4f  (%08x %08x %08x %08x)\n", q,
            g_vu1.mem[q].f[0], g_vu1.mem[q].f[1], g_vu1.mem[q].f[2], g_vu1.mem[q].f[3], g_vu1.mem[q].u[0], g_vu1.mem[q].u[1], g_vu1.mem[q].u[2], g_vu1.mem[q].u[3]);
        for (int q = 0; q < 9; q++) { uint32_t a = (vif.tops + q) & 1023; fprintf(stderr, "  top+%d = %10.4f %10.4f %10.4f %10.4f  (%08x %08x %08x %08x)\n", q,
            g_vu1.mem[a].f[0], g_vu1.mem[a].f[1], g_vu1.mem[a].f[2], g_vu1.mem[a].f[3], g_vu1.mem[a].u[0], g_vu1.mem[a].u[1], g_vu1.mem[a].u[2], g_vu1.mem[a].u[3]); }
    }
    vif.top = vif.tops; vif.itop = vif.itops;
    g_vu1.top = vif.top; g_vu1.itop = vif.itop;
    vu1_callms((int)addr);
    vif.dbf ^= 1;
    vif.tops = vif.base + (vif.dbf ? vif.ofst : 0);
}

void vu1_xgkick(const VuVec* mem, uint32_t qw, uint32_t mask)
{
    /* VR: what the emulated VU1 programs draw is 3D projected by them (Z = -b * Iz - a): marked
       so the renderer can re-project it per eye (gs_gl.c push); only while stereo is on */
    extern int xr_stereo_ready(void);
    extern void gs_write_reg(int reg, uint64_t v);
    int vr_mark = xr_stereo_ready();
    if (vr_mark) gs_write_reg(0x7E, 1);
    /* walk GIF tags in VU1 memory until EOP */
    for (int guard = 0; guard < 1024; guard++) {
        const uint32_t* tag = mem[qw & mask].u;
        uint32_t nloop = tag[0] & 0x7FFF, eop = (tag[0] >> 15) & 1, flg = (tag[1] >> 26) & 3, nreg = (tag[1] >> 28) & 0xF;
        uint32_t qwc;
        if (nreg == 0) nreg = 16;
        qwc = flg == 0 ? nloop * nreg : flg == 1 ? (nloop * nreg + 1) / 2 : nloop;
        if ((qw & mask) + 1 + qwc > mask + 1) {
            /* wraps around the end of VU memory: copy */
            static uint32_t tmp[1024 * 4];
            for (uint32_t i = 0; i <= qwc; i++) memcpy(&tmp[i * 4], mem[(qw + i) & mask].u, 16);
            gs_gif_transfer(1, tmp, qwc + 1);
        } else {
            gs_gif_transfer(1, tag, qwc + 1);
        }
        qw += qwc + 1;
        if (eop) break;
    }
    if (vr_mark) gs_write_reg(0x7E, 0);
}

/* ---- UNPACK ---- */

static int unpack_bits(uint32_t vnvl)
{
    static const int bits[16] = { 32, 16, 8, 0, 64, 32, 16, 0, 96, 48, 24, 0, 128, 64, 32, 16 };
    return bits[vnvl & 15];
}

static void unpack_element(uint32_t vnvl, int usn, const uint8_t* src, uint32_t out[4])
{
    int vn = (vnvl >> 2) & 3, vl = vnvl & 3;
    int n = vn + 1;
    if (vnvl == 0x0F) { /* V4-5: RGBA 5551 */
        uint16_t v = (uint16_t)(src[0] | (src[1] << 8));
        out[0] = (v & 0x1F) << 3; out[1] = ((v >> 5) & 0x1F) << 3; out[2] = ((v >> 10) & 0x1F) << 3; out[3] = (v >> 15) << 7;
        return;
    }
    for (int i = 0; i < 4; i++) {
        int k = (vn == 0) ? 0 : i;       /* S-xx broadcast */
        if (vn != 0 && i >= n) { out[i] = (i == 3 && vn == 2) ? 0 : out[i]; continue; }
        switch (vl) {
        case 0: memcpy(&out[i], src + k * 4, 4); break;
        case 1: { uint16_t h; memcpy(&h, src + k * 2, 2); out[i] = usn ? h : (uint32_t)(int32_t)(int16_t)h; break; }
        case 2: { uint8_t b = src[k]; out[i] = usn ? b : (uint32_t)(int32_t)(int8_t)b; break; }
        }
    }
}

static void unpack_write(uint32_t qwaddr, const uint32_t in[4], int vn)
{
    VuVec* dst = &g_vu1.mem[qwaddr & g_vu1.mem_mask];
    for (int i = 0; i < 4; i++) {
        uint32_t m = (vif.mask >> (((vif.cycle > 3 ? 3 : vif.cycle) * 8) + i * 2)) & 3;
        uint32_t v = in[i];
        int present = vn < 0 ? 0 : (vn == 0 || i <= vn);  /* vn < 0: filling write */
        if (!(vif.code & 0x10000000)) m = 0; /* mask bit (m) in the UNPACK code */
        switch (m) {
        case 0:
            if (!present) continue;  /* fields not provided keep their value */
            if (vif.mode == 1) v += vif.row[i];
            else if (vif.mode == 2) { v += vif.row[i]; vif.row[i] = v; }
            dst->u[i] = v;
            break;
        case 1: dst->u[i] = vif.row[i]; break;
        case 2: dst->u[i] = vif.col[vif.cycle > 3 ? 3 : vif.cycle]; break;
        case 3: break; /* write protected */
        }
    }
}

/* returns number of bytes consumed from p (of `avail`) */
static uint32_t do_unpack(const uint8_t* p, uint32_t avail)
{
    uint32_t vnvl = (vif.code >> 24) & 0xF;
    int usn = (vif.code >> 14) & 1;
    int vn = (int)((vnvl >> 2) & 3);
    uint32_t esize = (uint32_t)unpack_bits(vnvl) / 8;
    uint32_t num = (vif.code >> 16) & 0xFF;
    uint32_t used = 0;
    if (num == 0) num = 256;
    if (vnvl == 0x0F) esize = 2;
    while (vif.num_done < num) {
        uint32_t qa;
        uint32_t v[4] = { 0, 0, 0, 0 };
        if (vif.cl >= vif.wl) {
            /* skipping write: WL written, then CL-WL skipped */
            qa = vif.addr + (vif.num_done / vif.wl) * vif.cl + (vif.num_done % vif.wl);
        } else {
            qa = vif.addr + vif.num_done;
        }
        if (vif.cl < vif.wl && (vif.num_done % vif.wl) >= vif.cl) {
            /* filling write: no input consumed */
            uint32_t z[4] = { 0, 0, 0, 0 };
            vif.cycle = vif.num_done % vif.wl;
            unpack_write(qa, z, -1);
        } else {
            if (used + esize > avail) break;
            unpack_element(vnvl, usn, p + used, v);
            vif.cycle = vif.num_done % (vif.wl ? vif.wl : 1);
            unpack_write(qa, v, vn);
            used += esize;
        }
        vif.num_done++;
    }
    if (vif.num_done >= num) vif.cmd = 0;
    return used;
}

/* ---- command stream ---- */

static void vif1_words(const uint32_t* w, uint32_t n)
{
    uint32_t i = 0;
    while (i < n) {
        if (vif.cmd) {
            uint32_t cmd = vif.cmd;
            if (cmd >= 0x60) {
                /* UNPACK data: consumes whole words */
                uint32_t used = do_unpack((const uint8_t*)&w[i], (n - i) * 4);
                uint32_t words = (used + 3) / 4;
                i += words;
                if (vif.cmd && words == 0) return;
                continue;
            }
            if (cmd == 0x20 || cmd == 0x30 || cmd == 0x31) {
                /* STMASK / STROW / STCOL operands */
                vif.tmp[vif.tmp_n++] = w[i++];
                if (cmd == 0x20) { vif.mask = vif.tmp[0]; vif.cmd = 0; }
                else if (vif.tmp_n == 4) {
                    memcpy(cmd == 0x30 ? vif.row : vif.col, vif.tmp, 16);
                    vif.cmd = 0;
                }
                continue;
            }
            if (cmd == 0x4A) { /* MPG: skip the microcode (we run the decoded listing) */
                uint32_t take = n - i < vif.remaining ? n - i : vif.remaining;
                i += take; vif.remaining -= take;
                if (!vif.remaining) vif.cmd = 0;
                continue;
            }
            if (cmd == 0x50 || cmd == 0x51) { /* DIRECT/DIRECTHL: qwords to the GIF */
                uint32_t take = n - i < vif.remaining ? n - i : vif.remaining;
                take &= ~3u;
                if (take == 0) return;
                gs_gif_transfer(2, &w[i], take / 4);
                i += take; vif.remaining -= take;
                if (!vif.remaining) vif.cmd = 0;
                continue;
            }
            vif.cmd = 0;
            continue;
        }

        {
            uint32_t code = w[i++];
            uint32_t cmd = (code >> 24) & 0x7F, imm = code & 0xFFFF, num = (code >> 16) & 0xFF;
            vif.code = code;
            switch (cmd) {
            case 0x00: break;                                    /* NOP */
            case 0x01: vif.cl = imm & 0xFF; vif.wl = (imm >> 8) & 0xFF; break; /* STCYCL */
            case 0x02: vif.ofst = imm & 0x3FF; vif.dbf = 0; vif.tops = vif.base; break;   /* OFFSET */
            case 0x03: vif.base = imm & 0x3FF; break;            /* BASE */
            case 0x04: vif.itops = imm & 0x3FF; break;           /* ITOP */
            case 0x05: vif.mode = imm & 3; break;                /* STMOD */
            case 0x06: break;                                    /* MSKPATH3 */
            case 0x07: break;                                    /* MARK */
            case 0x10: case 0x11: case 0x13: break;              /* FLUSHE/FLUSH/FLUSHA */
            case 0x14: vu1_start(imm); break;                    /* MSCAL */
            case 0x15: vu1_start(imm); break;                    /* MSCALF */
            case 0x17: vu1_start(g_vu1.cmsar0); break;           /* MSCNT: continue (rare) */
            case 0x20: vif.cmd = 0x20; vif.tmp_n = 0; break;     /* STMASK */
            case 0x30: case 0x31: vif.cmd = cmd; vif.tmp_n = 0; break; /* STROW/STCOL */
            case 0x4A:                                           /* MPG */
                vif.cmd = 0x4A; vif.remaining = (num ? num : 256) * 2; vif.mpg_addr = imm;
                break;
            case 0x50: case 0x51:                                /* DIRECT / DIRECTHL */
                vif.cmd = cmd; vif.remaining = (imm ? imm : 65536) * 4;
                if (i & 3) i = (i + 3) & ~3u;                    /* data starts at the next qword */
                break;
            default:
                if (cmd >= 0x60) {                               /* UNPACK */
                    vif.cmd = cmd;
                    vif.addr = (imm & 0x3FF) + ((imm & 0x8000) ? vif.tops : 0);
                    vif.num_done = 0;
                    if (vif.wl == 0) { vif.wl = 1; vif.cl = 1; }
                } else {
                    host_log("vif1: unknown command %02x\n", cmd);
                }
                break;
            }
        }
    }
}

void ps2_vif1_transfer_words(const uint32_t* w, uint32_t n) { vif1_words(w, n); }
void ps2_vif1_transfer(const uint32_t* data, uint32_t qwc) { vif1_words(data, qwc * 4); }

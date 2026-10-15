/*
 * Native 3D path (rendering rework, stage 1): chunk-model strips (njCnkCsUvh/njCnkCsUvn)
 * are drawn by the GPU instead of the VU1 microprograms (vsm/ps2_vu1.vsm SingleMain /
 * DoubleMain) and the GS rasterizer emulation.
 *
 * Per vertex the VU1 programs take the vertex buffer entry made by VU0 CALCPOINT (view
 * position V, screen position S = k * V.xy / V.z, Iz, lit intensity I, normal z, fog) and
 * the strip's UV, and output ST = UV * Iz, Q = Iz, RGBA from one of the vu1GetVertexColor*
 * routines, XYZF with the screen offset and Z = clamp(-b * Iz - a). Triangles are clipped
 * against the clip volume in VU1 (SCISSOR), and SingleMain skips triangles whose three
 * vertex normals face away (Nz > 0).
 *
 * Here the same values are computed on the CPU, but the position is passed in homogeneous
 * form, (offset * Vz + k * V.xy, Z * Vz, Vz), which is exactly linear in V: the GPU projects
 * and clips (near/far as clip distances), so nothing is cut into differently shaped pieces
 * per frame, and the texture coordinates go as (u, v, 1) for perspective-correct
 * interpolation.
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_Vu1Strip.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaView.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaDraw2D.h"
#include "../src/gs/native.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern float fVu1AspectH, fVu1Projection, fVu1OffsetX, fVu1OffsetY;
extern float Ps2_zbuff_b;
extern int g_vr_fx_fix;   /* vr_glue.c, ini vr_fx_fix */
#define VR_FX_MIN_DEPTH 100.0f   /* GS depth 100: ~65 m away; overlays use 0 */
/* VR: a 2D primitive covering more than half the screen either way is a screen layer (copies,
   fades, full-screen effects), never a projected effect. xy: the first vertex's (w = 1). */
static int vr_big(float x, float y, const void* verts, int n, size_t stride)
{
    float x0 = x, x1 = x, y0 = y, y1 = y;
    for (int i = 1; i < n; i++) {
        const float* p = (const float*)((const char*)verts + (size_t)i * stride);
        x0 = fminf(x0, p[0]); x1 = fmaxf(x1, p[0]); y0 = fminf(y0, p[1]); y1 = fmaxf(y1, p[1]);
    }
    return (x1 - x0) > 320.0f || (y1 - y0) > 240.0f;
}

static float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

/* vu1GetVertexColor* (vf18 ambient, vf19 diffuse, vf20 specular, vf21.w alpha ratio) */
static void vertex_color(int mode, const float in[4], float out[4])
{
    const float amb[3] = { vu1Ambient.fR, vu1Ambient.fG, vu1Ambient.fB };
    const float dif[3] = { vu1Diffuse.fR, vu1Diffuse.fG, vu1Diffuse.fB };
    const float spe[3] = { vu1Specula.fR, vu1Specula.fG, vu1Specula.fB };
    out[3] = fVu1AlphaRatio;
    for (int c = 0; c < 3; c++) {
        float i = in[c], ia = in[c] + amb[c], s = 1.0f;
        switch (mode) {
        case 0: out[c] = i * 128.0f; break;                       /* vu1GetVertexColor */
        case 1: out[c] = dif[c]; break;                           /* CM */
        case 2: out[c] = 128.0f; break;                           /* Ignore */
        case 3: out[c] = clamp01(i) * dif[c]; break;              /* Dif */
        case 4: out[c] = clamp01(ia) * dif[c]; break;             /* DifAmb */
        case 5: out[c] = clamp01(i) * dif[c] + clamp01(i - 1.0f) * spe[c]; break;
        case 6: out[c] = clamp01(i) * dif[c] + clamp01(ia - 1.0f) * spe[c]; break;
        case 7: for (int k = 0; k < 17; k++) s *= i;
                out[c] = clamp01(i) * dif[c] + clamp01(s) * spe[c]; break;
        case 8: out[c] = clamp01(ia) * dif[c] + clamp01(i - 1.0f) * spe[c]; break;
        case 9: out[c] = clamp01(ia) * dif[c] + clamp01(ia - 1.0f) * spe[c]; break;
        default: s = ia + 1.0f; for (int k = 0; k < 17; k++) s *= ia;   /* 10: Spe3Amb */
                out[c] = clamp01(ia) * dif[c] + clamp01(s) * spe[c]; break;
        }
    }
    if (mode == 0) out[3] = in[3] * 255.0f;
}

static uint8_t to_byte(float v) { return (uint8_t)((int32_t)v & 0xFF); }   /* FTOI0, GS takes 8 bits */

/* idx_uv: n triples (vertex index, u, v), UV scaled by uv_scale (1/1023 Uvh, 1/255 Uvn); prim:
   GS PRIM attribute bits of the strip; single: SingleMain (normal-based culling) */
void pc_native_strip(const unsigned short* idx_uv, int n, const VU1_STRIP_BUF* vbuf, unsigned int prim,
                     int color_mode, int single, float uv_scale)
{
    NativeVertex v[64];
    const float kx = fVu1AspectW * fVu1Projection, ky = fVu1AspectH * fVu1Projection;
    static int dbg = -1;
    if (dbg < 0) dbg = getenv("RECVX_NATIVE_DEBUG") ? 20 : 0;
    if (n > 64) n = 64;
    for (int i = 0; i < n; i++) {
        const VU1_STRIP_BUF* p = &vbuf[idx_uv[i * 3]];
        const float in[4] = { p->fIr, p->fIg, p->fIb, p->fNz };
        float col[4], w = p->fVz, z = -Ps2_zbuff_b * p->fIz - Ps2_zbuff_a;
        NativeVertex* o = &v[i];
        if (dbg > 0) {
            dbg--;
            fprintf(stderr, "native: V %.3f %.3f %.3f  S %.3f %.3f (k*V/z %.3f %.3f) Iz %.6g (1/Vz %.6g) z %.1f fog %.2f\n",
                    p->fVx, p->fVy, p->fVz, p->fSx, p->fSy, kx * p->fVx / p->fVz, ky * p->fVy / p->fVz, p->fIz, 1.0f / p->fVz, z, p->fFog);
        }
        vertex_color(color_mode, in, col);
        o->x = fVu1OffsetX * w + kx * p->fVx;
        o->y = fVu1OffsetY * w + ky * p->fVy;
        o->z = -Ps2_zbuff_b - Ps2_zbuff_a * w;   /* z * w, z = -b * Iz - a with Iz = 1 / Vz */
        o->w = w;
        o->s = uv_scale * (float)idx_uv[i * 3 + 1];
        o->t = uv_scale * (float)idx_uv[i * 3 + 2];
        o->r = to_byte(col[0]); o->g = to_byte(col[1]); o->b = to_byte(col[2]); o->a = to_byte(col[3]);
        o->fog = p->fFog;   /* wrapped to the GS's 8 bits only when drawn: clipping and blending use the real value */
        o->near_d = w - _fNaViwClipNear;
        o->far_d = _fNaViwClipFar - w;
        o->cull = (uint8_t)((single && p->fNz > 0.0f) | 0x80);   /* 0x80: 3D (VR re-projects it) */
    }
    native_strip(prim, v, n);
}

/* Ordering-table 3D (ps2_dummy.c Ps2AddPrim3DEx): strips the CPU strip functions
   (ps2_Vu1Strip.c vu1DrawTriangleStrip*) projected themselves. Per vertex VU1_PRIM_BUF holds
   the screen position (offset + S), the view depth fZ = Vz, ST = UV * Iz, Q = Iz, the colour
   from VU0 CALC_COLOR (floats), fog, and ulKick: 0x8000 skips the triangle ending at the
   vertex, with bit 0 added on the PC when the reason is the normal test (single-sided) rather
   than clipping (ps2_Vu1Strip.c; clipped triangles are clipped at draw time instead of by the
   CPU scissor). The homogeneous position is (X * Vz, Y * Vz, -b - a * Vz, Vz) as for the VU1
   path. Returns the deferred strip's number; *zavg gets the ordering-table key the original
   computes (mean GS depth, clamped to 0..65534 per vertex). */
extern uint32_t pc_cur_mesh, pc_cur_strip;
uint32_t pc_native_prim3d(unsigned int prim, const VU1_PRIM_BUF* in, int n, int deferred, float* zavg)
{
    NativeVertex v[64];   /* what native_draw draws; the key is over all vertices */
    float zsum = 0.0f;
    for (int i = 0; i < n; i++) {
        const VU1_PRIM_BUF* p = &in[i];
        NativeVertex* o = &v[i];
        float w = p->fZ, zg = -Ps2_zbuff_b * p->fQ - Ps2_zbuff_a;
        zsum += zg < 0.0f ? 0.0f : zg > 65534.0f ? 65534.0f : zg;
        if (i >= 64) continue;
        o->x = p->fX * w; o->y = p->fY * w;
        o->z = -Ps2_zbuff_b - Ps2_zbuff_a * w;
        o->w = w;
        o->s = p->fQ != 0.0f ? p->fS / p->fQ : 0.0f;
        o->t = p->fQ != 0.0f ? p->fT / p->fQ : 0.0f;
        o->r = to_byte(p->fR); o->g = to_byte(p->fG); o->b = to_byte(p->fB); o->a = to_byte(p->fA);
        o->fog = p->fF;
        o->near_d = w - _fNaViwClipNear;
        o->far_d = _fNaViwClipFar - w;
        o->cull = (uint8_t)(((p->ulKick & 0x8001) == 0x8001 ? 2 : 0) | 0x80);
    }
    *zavg = n ? zsum / (float)n : 0.0f;
    return native_defer(prim, v, n < 64 ? n : 64, deferred ? pc_cur_mesh : 0, pc_cur_strip);
}

/* Ps2AddPrim (ps2_dummy.c), the general builder behind the Ninja polygon/particle/effect draw
   calls, for triangle types. Per vertex three qwords as the GS takes them: ST (S, T, Q = 1/z,
   flags) or UV (texel ints, FST), RGBA ints, X Y Z F floats (screen position with offset,
   view z, fog). 3D (clip_3d_on): homogeneous position with w = view z, GS depth
   -b * Q - a; triangles the caller marked (0x8000 in the flags) are skipped, the rest
   clipped at draw time (the original drops triangles with a vertex off the GS area or
   behind the camera). 2D: w = 1, depth a + b / z as in the original. *last_sz gets the
   ordering-table key the original uses (the last vertex's clamped depth). */
uint32_t pc_native_prim(unsigned int prim, const unsigned int* dp, int n, int clip_3d_on, int deferred, float* last_sz)
{
    NativeVertex v[64];
    int fst = (prim >> 8) & 1;
    if (n > 64) n = 64;
    *last_sz = 0.0f;
    for (int i = 0; i < n; i++, dp += 12) {
        const float* f0 = (const float*)dp; const float* f2 = (const float*)(dp + 8);
        NativeVertex* o = &v[i];
        float x = f2[0], y = f2[1], z = f2[2], q = f0[2], sz;
        if (clip_3d_on) {
            sz = -Ps2_zbuff_b * q - Ps2_zbuff_a;
            o->w = z;
            o->z = -Ps2_zbuff_b - Ps2_zbuff_a * z;
            o->near_d = z - _fNaViwClipNear;
            o->far_d = _fNaViwClipFar - z;
        } else {
            sz = Ps2_zbuff_a + Ps2_zbuff_b / z;
            sz = sz < 0.0f ? 0.0f : sz > 65534.0f ? 65534.0f : sz;
            o->w = 1.0f;
            o->z = (float)(int)sz;
            o->near_d = o->far_d = 1.0f;
            x = (float)(int)(x * 16.0f) / 16.0f;   /* the GS gets 2D positions as 12.4 fixed point (FTOI4) */
            y = (float)(int)(y * 16.0f) / 16.0f;
        }
        *last_sz = sz < 0.0f ? 0.0f : sz > 65534.0f ? 65534.0f : sz;
        o->x = x * o->w; o->y = y * o->w;
        if (fst) { o->s = (float)(dp[0] & 0x3FFF) / 16.0f; o->t = (float)(dp[1] & 0x3FFF) / 16.0f; }   /* UV is 10.4 fixed point */
        else { o->s = q != 0.0f ? f0[0] / q : 0.0f; o->t = q != 0.0f ? f0[1] / q : 0.0f; }
        o->r = (uint8_t)dp[4]; o->g = (uint8_t)dp[5]; o->b = (uint8_t)dp[6]; o->a = (uint8_t)dp[7];
        o->fog = f2[3];
        o->cull = (uint8_t)((clip_3d_on && (dp[3] & 0x8000) ? 2 : 0) | (clip_3d_on ? 0x80 : 0));
        /* VR: 2D the game projected itself (fire, glows, particles) carries the GS depth of its view
           depth, as 3D does (a + b / z here, -b / Vz - a there: the same within ~6 of 65535);
           screen overlays have depth 0 and stay 2D */
        /* (not the game's own 2D: texts, message windows, icons are drawn at priority -1, the
           nearest depth there is - those go on the VR panel, not into the world) */
        if (!clip_3d_on && g_vr_fx_fix && o->z > VR_FX_MIN_DEPTH && z < -1.0001f) o->cull |= 0x40;   /* (in front of the camera only: z >= 0 is behind it, its depth and position meaningless) */
    }
    if (!clip_3d_on && n > 0 && vr_big(v[0].x, v[0].y, v, n, sizeof(NativeVertex)))   /* a screen layer: stays 2D */
        for (int i = 0; i < n; i++) v[i].cull &= (uint8_t)~0x40;
    else if (!clip_3d_on && n > 0 && g_vr_fx_fix) {
        /* VR: one effect polygon with some corners in the world (0x40) and some not - those came out
           at depth 0, behind or too close to the camera, their screen positions meaningless. Split
           between the world and the head-locked panel it stretched (blood on the floor when the head
           turned) or floated in a corner: in VR it is left out, as the GS's clipping hid it */
        int fx = 0, other = 0;
        for (int i = 0; i < n; i++) { if (v[i].cull & 0x40) fx++; else other++; }
        if (fx && other) {
            extern int vr_on(void);
            if (vr_on()) for (int i = 1; i < n; i++) { v[i].x = v[0].x; v[i].y = v[0].y; }   /* (no area: not drawn) */
            for (int i = 0; i < n; i++) v[i].cull |= 0x40;
        }
    }
    if (!clip_3d_on && n > 0) {   /* tall (more than half the screen top to bottom): a screen layer VR keeps over the whole view (0x20); wide and low goes on the panel */
        float y0 = v[0].y, y1 = v[0].y;
        for (int i = 1; i < n; i++) { y0 = fminf(y0, v[i].y); y1 = fmaxf(y1, v[i].y); }
        if (y1 - y0 > 240.0f) for (int i = 0; i < n; i++) v[i].cull |= 0x20;
    }
    return native_defer(prim, v, n, deferred ? pc_cur_mesh : 0, pc_cur_strip);
}

/* Ps2AddPrim for points, lines and sprites: what its GS packet would put into the GS
   registers (XYZF2 after FTOI4, RGBAQ, ST or UV), drawn by the GS vertex kick logic
   (gs_draw_raw). ADC as in the original: in 3D, a vertex off the GS area or with z outside
   1..65535 (and the two after it) or one the caller marked does not draw. */
uint32_t pc_native_prim_raw(unsigned int prim, const unsigned int* dp, int n, int clip_3d_on, float* last_sz)
{
    NativeRawVertex v[64];
    unsigned int clip_flag = 0;
    if (n > 64) n = 64;
    *last_sz = 0.0f;
    for (int i = 0; i < n; i++, dp += 12) {
        const float* f0 = (const float*)dp; const float* f2 = (const float*)(dp + 8);
        NativeRawVertex* o = &v[i];
        float x = f2[0], y = f2[1], z = f2[2], sz;
        clip_flag >>= 1;
        if (x < 0 || x > 4095.0f || y < 0 || y > 4095.0f) clip_flag |= 4;
        if (z < 1.0f || z > 65535.0f) clip_flag |= 4;
        sz = clip_3d_on ? -Ps2_zbuff_b * f0[2] - Ps2_zbuff_a : Ps2_zbuff_a + Ps2_zbuff_b / z;
        sz = sz < 0.0f ? 0.0f : sz > 65534.0f ? 65534.0f : sz;
        *last_sz = sz;
        o->x = (uint16_t)(int)(x * 16.0f); o->y = (uint16_t)(int)(y * 16.0f);
        o->z = (uint32_t)(((int)(sz * 16.0f)) >> 4);
        o->f = (uint8_t)((int)(f2[3] * 16.0f) >> 4);
        o->s = f0[0]; o->t = f0[1]; o->q = f0[2];
        o->u = (uint16_t)(dp[0] & 0x3FFF); o->v = (uint16_t)(dp[1] & 0x3FFF);
        o->r = (uint8_t)dp[4]; o->g = (uint8_t)dp[5]; o->b = (uint8_t)dp[6]; o->a = (uint8_t)dp[7];
        o->adc = clip_3d_on && (clip_flag != 0 || (dp[3] & 0x8000));
        o->ui = !clip_3d_on && z >= -1.0001f && z < 0.0f;   /* the game's own 2D (priority -1): the VR panel */
        o->vr3d = clip_3d_on || (g_vr_fx_fix && sz > VR_FX_MIN_DEPTH && !o->ui) ? 1 : 0;   /* as pc_native_prim */
    }
    if (!clip_3d_on && n > 0) {   /* a screen layer (more than half the screen either way) stays 2D */
        int x0 = v[0].x, x1 = v[0].x, y0 = v[0].y, y1 = v[0].y;
        for (int i = 1; i < n; i++) { x0 = v[i].x < x0 ? v[i].x : x0; x1 = v[i].x > x1 ? v[i].x : x1; y0 = v[i].y < y0 ? v[i].y : y0; y1 = v[i].y > y1 ? v[i].y : y1; }
        if (x1 - x0 > 320 * 16 || y1 - y0 > 240 * 16) for (int i = 0; i < n; i++) v[i].vr3d = 0;
    }
    return native_defer_raw(prim, v, n);
}

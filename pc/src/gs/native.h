/* Native 3D path (pc/glue/native_glue.c): GPU-projected triangles in the GS stream. */
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    float x, y, z, w;           /* homogeneous GS screen position (pixels before XYOFFSET) and GS depth, times w */
    float s, t;                 /* texture coordinates (0..1), interpolated perspective-correct */
    float fog;                  /* GS fog coefficient as computed (the GS takes its low 8 bits) */
    float near_d, far_d;        /* clip distances: >= 0 inside the near/far planes */
    uint8_t r, g, b, a;         /* GS colour (0x80 = 1.0) */
    uint8_t cull;               /* bit 0: normal faces away (a triangle of three such is skipped,
                                   SingleMain); bit 1: the triangle ending here is skipped;
                                   bit 7: a 3D vertex (homogeneous view position), VR re-projects it */
} NativeVertex;

/* game thread: triangles with GS PRIM bits (type 3 list, 4 strip, 5 fan; TME/FGE/ABE/IIP/
   FST/CTXT); see NativeVertex.cull */
void native_strip(unsigned int prim, const NativeVertex* v, int n);
/* game thread: a strip drawn later from GS data (ordering-table entries, see
   ps2_dummy.c Ps2AddPrim3DEx): the record is stored and a write of the returned number to
   GS register NATIVE_MARKER_REG takes the place of its vertices in the GS packet, so it is
   emitted exactly where the GS would have drawn it. mesh/strip: identity for interpolation
   (0: draw within the current mesh tag). */
#define NATIVE_MARKER_REG 0x7F
/* GS register write that follows an ordering-table entry drawn by the GS path inside an effect
   (ps2_dummy.c Ps2AddOT): the primitive group just drawn is the effect's (low 32 bits: its
   tag, high: the polygon's number in it), for frame interpolation (stream_tag_last) */
#define STREAM_TAG_REG 0x7E
uint32_t native_defer(unsigned int prim, const NativeVertex* v, int n, uint32_t mesh, uint32_t strip);
void     native_marker(uint64_t v);
/* Points, lines and sprites (and anything else the GS would draw from its registers): the
   values the GS registers would receive, drawn by the same vertex kick logic (gs.c
   gs_draw_raw), so sprites keep the renderer's GS semantics (e.g. widescreen fills). */
typedef struct {
    uint16_t x, y;              /* XYZ in 12.4 fixed point, before XYOFFSET */
    uint32_t z;
    float s, t, q;              /* ST / RGBAQ.Q */
    uint16_t u, v;              /* UV, 10.4 fixed point */
    uint8_t r, g, b, a, f;
    uint8_t adc;                /* 1: the vertex does not draw (XYZ3 / ADC) */
    uint8_t vr3d;               /* 1: a projected 3D point (z is the GS depth of its view depth): VR re-projects it */
    uint8_t ui;                 /* 1: the game's own 2D (texts, windows, icons at priority -1): VR puts it on the panel */
} NativeRawVertex;
uint32_t native_defer_raw(unsigned int prim, const NativeRawVertex* v, int n);
void     gs_draw_raw(unsigned int prim, const NativeRawVertex* v, int n);

/* render thread: draw n vertices (triangle list) with the current GS context; ev >= 0: the
   executed frame's strip of that stream event, kept for its replays (gs_draw_native_cached) */
void gs_draw_native(unsigned int prim, const NativeVertex* v, int n, long ev);
void gs_native_cache_begin(size_t nev);                 /* an executed frame starts */
int  gs_draw_native_cached(unsigned int prim, size_t ev);   /* replay: 0 if not kept */

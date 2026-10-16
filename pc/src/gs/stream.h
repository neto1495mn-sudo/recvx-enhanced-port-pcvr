/* GS command stream: game thread -> render thread, and frame interpolation (stream.c). */
#pragma once
#include <stdint.h>
#include "../host/vr_view.h"

typedef struct Frame Frame;

void     stream_init(void);
/* game thread */
void     stream_reg(int reg, uint64_t v);
void     stream_image(const uint8_t* data, uint32_t len);
void     stream_disp(uint32_t fbp, uint32_t fbw, uint32_t psm);
void     stream_tag(uint32_t mesh);                 /* following GS output belongs to this mesh (0: none) */
void     stream_strip(uint32_t strip);              /* ... and within it to this strip */
void     stream_tag_last(uint32_t mesh, uint32_t strip);   /* the primitive group just drawn belongs to this mesh and strip */
void     stream_tex_def_begin(uint32_t id, uint32_t tbp, uint32_t cbp, uint32_t image_pages);
void     stream_tex_def_end(void);
void     stream_tex_bind(uint32_t id, uint32_t tbp, uint32_t cbp);
void     stream_tex_forget(uint32_t id);
void     stream_rtex_capture(uint32_t id, uint32_t fbp, uint32_t fbw, uint32_t w, uint32_t h, int wide);
void     stream_movie(uint32_t tbp, int index);    /* movie frame index is the texture at tbp */
/* a point the overlay draws at (the co-op name tag), kept with the frame being drawn: screen x, y
   (640x480) and view depth w, STREAM_PT_* (only the frame's last call counts; none: 0) */
enum { STREAM_PT_FRONT = 1, STREAM_PT_BEHIND = 2 };
void     stream_point(float x, float y, float w, int flags);
void     stream_vr_view(const VrView* v);           /* VR: the camera of the frame being recorded (vr_glue.c) */
void     stream_submit(void);                       /* end of frame (buffer swap) */
void     stream_download(void* dst, uint32_t len);  /* StoreImage: waits for the pixels */
uint64_t stream_game_frames(void);
/* render thread */
Frame*   stream_pop(int timeout_ms);
void     stream_release(Frame* f);
int      stream_is_partial(const Frame* f);
int      stream_queued(void);      /* render thread: a game frame waits to be executed */
void     stream_exec(Frame* f);
void     stream_note_upload(const uint64_t* pages);
int      stream_match(const Frame* prev, const Frame* cur);
void     stream_replay(const Frame* f, float t);
/* VR: replay the executed frame exactly as executed (no interpolation, no match needed) */
void     stream_replay_exact(const Frame* f);
const VrView* stream_frame_vr(const Frame* f);   /* the frame's VR camera (valid = 0: none) */
/* cur's stream_point at t (in-between frames): blended from prev's as the 3D is, while both are
   in front of the camera; prev NULL or t >= 1: cur's. Returns cur's flags (0: no point). */
int      stream_point_at(const Frame* prev, const Frame* cur, float t, float* x, float* y);

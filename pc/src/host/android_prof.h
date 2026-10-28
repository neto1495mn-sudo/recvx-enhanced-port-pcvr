/* Android (Quest) diagnosis: CPU and GPU time of the render thread (a game frame's drawing and the headset images)
   and of the game thread, logged once a second (android_prof.c). */
#pragma once
#ifdef __ANDROID__
#include <stdint.h>

enum { AND_EXEC, AND_PRESENT, AND_N };

uint64_t and_thread_cpu_ns(void);                  /* this thread's CPU time */
/* render thread, GL context current: GPU time (GL_EXT_disjoint_timer_query) of the GL work between begin and end,
   read back frames later without waiting */
void and_gpu_begin(int k);
void and_gpu_end(int k);
void and_render_add(int k, uint64_t wall_ns, uint64_t cpu_ns, unsigned draws, unsigned prims);
void and_exec_gl(uint64_t flush_ns, uint64_t upload_ns, uint64_t draw_ns, uint64_t fbo_ns, uint64_t tex_reload_ns);   /* render thread: one game frame's drawing */
void and_game_frame(uint64_t wall_ns, uint64_t cpu_ns, uint64_t wait_ns);   /* game thread, at each submitted frame */
void and_prof_report(void);                        /* render thread, once a second */

extern volatile unsigned g_and_fx_split;           /* native_glue.c: effect polygons collapsed (corners in and out of the world) */
#endif

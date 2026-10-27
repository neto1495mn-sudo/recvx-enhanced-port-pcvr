/*
 * RECVX_REPLAY_CHECK=1: interpolation self-test. A replay at t = 1 blends every vertex to its
 * own position, so it must reproduce the executed frame; any difference is state the replay
 * sees wrongly (GS memory, CLUT, native textures as the frame left them instead of as each
 * draw saw them). gs_gl.c (gs_frame_executed) reads both images back and calls this; frames
 * with more than 0.2% of pixels off (max channel diff > 24, or > n with RECVX_REPLAY_CHECK=n)
 * are saved as BMP (at most every 30th frame) to
 * RECVX_REPLAY_CHECK_DIR (or RECVX_DUMP_FRAMES): exec, replay, and a diff (exec dimmed to
 * grey, differing pixels red).
 */
#include "../ps2_platform.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

#define SAVE_PERMILLE 2    /* save images above 0.2% differing pixels, at most every 30 frames */

static void save(const char* dir, uint64_t frame, const char* kind, uint8_t* px, int w, int h)
{
    char path[1024];
    SDL_Surface* sf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ABGR8888, px, w * 4);
    SDL_snprintf(path, sizeof(path), "%s/replaycheck_%05llu_%s.bmp", dir, (unsigned long long)frame, kind);
    SDL_SaveBMP(sf, path);
    SDL_DestroySurface(sf);
}

/* exec, replay: w*h RGBA8 read back from the render target (row 0 = GS line 0) */
void replay_check_compare(uint64_t frame, uint8_t* exec, uint8_t* replay, int w, int h)
{
    static const char* dir;
    static int init;
    static uint64_t checked, bad, saved;
    static int diff_min = 24;   /* max channel difference counted as a mismatch: RECVX_REPLAY_CHECK=n (n > 1) */
    size_t n = (size_t)w * h, diff = 0;
    int maxd = 0;
    uint8_t* d = malloc(n * 4);
    if (!init++) {
        dir = SDL_getenv("RECVX_REPLAY_CHECK_DIR");
        if (!dir) dir = SDL_getenv("RECVX_DUMP_FRAMES");
        if (SDL_atoi(SDL_getenv("RECVX_REPLAY_CHECK")) > 1) diff_min = SDL_atoi(SDL_getenv("RECVX_REPLAY_CHECK"));
    }
    for (size_t i = 0; i < n; i++) {
        const uint8_t *a = exec + i * 4, *b = replay + i * 4;
        int m = 0;
        for (int c = 0; c < 3; c++) { int x = abs(a[c] - b[c]); if (x > m) m = x; }
        if (m > maxd) maxd = m;
        if (m > diff_min) {
            diff++;
            d[i * 4] = 255; d[i * 4 + 1] = 0; d[i * 4 + 2] = 0;
        } else {
            uint8_t g = (uint8_t)((a[0] + a[1] + a[2]) / 9);
            d[i * 4] = d[i * 4 + 1] = d[i * 4 + 2] = g;
        }
        d[i * 4 + 3] = exec[i * 4 + 3] = replay[i * 4 + 3] = 255;   /* opaque BMPs */
    }
    checked++;
    if (diff * 1000 > n * SAVE_PERMILLE) bad++;
    host_log("replaycheck frame %llu: %zu px differ (%.3f%%), max diff %d [%llu/%llu frames bad]\n", (unsigned long long)frame, diff,
             100.0 * diff / n, maxd, (unsigned long long)bad, (unsigned long long)checked);
    if (dir && diff * 1000 > n * SAVE_PERMILLE && (!saved || frame >= saved + 30)) {
        saved = frame;
        save(dir, frame, "exec", exec, w, h);
        save(dir, frame, "replay", replay, w, h);
        save(dir, frame, "diff", d, w, h);
    }
    free(d);
}

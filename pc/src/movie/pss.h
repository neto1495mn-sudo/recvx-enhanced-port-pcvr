/* PSS (PS2 MPEG-2 program stream) demuxer: video elementary stream + SShd/SSbd PCM audio. */
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void (*video)(void* user, const uint8_t* es, size_t len);
    void (*audio)(void* user, const int16_t* stereo, size_t frames, int rate);   /* interleaved s16 */
    void* user;
} PssSink;

typedef struct Pss Pss;
Pss* pss_create(const PssSink* sink);
void pss_destroy(Pss* p);
/* Feed the next bytes of the file; returns 0 at the program end code or on a fatal error. */
int  pss_feed(Pss* p, const uint8_t* data, size_t len);

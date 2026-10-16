/* MPEG-2 video decoder for PSS movies (see mpeg2.c for the supported subset). */
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *y, *cb, *cr;
    int width, height;
    int y_stride, c_stride;
} Mpeg2Frame;

typedef void (*Mpeg2FrameFn)(const Mpeg2Frame* frame, void* user);   /* frames in display order */
typedef struct Mpeg2 Mpeg2;

Mpeg2*   mpeg2_create(Mpeg2FrameFn on_frame, void* user);
void     mpeg2_destroy(Mpeg2* d);
/* Decode all complete start-code units in data; returns the bytes consumed (keep the rest
   and pass it again with more data). eof = no more data follows. */
size_t   mpeg2_decode(Mpeg2* d, const uint8_t* data, size_t len, int eof);
/* the same, but returns once a frame has been delivered */
size_t   mpeg2_decode_frame(Mpeg2* d, const uint8_t* data, size_t len, int eof);
unsigned mpeg2_slice_errors(const Mpeg2* d);
unsigned mpeg2_pictures(const Mpeg2* d);

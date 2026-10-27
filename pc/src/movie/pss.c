/*
 * PSS demuxer. Packs/PES packets are MPEG-2 program stream; stream 0xE0 is the video
 * elementary stream, private stream 1 (0xBD) carries audio behind a 4-byte sub-stream
 * header: an "SShd" chunk (type, rate, channels, interleave) then "SSbd" with the data,
 * channel-interleaved in blocks of `interleave` bytes. Only 16-bit PCM is used on this disc.
 */
#include "pss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Pss {
    PssSink sink;
    uint8_t* buf; size_t len, cap;          /* unparsed stream bytes */
    uint8_t* abuf; size_t alen, acap;       /* audio payload not yet converted */
    int have_hdr, type, rate, channels, interleave;
    size_t hdr_skip;                        /* bytes of SShd/SSbd header still to drop */
    int video_id;                           /* vr189: the video stream's id (0xE0 on the original disc; 0xE1 in some re-made ones) */
};

Pss* pss_create(const PssSink* sink)
{
    Pss* p = calloc(1, sizeof(*p));
    p->sink = *sink;
    return p;
}

void pss_destroy(Pss* p)
{
    if (!p) return;
    free(p->buf); free(p->abuf); free(p);
}

static void append(uint8_t** b, size_t* len, size_t* cap, const uint8_t* d, size_t n)
{
    if (*len + n > *cap) { *cap = (*len + n) * 2 + 4096; *b = realloc(*b, *cap); }
    memcpy(*b + *len, d, n);
    *len += n;
}

static uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

static void audio_payload(Pss* p, const uint8_t* d, size_t n)
{
    append(&p->abuf, &p->alen, &p->acap, d, n);
    if (!p->have_hdr) {
        if (p->alen < 40) return;
        if (memcmp(p->abuf, "SShd", 4) || le32(p->abuf + 4) > p->alen - 12 || memcmp(p->abuf + 8 + le32(p->abuf + 4), "SSbd", 4)) {   /* (vr189: a length past the bytes read is no header) */
            /* vr189: no SShd/SSbd header - movies re-made for a dubbed disc (MV_002..004 of the BR one) start
               without it, partway into the sound. The original's format is taken (16-bit PCM, 48000 Hz, 2
               channels, 512-byte blocks: as MV_001's header) and the block start found from the sound itself:
               the offset where each block's last sample runs on into the next block of the same channel */
            enum { NEED = 65536 };
            int best_off = 0;
            double best = 1e30;
            if (p->alen < NEED) return;
            for (int off = 0; off < 1024; off += 2) {
                double tot = 0; int n = 0;
                for (size_t b = off; b + 1024 + 2 <= p->alen; b += 1024) {
                    int16_t l_end = (int16_t)(p->abuf[b + 510] | (p->abuf[b + 511] << 8));
                    int16_t l_next = (int16_t)(p->abuf[b + 1024] | (p->abuf[b + 1025] << 8));
                    tot += abs(l_end - l_next); n++;
                }
                if (n && tot / n < best) { best = tot / n; best_off = off; }
            }
            fprintf(stderr, "pss: no audio header - taken as 16-bit PCM 48000 Hz stereo, 512-byte blocks, starting %d bytes in (block-edge step %.0f)\n", best_off, best);
            p->type = 1; p->rate = 48000; p->channels = 2; p->interleave = 512;
            p->hdr_skip = (size_t)best_off;
            p->have_hdr = 1;
        } else {
        p->type = (int)le32(p->abuf + 8);
        p->rate = (int)le32(p->abuf + 12);
        p->channels = (int)le32(p->abuf + 16);
        p->interleave = (int)le32(p->abuf + 20);
        p->hdr_skip = 8 + le32(p->abuf + 4) + 8;
        p->have_hdr = 1;
        }
        if (p->type != 1 || p->channels < 1 || p->channels > 2 || p->interleave <= 0 || p->interleave % 2) {
            fprintf(stderr, "pss: unsupported audio (type %d, %d ch, interleave %d)\n", p->type, p->channels, p->interleave);
            p->channels = 0;
        }
    }
    if (p->hdr_skip) {
        size_t k = p->hdr_skip < p->alen ? p->hdr_skip : p->alen;
        memmove(p->abuf, p->abuf + k, p->alen - k);
        p->alen -= k; p->hdr_skip -= k;
    }
    if (!p->channels) { p->alen = 0; return; }
    {
        size_t blk = (size_t)p->interleave * p->channels, frames = p->interleave / 2, done = 0;
        int16_t out[4096];
        while (p->alen - done >= blk && frames * 2 <= 4096) {
            const uint8_t* s = p->abuf + done;
            for (size_t i = 0; i < frames; i++) {
                int16_t l = (int16_t)(s[i * 2] | (s[i * 2 + 1] << 8));
                int16_t r = p->channels > 1 ? (int16_t)(s[p->interleave + i * 2] | (s[p->interleave + i * 2 + 1] << 8)) : l;
                out[i * 2] = l; out[i * 2 + 1] = r;
            }
            if (p->sink.audio) p->sink.audio(p->sink.user, out, frames, p->rate);
            done += blk;
        }
        memmove(p->abuf, p->abuf + done, p->alen - done);
        p->alen -= done;
    }
}

int pss_feed(Pss* p, const uint8_t* data, size_t n)
{
    size_t pos = 0;
    append(&p->buf, &p->len, &p->cap, data, n);
    for (;;) {
        size_t need;
        int id;
        while (pos + 4 <= p->len && !(p->buf[pos] == 0 && p->buf[pos + 1] == 0 && p->buf[pos + 2] == 1)) pos++;
        if (pos + 6 > p->len) break;
        id = p->buf[pos + 3];
        if (id == 0xB9) { p->len = 0; return 0; }               /* program end */
        if (id == 0xBA) {
            if (pos + 14 > p->len) break;
            need = (p->buf[pos + 4] & 0xC0) == 0x40 ? 14 + (p->buf[pos + 13] & 7) : 12;
        } else if (id >= 0xBB) {
            need = 6 + ((size_t)p->buf[pos + 4] << 8 | p->buf[pos + 5]);
        } else { pos += 3; continue; }                           /* stray start code */
        if (pos + need > p->len) break;
        if ((id & 0xF0) == 0xE0 && !p->video_id) {   /* vr189: the first video stream seen */
            p->video_id = id;
            if (id != 0xE0) fprintf(stderr, "pss: video in stream %02X\n", id);
        }
        if (id == p->video_id || id == 0xBD) {
            const uint8_t* pk = p->buf + pos;
            size_t hdr = 9 + pk[8];
            if ((pk[6] & 0xC0) == 0x80 && hdr <= need) {
                if (id == p->video_id) { if (p->sink.video) p->sink.video(p->sink.user, pk + hdr, need - hdr); }
                else if (need >= hdr + 4) audio_payload(p, pk + hdr + 4, need - hdr - 4);
            }
        }
        pos += need;
    }
    memmove(p->buf, p->buf + pos, p->len - pos);
    p->len -= pos;
    return 1;
}

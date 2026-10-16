/*
 * Decodes every PSS movie of the game disc a frame at a time, as the player does, and checks
 * that each MPEG-2 slice ends exactly at the next start code (a wrong VLC table entry
 * desynchronises slices) and that every picture comes out. Needs RECVX_ISO; skipped (exit 77)
 * without it. Reads the image in place.
 */
#include "../src/movie/mpeg2.h"
#include "../src/movie/pss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* iso;
static uint8_t* es; static size_t es_len, es_cap;
static unsigned frames; static size_t audio_frames;

static int rd(uint32_t lsn, uint32_t n, void* dst) { return fseeko(iso, (off_t)lsn * 2048, SEEK_SET) == 0 && fread(dst, 2048, n, iso) == n; }
static uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static void on_frame(const Mpeg2Frame* f, void* u) { (void)f; (void)u; frames++; }
static void on_video(void* u, const uint8_t* d, size_t n)
{
    (void)u;
    if (es_len + n > es_cap) { es_cap = (es_len + n) * 2; es = realloc(es, es_cap); }
    memcpy(es + es_len, d, n); es_len += n;
}
static void on_audio(void* u, const int16_t* s, size_t n, int rate) { (void)u; (void)s; (void)rate; audio_frames += n; }
/* a frame at a time, as the player decodes; returns the bytes consumed */
static size_t decode(Mpeg2* dec, int eof)
{
    size_t total = 0, used;
    do total += used = mpeg2_decode_frame(dec, es + total, es_len - total, eof);
    while (used && total < es_len);
    return total;
}

int main(void)
{
    const char* path = getenv("RECVX_ISO");
    uint8_t sec[2048], dir[2048 * 8];
    int failed = 0, count = 0;
    if (!path || !(iso = fopen(path, "rb"))) { printf("RECVX_ISO not set: skipped\n"); return 77; }
    if (!rd(16, 1, sec)) return 1;
    {   /* \MOVIE directory */
        uint32_t root = le32(sec + 158), rsize = le32(sec + 166), mlsn = 0, msize = 0;
        if (rsize > sizeof(dir) || !rd(root, (rsize + 2047) / 2048, dir)) return 1;
        for (uint32_t p = 0; p < rsize && dir[p];) {
            if (dir[p + 32] == 5 && !memcmp(dir + p + 33, "MOVIE", 5)) { mlsn = le32(dir + p + 2); msize = le32(dir + p + 10); }
            p += dir[p]; if (p % 2048 && !dir[p]) p = (p / 2048 + 1) * 2048;
        }
        if (!mlsn || msize > sizeof(dir) || !rd(mlsn, (msize + 2047) / 2048, dir)) { printf("no MOVIE directory\n"); return 1; }
        for (uint32_t p = 0; p < msize;) {
            if (!dir[p]) { p = (p / 2048 + 1) * 2048; continue; }
            if (dir[p + 32] > 4 && !memcmp(dir + p + 33 + dir[p + 32] - 6, ".PSS;1", 6)) {
                uint32_t lsn = le32(dir + p + 2), size = le32(dir + p + 10);
                PssSink sink = { on_video, on_audio, NULL };
                Pss* pss = pss_create(&sink);
                Mpeg2* dec = mpeg2_create(on_frame, NULL);
                static uint8_t chunk[64 * 2048];
                size_t used;
                frames = 0; audio_frames = 0; es_len = 0;
                for (uint32_t off = 0; off < size; off += sizeof(chunk)) {
                    uint32_t n = size - off < sizeof(chunk) ? size - off : sizeof(chunk);
                    if (!rd(lsn + off / 2048, (n + 2047) / 2048, chunk)) break;
                    pss_feed(pss, chunk, n);
                    used = decode(dec, 0);
                    memmove(es, es + used, es_len - used); es_len -= used;
                }
                decode(dec, 1);
                printf("%.*s: %u pictures, %u frames out, %.1f s audio, %u slice errors\n", dir[p + 32] - 2, dir + p + 33,
                       mpeg2_pictures(dec), frames, audio_frames / 48000.0, mpeg2_slice_errors(dec));
                failed |= mpeg2_slice_errors(dec) != 0 || frames == 0 || frames != mpeg2_pictures(dec);
                count++;
                mpeg2_destroy(dec);
                pss_destroy(pss);
            }
            p += dir[p];
        }
    }
    printf("%d movies, %s\n", count, failed ? "FAILED" : "ok");
    return failed;
}

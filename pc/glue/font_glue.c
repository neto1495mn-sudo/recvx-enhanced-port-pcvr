/* vr138: the font's letter widths (FontSz, message.c) come from the disc's SLUS_201.84. Fan
   translations put accented letters in the spare font cells and patch their widths there; the
   decomp's table is the US one, so "Não" drew as "Nã o". Called once after the disc image opens. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ps2_platform.h"

extern unsigned char FontSz[126];

void fontsz_from_iso(void)
{
    uint32_t lsn, size, n;
    uint8_t* exe;
    if (!cdvd_find_file("cdrom0:\\SLUS_201.84;1", &lsn, &size) || size < 126) return;
    n = (size + 2047) / 2048;
    exe = malloc((size_t)n * 2048);
    if (!exe) return;
    if (cdvd_read_sectors(lsn, n, exe))
        /* the first 16 widths (space, punctuation) are the same in every version seen */
        for (uint32_t i = 0; i + 126 <= size; i++)
            if (memcmp(exe + i, FontSz, 16) == 0) { memcpy(FontSz, exe + i, 126); break; }
    free(exe);
}

/* vr138: the font's letter widths (FontSz, message.c) come from the disc's SLUS_201.84. Fan
   translations put accented letters in the spare font cells and patch their widths there; the
   decomp's table is the US one, so "Não" drew as "Nã o". Called once after the disc image opens. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ps2_platform.h"

extern unsigned char FontSz[126];
int lang_exe_font(const unsigned char* x, uint32_t n, unsigned* count, const unsigned char** table);   /* import/lang.c */

/* a disc whose own font has more letters than FontSz (the Russian fan translation: 192), played directly: their
   widths; without them bhGetFontSize gives letters from 108 up no width, and they are drawn on top of each other */
static unsigned char disc_w[256];
static int disc_nw;
float pc_disc_font_width(unsigned short code) { return code < disc_nw ? disc_w[code] : -1; }

void fontsz_from_iso(void)
{
    uint32_t lsn, size, n;
    uint8_t* exe;
    if (!cdvd_find_file("cdrom0:\\SLUS_201.84;1", &lsn, &size) || size < 126) return;
    n = (size + 2047) / 2048;
    exe = malloc((size_t)n * 2048);
    if (!exe) return;
    if (cdvd_read_sectors(lsn, n, exe)) {
        unsigned count = 0;
        const unsigned char* table = NULL;
        disc_nw = 0;
        if (lang_exe_font(exe, size, &count, &table) && count > 126 && count <= 256) {
            memcpy(disc_w, table, count);
            disc_nw = (int)count;
            host_log("font: the disc's font has %u letters, their widths from its executable\n", count);
        }
        /* the first 16 widths (space, punctuation) are the same in every version seen */
        for (uint32_t i = 0; i + 126 <= size; i++)
            if (memcmp(exe + i, FontSz, 16) == 0) { memcpy(FontSz, exe + i, 126); break; }
    }
    free(exe);
}

/* vr151: which plate texture the title has (lobby_glue.c MULTIPLAYER / SAIR letters), seen on the disc: the
   Brazilian translation's title texture sits at byte 0x266BBB20 (sector 0x4CD77 + 0x320); its first row
   (v 64) has letters at u 100..135 (CARREGAR PARTIDA), the US one (LOAD GAME from u 160) none there */
int g_plates_br;
void plates_from_iso(void)
{
    enum { LSN = 0x4CD77, AT = 0x320, NS = 12 };
    uint8_t* b = malloc(NS * 2048);
    int ink = 0;
    if (!b) return;
    if (cdvd_read_sectors(LSN, NS, b) && memcmp(b + AT, "TIM2", 4) == 0
        && b[AT + 0x94] == 0x00 && b[AT + 0x95] == 0x02 && b[AT + 0x93] == 4) {   /* 512 wide, 4 bits */
        const uint8_t* img = b + AT + 0x80 + (b[AT + 0x8C] | b[AT + 0x8D] << 8);
        for (int y = 74; y < 86; y++)
            for (int x = 100; x < 136; x += 2) if (img[(y * 512 + x) / 2]) ink++;
    }
    g_plates_br = ink > 8;
    free(b);
}

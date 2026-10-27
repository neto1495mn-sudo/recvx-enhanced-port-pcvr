/*
 * libcdvd on the host: reads the player's game disc image (ISO9660) in place.
 * Sector reads complete synchronously; the async API (sceCdRead + sceCdSync)
 * therefore always reports "done".
 */
#include "ps2_platform.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define SECTOR 2048

static FILE* iso;
static int cd_error;

int cdvd_open_image(const char* path)
{
    char pvd[SECTOR];
    if (iso) fclose(iso);
    iso = pc_fopen(path, "rb");
    if (!iso) return 0;
    if (cdvd_read_sectors(16, 1, pvd) != 1 || memcmp(pvd + 1, "CD001", 5) != 0) {
        fclose(iso); iso = NULL;
        return 0;
    }
    return 1;
}

int cdvd_read_sectors(uint32_t lsn, uint32_t count, void* dst)
{
    if (!iso) return 0;
    if (fseeko(iso, (off_t)lsn * SECTOR, SEEK_SET) != 0) return 0;
    { extern void pc_tex_invalidate(const void*, unsigned int); pc_tex_invalidate(dst, count * SECTOR); }   /* native textures */
    return (int)(fread(dst, SECTOR, count, iso) == count ? count : 0);
}

static uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

/* Look up one path component in a directory extent. */
static int find_in_dir(uint32_t dir_lsn, uint32_t dir_size, const char* name, uint32_t* lsn, uint32_t* size, int* is_dir)
{
    uint8_t sec[SECTOR];
    size_t nlen = strlen(name);
    for (uint32_t off = 0; off < dir_size; off += SECTOR) {
        if (!cdvd_read_sectors(dir_lsn + off / SECTOR, 1, sec)) return 0;
        for (uint32_t p = 0; p < SECTOR && sec[p];) {
            const uint8_t* rec = sec + p;
            uint8_t len = rec[0], flen = rec[32];
            const char* fid = (const char*)rec + 33;
            size_t cmp = flen;
            const char* semi = memchr(fid, ';', flen);
            if (semi) cmp = (size_t)(semi - fid);
            if (cmp == nlen && SDL_strncasecmp(fid, name, nlen) == 0) {
                *lsn = le32(rec + 2); *size = le32(rec + 10); *is_dir = (rec[25] & 2) != 0;
                return 1;
            }
            p += len;
        }
    }
    return 0;
}

/* path like "\\PS2_DATA\\MODULES\\PADMAN.IRX;1" or "cdrom0:\\SYSTEM.AFS;1" */
int cdvd_find_file(const char* path, uint32_t* lsn, uint32_t* size)
{
    uint8_t pvd[SECTOR];
    char comp[256];
    uint32_t cur_lsn, cur_size;
    int is_dir = 1;
    const char* p = strchr(path, ':');
    p = p ? p + 1 : path;

    if (!cdvd_read_sectors(16, 1, pvd)) return 0;
    cur_lsn = le32(pvd + 156 + 2);
    cur_size = le32(pvd + 156 + 10);

    while (*p) {
        size_t n = 0;
        while (*p == '\\' || *p == '/') p++;
        if (!*p) break;
        while (*p && *p != '\\' && *p != '/' && *p != ';' && n < sizeof(comp) - 1) comp[n++] = *p++;
        comp[n] = 0;
        if (*p == ';') while (*p && *p != '\\' && *p != '/') p++;
        if (!is_dir || !find_in_dir(cur_lsn, cur_size, comp, &cur_lsn, &cur_size, &is_dir)) return 0;
    }
    *lsn = cur_lsn; *size = cur_size;
    return 1;
}

/* ---------------- libcdvd API ---------------- */
int sceCdInit(int mode)   { (void)mode; ps2_poll_interrupts(); return 1; }
int sceCdMmode(int media) { (void)media; return 1; }
int sceCdDiskReady(int mode) { (void)mode; return 2; /* SCECdComplete */ }
int sceCdSync(int mode)   { (void)mode; ps2_poll_interrupts(); return 0; }
int sceCdGetError(void)   { return cd_error; }
int sceCdBreak(void)      { return 1; }
int sceCdPause(void)      { return 1; }

int sceCdSearchFile(sceCdlFILE* fp, const char* name)
{
    uint32_t lsn, size;
    if (!cdvd_find_file(name, &lsn, &size)) {
        host_log("cdvd: file not found: %s\n", name);
        return 0;
    }
    memset(fp, 0, sizeof(*fp));
    fp->lsn = lsn; fp->size = size;
    {
        const char* base = strrchr(name, '\\');
        SDL_strlcpy(fp->name, base ? base + 1 : name, sizeof(fp->name));
    }
    return 1;
}

int sceCdRead(u_int lbn, u_int sectors, void* buf, sceCdRMode* mode)
{
    (void)mode;
    cd_error = cdvd_read_sectors(lbn, sectors, buf) ? 0 : 0x30 /* SCECdErREAD */;
    ps2_poll_interrupts();
    return 1;
}

int sceCdReadClock(sceCdCLOCK* rtc)
{
    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
#define BCD(v) (u_char)((((v) / 10) << 4) | ((v) % 10))
    rtc->stat = 0;
    rtc->second = BCD(tmv.tm_sec);
    rtc->minute = BCD(tmv.tm_min);
    rtc->hour = BCD(tmv.tm_hour);
    rtc->pad = 0;
    rtc->day = BCD(tmv.tm_mday);
    rtc->month = BCD(tmv.tm_mon + 1);
    rtc->year = BCD(tmv.tm_year % 100);
#undef BCD
    return 1;
}

/* ---------------- streaming (sceCdSt*): sequential reads for movies ---------------- */
static uint32_t st_lsn;

int sceCdStInit(u_int bufmax, u_int bankmax, u_int iop_bufaddr) { (void)bufmax; (void)bankmax; (void)iop_bufaddr; return 1; }
int sceCdStStart(u_int lbn, sceCdRMode* mode) { (void)mode; st_lsn = lbn; return 1; }
int sceCdStStop(void) { return 1; }
int sceCdStRead(u_int size, u_int* buf, u_int mode, u_int* err)
{
    (void)mode;
    int n = cdvd_read_sectors(st_lsn, size, buf);
    st_lsn += (uint32_t)n;
    if (err) *err = n ? 0 : 0x32; /* SCECdErEOM */
    return n;
}

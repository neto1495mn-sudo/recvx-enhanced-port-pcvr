/* pc/src/host/fileio.c: UTF-8 paths, saving through a temporary file. Also runs as the
   Windows build under Wine (test_fileio.exe), where the UTF-8 -> UTF-16 path matters. */
#include "../src/host/fileio.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int fill_text(FILE* f, void* ud) { return fputs((const char*)ud, f) < 0 ? -1 : 0; }
static int fill_fail(FILE* f, void* ud) { fputs("half", f); (void)ud; return -1; }

static int read_back(const char* path, char* buf, int n)
{
    FILE* f = pc_fopen(path, "rb");
    int k;
    if (!f) return -1;
    k = (int)fread(buf, 1, (size_t)n - 1, f);
    buf[k] = 0;
    fclose(f);
    return k;
}

int main(void)
{
    const char* path = "test_fileio_\xC3\x9C" "ber \xE6\x97\xA5\xE6\x9C\xAC.ini";   /* "Über 日本" in UTF-8 */
    char buf[64], tmp[256];
    snprintf(tmp, sizeof tmp, "%s.%d.tmp", path, (int)getpid());
    CHECK(pc_save_file(path, "wb", fill_text, "first\n") == 0);
    CHECK(read_back(path, buf, sizeof buf) == 6 && !strcmp(buf, "first\n"));
    CHECK(pc_save_file(path, "wb", fill_text, "second, longer\n") == 0);   /* replaces an existing file */
    CHECK(read_back(path, buf, sizeof buf) > 0 && !strcmp(buf, "second, longer\n"));
    CHECK(pc_save_file(path, "wb", fill_fail, NULL) == -1);   /* a failed write keeps the old file */
    CHECK(read_back(path, buf, sizeof buf) > 0 && !strcmp(buf, "second, longer\n"));
    CHECK(read_back(tmp, buf, sizeof buf) < 0);   /* and leaves no temporary file */
    CHECK(pc_save_file("no_such_dir_test_fileio/x.ini", "wb", fill_text, "x") == -1);
#ifdef _WIN32
    {   /* the name on disk is the real one, not the ANSI code page's rendering of the bytes */
        FILE* f = _wfopen(L"test_fileio_Über 日本.ini", L"rb");
        CHECK(f != NULL);
        if (f) fclose(f);
        _wremove(L"test_fileio_Über 日本.ini");
    }
#else
    remove(path);
#endif
    printf("fileio: %s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}

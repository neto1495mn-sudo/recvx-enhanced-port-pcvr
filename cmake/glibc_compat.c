/* RECVX_GLIBC_COMPAT (Linux): definitions in the executable for glibc symbols newer than 2.34,
   so it does not need them from the system's glibc (docs/BUILDING.md "Running on older distributions (glibc)").
   - __isoc23_*: glibc 2.38 headers redirect scanf/strtol to these under C23 or _GNU_SOURCE
     (SDL, ImGui, miniupnpc); here they call the classic functions (no 0b prefix: unused).
   - strlcpy & co. (2.38): SDL uses them when the build system has them.
   - _dl_find_object (2.35): the static libgcc unwinder; the system's if it has one.
   Compiled as C11 without _GNU_SOURCE, so the calls below are not redirected themselves. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#if __GLIBC_USE(C23_STRTOL)
#error "glibc_compat.c must be compiled without C23/_GNU_SOURCE (it would call itself)"
#endif

int __isoc23_sscanf(const char* s, const char* f, ...)
{
    va_list ap;
    int r;
    va_start(ap, f);
    r = vsscanf(s, f, ap);
    va_end(ap);
    return r;
}
int __isoc23_fscanf(FILE* fp, const char* f, ...)
{
    va_list ap;
    int r;
    va_start(ap, f);
    r = vfscanf(fp, f, ap);
    va_end(ap);
    return r;
}
int __isoc23_vsscanf(const char* s, const char* f, va_list ap) { return vsscanf(s, f, ap); }
long __isoc23_strtol(const char* s, char** e, int b) { return strtol(s, e, b); }
long long __isoc23_strtoll(const char* s, char** e, int b) { return strtoll(s, e, b); }
unsigned long __isoc23_strtoul(const char* s, char** e, int b) { return strtoul(s, e, b); }
unsigned long long __isoc23_strtoull(const char* s, char** e, int b) { return strtoull(s, e, b); }
long __isoc23_wcstol(const wchar_t* s, wchar_t** e, int b) { return wcstol(s, e, b); }

size_t strlcpy(char* d, const char* s, size_t n)
{
    size_t l = strlen(s);
    if (n) { size_t k = l < n - 1 ? l : n - 1; memcpy(d, s, k); d[k] = 0; }
    return l;
}
size_t strlcat(char* d, const char* s, size_t n)
{
    size_t dl = strnlen(d, n);
    return dl == n ? n + strlen(s) : dl + strlcpy(d + dl, s, n - dl);
}
size_t wcslcpy(wchar_t* d, const wchar_t* s, size_t n)
{
    size_t l = wcslen(s);
    if (n) { size_t k = l < n - 1 ? l : n - 1; wmemcpy(d, s, k); d[k] = 0; }
    return l;
}
size_t wcslcat(wchar_t* d, const wchar_t* s, size_t n)
{
    size_t dl = 0;
    while (dl < n && d[dl]) dl++;
    return dl == n ? n + wcslen(s) : dl + wcslcpy(d + dl, s, n - dl);
}

/* ponytail: on glibc < 2.35 there is no lookup, so the static unwinder finds no unwind data
   (C++ exceptions would terminate; the port throws none). Upgrade: a dl_iterate_phdr search. */
extern void* dlvsym(void* handle, const char* name, const char* version);
int _dl_find_object(void* pc, void* result)
{
    static int (*sys)(void*, void*);
    static int looked;
    if (!looked) { sys = (int (*)(void*, void*))dlvsym((void*)-1l /* RTLD_NEXT */, "_dl_find_object", "GLIBC_2.35"); looked = 1; }
    return sys ? sys(pc, result) : -1;
}

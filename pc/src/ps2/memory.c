#ifndef _WIN32
#define _GNU_SOURCE
#endif
/*
 * PS2 address-space setup.
 *
 * Memory model (docs/ARCHITECTURE.md "Data model and memory"): the executable is linked at 0x30000000 so that
 * the EE's uncached / uncached-accelerated aliases (addr|0x20000000,
 * addr|0x30000000) are identities on every game object, and DMA tags can hold
 * plain 31-bit host addresses. The 16 KB scratchpad is mapped at its real EE
 * address, 0x70000000, because the platform code builds packets at literal
 * scratchpad addresses.
 */
#ifdef _WIN32
/* the PS2 kernel shim declares its own GetThreadId/CreateThread */
#define GetThreadId win_GetThreadId
#define CreateThread win_CreateThread
#include <windows.h>
#undef GetThreadId
#undef CreateThread
#endif
#include "ps2_platform.h"
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/mman.h>
#include <link.h>
#include <unistd.h>
#endif

Ps2FakeRegs ps2_regs;

#ifndef _WIN32
/* The decomp writes into string literals (no MMU protection on the EE). Make the
   executable's read-only data writable. */
static int unprotect_cb(struct dl_phdr_info* info, size_t sz, void* data)
{
    (void)sz; (void)data;
#ifdef __ANDROID__
    {   /* Android: the game is librecvx.so (android/loader.c), linked with its read-only data in a segment of its own */
        const char* n = info->dlpi_name ? info->dlpi_name : "";
        size_t l = strlen(n);
        if (l < 11 || strcmp(n + l - 11, "librecvx.so")) return 0;
    }
#else
    if (info->dlpi_name && info->dlpi_name[0]) return 0; /* main executable only */
#endif
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr)* ph = &info->dlpi_phdr[i];
        if (ph->p_type == PT_LOAD && !(ph->p_flags & PF_W) && !(ph->p_flags & PF_X)) {
            uintptr_t page = (uintptr_t)sysconf(_SC_PAGESIZE);
            uintptr_t a = (info->dlpi_addr + ph->p_vaddr) & ~(page - 1);
            uintptr_t e = info->dlpi_addr + ph->p_vaddr + ph->p_memsz;
            if (mprotect((void*)a, e - a, PROT_READ | PROT_WRITE) != 0) perror("mprotect(rodata)");
        }
    }
    return 1;
}
#endif

/* The scratchpad's address is taken: name what holds it (on Windows usually a DLL another program
   injects, an overlay or an antivirus), so the player knows what to close */
static void spr_taken(void)
{
    char who[512] = "";
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((void*)(uintptr_t)PS2_SPR_BASE, &mbi, sizeof(mbi)) && mbi.Type == MEM_IMAGE)
        GetModuleFileNameA((HMODULE)mbi.AllocationBase, who, sizeof(who));
#else
    FILE* f = fopen("/proc/self/maps", "r");
    char line[600];
    unsigned long a, e;
    while (f && fgets(line, sizeof(line), f))
        if (sscanf(line, "%lx-%lx", &a, &e) == 2 && a < PS2_SPR_BASE + PS2_SPR_SIZE && e > PS2_SPR_BASE) {
            char* p = strchr(line, '/');
            if (p) { p[strcspn(p, "\n")] = 0; snprintf(who, sizeof(who), "%s", p); }
            break;
        }
    if (f) fclose(f);
#endif
#ifdef __ANDROID__
    {   /* the log is not open yet and a message box would never show in a headset app: say it in logcat (tag recvx) and end,
           instead of staying on the three loading dots */
        extern int __android_log_print(int, const char*, const char*, ...);
        __android_log_print(6, "recvx", "Cannot reserve the scratchpad at 0x%08X (used by: %s)\n", PS2_SPR_BASE, who[0] ? who : "?");
        _exit(3);
    }
#endif
    if (who[0]) host_fatal("Cannot reserve game memory at 0x%08X: it is used by\n%s\n\nClose the program that adds it (an overlay or antivirus), or restart the computer.", PS2_SPR_BASE, who);
    host_fatal("Cannot reserve game memory at 0x%08X. Restart the computer and try again.", PS2_SPR_BASE);
}

void ps2_memory_init(void)
{
#ifdef _WIN32
    void* spr = VirtualAlloc((void*)(uintptr_t)PS2_SPR_BASE, PS2_SPR_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (spr != (void*)(uintptr_t)PS2_SPR_BASE) spr_taken();
    {   /* the decomp writes into string literals: make the image's read-only data sections writable */
        uint8_t* base = (uint8_t*)GetModuleHandleA(NULL);
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
        IMAGE_SECTION_HEADER* sh = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++, sh++) {
            DWORD old, ch = sh->Characteristics;
            if (!(ch & IMAGE_SCN_MEM_READ) || (ch & (IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE))) continue;
            if (!VirtualProtect(base + sh->VirtualAddress, sh->Misc.VirtualSize, PAGE_READWRITE, &old))
                host_fatal("cannot make section %.8s writable", (const char*)sh->Name);
        }
    }
#else
    void* spr = mmap((void*)(uintptr_t)PS2_SPR_BASE, PS2_SPR_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (spr != (void*)(uintptr_t)PS2_SPR_BASE) spr_taken();
    dl_iterate_phdr(unprotect_cb, NULL);
#endif
    {
        extern char ps2_window_probe;
        uintptr_t end = 0;
#ifdef _WIN32
        uint8_t* base = (uint8_t*)GetModuleHandleA(NULL);
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
        IMAGE_SECTION_HEADER* sh = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++, sh++)   /* not .reloc, .debug_* */
            if (!(sh->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) && (uintptr_t)base + sh->VirtualAddress + sh->Misc.VirtualSize > end)
                end = (uintptr_t)base + sh->VirtualAddress + sh->Misc.VirtualSize;
#else
        extern char _end;
        end = (uintptr_t)&_end;
#endif
        if ((uintptr_t)&ps2_window_probe < PS2_WINDOW || (uintptr_t)&ps2_window_probe >= PS2_WINDOW_END)
            host_fatal("executable is not loaded inside the PS2 address window (0x%08X)", (unsigned)PS2_WINDOW);
        /* all static game memory (room arena, texture heap, ...) must lie in the window: the budgets
           in pc_prelude.h share the 256 MB up to 0x40000000 (docs/modding/ROOM_FORMAT.md "Limits and budgets") */
        if (end > PS2_WINDOW_END)
            host_fatal("the executable's data end at 0x%08X, past the PS2 address window (0x%08X)", (unsigned)end, (unsigned)PS2_WINDOW_END);
    }
}
char ps2_window_probe;

/* blocks of host memory outside the window that DMA reads (textures of our own: lang_glue.c), above the window
   (never a physical PS2 address below 0x10000000), which ps2_dma_ptr passes through as host pointers. One per
   slot; a new one replaces the slot's last. */
static struct { uint32_t base, len; } dma_host[4];
static void dma_unmap(void* p, uint32_t len)
{
#ifdef _WIN32
    (void)len;
    VirtualFree(p, 0, MEM_RELEASE);
#else
    munmap(p, len);
#endif
}
void* ps2_dma_alloc(int slot, uint32_t len)
{
    void* p;
    if (slot < 0 || slot >= 4) return NULL;
    if (dma_host[slot].base) {
        uint32_t n = dma_host[slot].len;
        dma_host[slot].len = 0;
        dma_unmap((void*)(uintptr_t)dma_host[slot].base, n);
        dma_host[slot].base = 0;
    }
    if (!len) return NULL;
#ifdef _WIN32
    p = VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE);
#else
    p = mmap((void*)(uintptr_t)PS2_WINDOW_END, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) p = NULL;
#endif
    if (p && (uintptr_t)p < PS2_WINDOW_END) { dma_unmap(p, len); p = NULL; }   /* (it would read as a PS2 address) */
    if (!p) return NULL;
    dma_host[slot].base = (uint32_t)(uintptr_t)p;
    dma_host[slot].len = len;
    return p;
}

/* DMA MADR/TADR/ADDR field -> host pointer */
void* ps2_dma_ptr(uint32_t addr)
{
    for (int i = 0; i < 4; i++) if (addr - dma_host[i].base < dma_host[i].len) return (void*)(uintptr_t)addr;
    if (addr & 0x80000000u) return (void*)(uintptr_t)(PS2_SPR_BASE | (addr & (PS2_SPR_SIZE - 1)));
    if ((addr & 0xF0000000u) == 0x70000000u) return (void*)(uintptr_t)(PS2_SPR_BASE | (addr & (PS2_SPR_SIZE - 1)));
    if (addr < 0x10000000u) addr |= PS2_WINDOW; /* physical address (masked with 0x0FFFFFFF) */
    return (void*)(uintptr_t)addr;
}

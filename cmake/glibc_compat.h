/* RECVX_GLIBC_COMPAT (Linux): force-included into every C/C++ file (also SDL, ImGui, ...) so the
   executable runs on older glibc. glibc 2.43 gave these math functions new symbol versions on
   i386; bind them to their original ones (same functions for our use). The rest is in
   glibc_compat.c; docs/BUILDING.md "Running on older distributions (glibc)". */
__asm__(".symver fmodf,fmodf@GLIBC_2.0");
__asm__(".symver fmod,fmod@GLIBC_2.0");
__asm__(".symver log10f,log10f@GLIBC_2.0");
__asm__(".symver atan2f,atan2f@GLIBC_2.0");
__asm__(".symver asinf,asinf@GLIBC_2.0");
__asm__(".symver acosf,acosf@GLIBC_2.0");
__asm__(".symver sqrtf,sqrtf@GLIBC_2.0");

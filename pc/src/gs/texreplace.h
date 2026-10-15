/* Texture replacements: HD packs made for PCSX2 (texreplace.c). */
#pragma once
#include <stdint.h>

extern int g_tex_hd, g_tex_hd_log, g_tex_hd_mb;
int trep_active(void);   /* packs found (or name logging on) */
/* the replacement for a texture in (scratch) GS memory and the current CLUT buffer; GL name or 0 */
unsigned int trep_lookup(uint64_t tex0, uint64_t texa);
int trep_kind(unsigned int gl_tex);   /* 0 not a replacement, 1 without mipmaps, 2 with */
/* textures.c: forget native entries that use this GL texture (it is being deleted) */
void ntex_drop_gl(unsigned int gl_tex);
void gs_texcache_drop_gl(unsigned int gl_tex);   /* gs_gl.c, the same for its own cache */

/* Files with UTF-8 paths on every platform (SDL hands out UTF-8; on Windows the narrow CRT
   functions would take the ANSI code page instead), and saving by replace-on-rename. */
#pragma once
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

FILE* pc_fopen(const char* utf8_path, const char* mode);
int   pc_chdir(const char* utf8_path);   /* 1 = changed */
/* Writes utf8_path through a temporary file next to it: fill() writes and returns 0 on success;
   the data is flushed to disk and renamed over the file (rename on POSIX, MoveFileEx on
   Windows). On any error the file is left as it was. Returns 0 on success, -1 on error. */
int pc_save_file(const char* utf8_path, const char* mode, int (*fill)(FILE* f, void* ud), void* ud);
/* main()'s arguments as UTF-8 (Windows: from the wide command line; elsewhere argv itself) */
char** pc_utf8_argv(int argc, char** argv);

#ifdef __cplusplus
}
#endif

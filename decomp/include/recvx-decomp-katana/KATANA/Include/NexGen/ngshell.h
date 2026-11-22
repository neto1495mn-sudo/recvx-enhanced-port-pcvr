/*****************************************************************************
 * $Id: ngshell.h,v 1.5 2000/09/19 08:56:02 af Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * embedded shell interface
 *----------------------------------------------------------------------------
 *		Copyright (c) 2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 09/08/2000 - Regis Feneon
 * 11/09/2000 - Adrien Felon
 *  added shell command for debug management (ngShellCmd_debug)
 * 19/09/2000
 *  added new flag definitions in sh_flags for command completion
 *****************************************************************************/

#ifndef __NG_NGSHELL_H_INCLUDED__
#define __NG_NGSHELL_H_INCLUDED__

#include <ngos.h>
#include <ngos/keydef.h>

NG_BEGIN_DECLS

#define NG_SH_MAX_ARG       16

struct NGshellctx_S;

/* shell command definition */
typedef struct {
    char    *cmd_label;     /* name of command */
    int    (*cmd_callback)( struct NGshellctx_S *ctx,   /* function */
                            int argc,
                            char **argv);
    char    *cmd_help;      /* help message */
} NGshellcmd;

typedef struct NGshellctx_S {
  int             sh_flags;       /* some flags: */
#define NG_SHF_INSERT   0x0001      /*  insert mode */
#define NG_SHF_EXIT     0x0002      /*  exit from shell */
#define NG_SHF_NOCOMPL  0x0004      /*  disable completion */
#define NG_SHF_TABSEEN  0x0008      /*  to trigger double tab (completion) */
  const NGshellcmd  **sh_cmdlist;     /* list of commands */
  void          (*sh_outc_f)( int, void *);   /* output function */
  void           *sh_outc_data;               /* data passed to output func */
  char           *sh_buf;         /* command buffer */
  int             sh_cmdsize;     /* maximum command line size */
  int             sh_histmax;     /* max number of lines in history */
  int             sh_curidx;      /* current edited line */
  int             sh_curpos;      /* cursor position */
  int             sh_argc;                /* number of arguments */
  char           *sh_argv[NG_SH_MAX_ARG]; /* list of arguments */
  const char     *sh_prompt;      /* prompt string */
  int             sh_esc;         /* used by input filter */
  int             sh_lastret;     /* last return value */
} NGshellctx;

/* user functions */
int  ngShellInit( NGshellctx *ctx, const NGshellcmd **cmdlist, void *buf,
        int cmdmax, int histmax, void (*outchar)( int, void *),
        void *outdata, const char *prompt);
int  ngShellKey( NGshellctx *ctx, int keycode);
int  ngShellExit( NGshellctx *ctx);
int  ngShellGetLastRet( NGshellctx *ctx);
const NGshellcmd *ngShellFindCmd( NGshellctx *ctx, const char *cmd);
void ngShellPutChar( NGshellctx *ctx, int c);
void ngShellFlush( NGshellctx *ctx);
int  ngShellPrintf( NGshellctx *ctx, const char *fmt, ...);
int  ngShellVPrintf( NGshellctx *ctx, const char *fmt, NGva_list args);
void ngShellPrintHelp( NGshellctx *ctx, const NGshellcmd *cmd);

/* ansi codes support */
int  ngShellKeyAnsi( NGshellctx *ctx, char c);
int  ngShellKeyPC( NGshellctx *ctx, char c);

/* standard commands */
extern const NGshellcmd ngShellCmd_help;    /* display help information */
extern const NGshellcmd ngShellCmd_ver;     /* display version string */
extern const NGshellcmd ngShellCmd_exit;    /* terminate shell session */
extern const NGshellcmd ngShellCmd_devstat; /* display devices statistics */
extern const NGshellcmd ngShellCmd_debug;   /* manage debug log */

NG_END_DECLS

#endif


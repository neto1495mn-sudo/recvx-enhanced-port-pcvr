/* Co-op lobby (docs/COOP.md "Lobby"): the title menu's MULTIPLAYER entry and its screen.
 *
 * Game thread, called by the title task (adv.c Adv_BioCvTitle; mode 30 is the lobby). All of
 * it is drawn with the game's own means: the title picture and plates (the MULTIPLAYER row is
 * set from letters of the plate texture), the game's font (sys->ef_tlist, ASCII 32..126 as
 * codes 0..94), and the player and actor models, loaded with the stock loaders into buffers of
 * our own and posed, lit and drawn in a small scene of their own in front of the picture.
 * Networking stays in pc/glue/coop_glue.c: the lobby only sets g_coop and requests.
 */
#include "../src/host/lang.h"
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/adv.h"
#include "../../decomp/include/ps2/veronica/prog/adxwrap.h"
#include "../../decomp/include/ps2/veronica/prog/padman.h"
#include "../../decomp/include/ps2/veronica/prog/message.h"
#include "../../decomp/include/ps2/veronica/prog/dread.h"
#include "../../decomp/include/ps2/veronica/prog/Motion.h"
#include "../../decomp/include/ps2/veronica/prog/MdlPut.h"
#include "../../decomp/include/ps2/veronica/prog/objitm.h"
#include "../../decomp/include/ps2/veronica/prog/sdfunc.h"
#include "../../decomp/include/ps2/veronica/prog/effect.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_texture.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaTextureFunction.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaView.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaDraw.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NinjaCnk.h"
#include "../src/host/coop.h"
#include "../src/host/actors.h"
#include "../src/host/menu_mouse.h"
#include "../src/host/lobby.h"
#include "../src/host/settings.h"
#include "../src/host/enemies.h"
#include "../src/net/net.h"
#include "../src/net/rdv_proto.h"
#include "../src/net/coop_proto.h"   /* REC_*: what the host keeps for us */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void host_log(const char* fmt, ...);
void bhDispFontEx(NJS_POINT2* pos, int code, unsigned int argb, float pri);
void coop_own_from_save(const void* save);
void bhReturnTitle(void);
extern ADV_WORK AdvWork;
extern float FontScaleY;
extern const char PlyFlip[23];

enum { BTN_START = 0x800, BTN_UP = 0x1000, BTN_RIGHT = 0x2000, BTN_DOWN = 0x4000, BTN_LEFT = 0x8000 };   /* Pad[] bits */
#define TITLE_MP 4                      /* ModeCommandId of the MULTIPLAYER row */
#define TITLE_EXIT 5                    /* vr145: ... of the EXIT row (SAIR, to the desktop) */
void pc_quit_desktop(void);             /* settings.c */
static int exit_armed;                  /* vr145: EXIT pressed once - the second press quits */

/* ---------------------------------------------------------------- text */
#define PRI_TEXT -1.0f
/* 2D depth (njDrawPolygon / njDrawQuadTexture z: the GS depth is -1/z, as a 3D point at view
   distance 1/z): the title picture is at 0.01 (100 units), the models stand 34 units away */
#define Z_DIM 0.012f                    /* behind the models */
#define Z_FRONT 0.3f                    /* in front of them */

static int code_of(char c) { return c > 32 && c < 127 ? c - 32 : 0; }
static float adv(int c) { return c ? FontSz[c] : 11.0f; }   /* the font's space is a full cell */

#define TRACK 0.88f                     /* a little tighter than the game's messages */

static float text_w(const char* s, float sc)
{
    float w = 0;
    for (; *s; s++) w += adv(code_of(*s)) * sc * TRACK;
    return w;
}

/* the game's font, left-aligned at x (bhDispMessage's advance), with a drop shadow */
static int font_ok;                     /* the game's font is loaded (font_ready) */
static int font_state;                  /* lobby_glue loads it: 1 reading, 2 loaded by us */

static void text(float x, float y, const char* s, float sc, unsigned int argb)
{
    float kx = FontScaleX, ky = FontScaleY, kc = FontScaleCR, x0 = x;
    const char* p;
    int pass;
    if (!font_ok) return;
    bhFontScaleSet(sc, sc, sc);
    for (pass = 0; pass < 2; pass++)
        for (p = s, x = x0; *p; p++) {
            int c = code_of(*p);
            NJS_POINT2 pos = { x + (pass ? 0 : 2), y + (pass ? 0 : 2) };
            if (c) bhDispFontEx(&pos, c, pass ? argb : (argb & 0xFF000000) >> 1 & 0xFF000000, PRI_TEXT);
            x += adv(c) * sc * TRACK;
        }
    bhFontScaleSet(kx, ky, kc);
}

static void text_c(float cx, float y, const char* s, float sc, unsigned int argb) { text(cx - text_w(s, sc) / 2, y, s, sc, argb); }

/* at most `w` wide: the words that fit, then the next line (returns the lines drawn) */
static int text_wrap(float x, float y, float w, const char* s, float sc, unsigned int argb, int max_lines)
{
    char line[160];
    int n = 0;
    while (*s && n < max_lines) {
        int len = 0, fit = 0;
        while (s[len] && len < (int)sizeof(line) - 1) {
            memcpy(line, s, len + 1); line[len + 1] = 0;
            if (text_w(line, sc) > w) break;
            len++;
            if (!s[len] || s[len] == ' ') fit = len;
        }
        if (!s[len]) fit = len;
        if (!fit) fit = len ? len : 1;
        memcpy(line, s, fit); line[fit] = 0;
        text(x, y + n * 28 * sc * 1.05f, line, sc, argb);
        n++;
        s += fit;
        while (*s == ' ') s++;
    }
    return n;
}

static void rect(float x0, float y0, float x1, float y1, unsigned int argb, float z)
{
    NJS_POLYGON_VTX v[4] = { { x0, y0, z, argb }, { x0, y1, z, argb }, { x1, y0, z, argb }, { x1, y1, z, argb } };
    njDrawPolygon(v, 4, 1);
}

/* a small arrow head pointing left (dir -1) or right (1), its tip at x */
static void arrow(float x, float y, int dir, unsigned int argb)
{
    NJS_POLYGON_VTX v[3] = { { x, y, 0.3f, argb }, { x - dir * 7.0f, y - 6.0f, 0.3f, argb }, { x - dir * 7.0f, y + 6.0f, 0.3f, argb } };
    njDrawPolygon(v, 3, 1);
}

/* ---------------------------------------------------------------- title menu */
/* The MULTIPLAYER row in the plates' own lettering: each letter cut from a plate of the title
   texture (texture 1: LOAD GAME at v 64, OPTION 128, SOUND MUSEUM 192, EASY 288, NORMAL 320;
   u = the letter's left edge, w its width, measured from the texture). */
static const struct { short u, v, w; } mp_letters[] = {
    { 312, 64, 25 },  /* M of GAME */
    { 174, 192, 23 }, /* U of SOUND */
    { 160, 64, 20 },  /* L of LOAD */
    { 246, 128, 21 }, /* T of OPTION */
    { 268, 128, 12 }, /* I */
    { 224, 128, 21 }, /* P */
    { 160, 64, 20 },  /* L */
    { 286, 64, 24 },  /* A of GAME */
    { 107, 288, 23 }, /* Y of EASY */
    { 339, 64, 20 },  /* E of GAME */
    { 67, 320, 23 },  /* R of NORMAL */
};

/* vr151: the Brazilian translation's plate texture (the same rows, other words: CARREGAR PARTIDA 64, NOVA PARTIDA 96,
   OPÇÕES 128, MUSEU DE SOM 192, MUITO FÁCIL 256, NORMAL 320, STEVE 352; measured from the disc). It has no Y: the top
   half of STEVE's V over the bottom half of MUITO's I (bu/bv/bw). */
typedef struct { short u, v, w, bu, bv, bw; } PlateLetter;
static const PlateLetter mp_letters_br[] = {
    { 144, 192, 24 }, /* M of MUSEU */
    { 171, 192, 21 }, /* U of MUSEU */
    { 141, 320, 19 }, /* L of NORMAL */
    { 88, 352, 19 },  /* T of STEVE */
    { 50, 256, 10 },  /* I of MUITO */
    { 225, 128, 19 }, /* P of OPÇÕES */
    { 141, 320, 19 }, /* L */
    { 117, 320, 22 }, /* A of NORMAL */
    { 131, 352, 22, 50, 256, 10 }, /* Y: V of STEVE over I of MUITO */
    { 110, 352, 18 }, /* E of STEVE */
    { 68, 320, 21 },  /* R of NORMAL */
};
static const PlateLetter exit_letters_br[] = {
    { 69, 352, 16 },  /* S of STEVE */
    { 117, 320, 22 }, /* A of NORMAL */
    { 50, 256, 10 },  /* I of MUITO */
    { 68, 320, 21 },  /* R of NORMAL */
};
static const PlateLetter exit_letters_us[] = {
    { 130, 192, 20 }, /* S of SOUND (its place estimated from the U next to it) */
    { 286, 64, 24 },  /* A of GAME */
    { 268, 128, 12 }, /* I of OPTION */
    { 67, 320, 23 },  /* R of NORMAL */
};

/* which plate texture the title has: seen on the disc when it opens (font_glue.c plates_from_iso) */
extern int g_plates_br;
static int plates_lang = -1;            /* -1 not logged yet, 0 the US texture (aikatore's letters), 1 Brazilian */
static void plates_check(void)
{
    if (plates_lang >= 0) return;
    plates_lang = g_plates_br != 0;
    host_log("title: plate texture %s\n", plates_lang ? "Brazilian - its own letters" : "US - aikatore's letters");
}

static void plate_word(const PlateLetter* L, int n, float x, float y, unsigned int col, unsigned int shadow, float scale)
{
    QUAD q;
    int tex = lang_us_plates(2) ? 0 : 1;   /* (every language's picture: our words from the US plates) */
    for (int i = 0; i < n; i++) {
        if (L[i].bw) {   /* two halves: the top of one letter over the bottom of another, centred */
            SetQuadPos(x, y, L[i].w * scale, 18.0f * scale, &q);   /* vr153: the V down to 18 (it was cut at 16) */
            SetQuadUv2(L[i].u, L[i].v, L[i].w, 18.0f, 1, &q);
            AdvEasyDrawTextureS(tex, col, &q, 0.5f, 1, shadow);
            SetQuadPos(x + (L[i].w - L[i].bw) * scale / 2, y + 17.0f * scale, L[i].bw * scale, 16.0f * scale, &q);   /* (one row over) */
            SetQuadUv2(L[i].bu, L[i].bv + 17, L[i].bw, 16.0f, 1, &q);
            AdvEasyDrawTextureS(tex, col, &q, 0.5f, 1, shadow);
        } else {
            SetQuadPos(x, y, L[i].w * scale, 33.0f * scale, &q);
            SetQuadUv2(L[i].u, L[i].v, L[i].w, 33.0f, 1, &q);
            AdvEasyDrawTextureS(tex, col, &q, 0.5f, 1, shadow);
        }
        x += (L[i].w + 1) * scale;
    }
    if (!tex) AdvEasySetTextureList(0);   /* the title's textures again */
}
static float plate_w(const PlateLetter* L, int n, float scale) { float w = 0; for (int i = 0; i < n; i++) w += (L[i].w + 1) * scale; return w; }

static void exit_word(float x1, float y, unsigned int col, unsigned int shadow, float scale)
{
    const PlateLetter* L = plates_lang == 1 ? exit_letters_br : exit_letters_us;
    plates_check();
    plate_word(L, 4, x1 - plate_w(L, 4, scale), y, col, shadow, scale);
}

static void mp_word(float cx, float y, unsigned int col, unsigned int shadow, float scale)
{
    float w = 0, x;
    int i;
    QUAD q;
    plates_check();
    if (plates_lang == 1) {   /* vr151 */
        int n = (int)(sizeof(mp_letters_br) / sizeof(mp_letters_br[0]));
        plate_word(mp_letters_br, n, cx - plate_w(mp_letters_br, n, scale) / 2, y, col, shadow, scale);
        return;
    }
    for (i = 0; i < (int)(sizeof(mp_letters) / sizeof(mp_letters[0])); i++) w += (mp_letters[i].w + 1) * scale;
    x = cx - w / 2;
    for (i = 0; i < (int)(sizeof(mp_letters) / sizeof(mp_letters[0])); i++) {
        SetQuadPos(x, y, mp_letters[i].w * scale, 33.0f * scale, &q);
        SetQuadUv2(mp_letters[i].u, mp_letters[i].v, mp_letters[i].w, 33.0f, 1, &q);
        AdvEasyDrawTextureS(1, col, &q, 0.5f, 1, shadow);
        x += (mp_letters[i].w + 1) * scale;
    }
}

/* the rows between the logo (to y 235) and the copyright (from y 396): 4 rows as the game's
   own 4-row layout but 16 higher, 5 rows closer together */
static float title_row_y(int i, int n) { return n >= 5 ? 234.0f + 30.0f * i : 256.0f + 32.0f * i; }

int pc_battle_unlocked(void);   /* host.c: the GAMEPLAY setting */

void pc_title_menu_init(void)
{
    ADV_WORK* ap = &AdvWork;
    int n;
    if (pc_battle_unlocked() && !ap->ExtraFlag) {   /* setting: BATTLE GAME as after a cleared game; the system data's clear flag (ssd_flg 1) stays as it is */
        ap->ExtraFlag = 1;
        ap->CursorMax[0] = 4;
        ap->ModeCommandId[0] = 0; ap->ModeCommandId[1] = 1; ap->ModeCommandId[2] = 3; ap->ModeCommandId[3] = 2;
    }
    n = ap->CursorMax[0];
    ap->ModeCommandId[n] = TITLE_MP;
    ap->CursorFlag[0][n] = 1;
    ap->ModeCommandId[n + 1] = TITLE_EXIT;   /* vr145: SAIR, small at the right under MULTIPLAYER (not a row of its own) */
    ap->CursorFlag[0][n + 1] = 1;
    ap->CursorMax[0] = n + 2;
    exit_armed = 0;
}

/* vr153: SAIR's "press again" line in letters of our own (5x7 blocks as rectangles, no texture) */
static const struct { char c; unsigned char row[7]; } block_font[] = {
    { 'A', { 14, 17, 17, 31, 17, 17, 17 } }, { 'D', { 30, 17, 17, 17, 17, 17, 30 } }, { 'E', { 31, 16, 16, 30, 16, 16, 31 } },
    { 'I', { 14, 4, 4, 4, 4, 4, 14 } },      { 'L', { 16, 16, 16, 16, 16, 16, 31 } }, { 'M', { 17, 27, 21, 21, 17, 17, 17 } },
    { 'N', { 17, 25, 21, 19, 17, 17, 17 } }, { 'O', { 14, 17, 17, 17, 17, 17, 14 } }, { 'P', { 30, 17, 17, 30, 16, 16, 16 } },
    { 'R', { 30, 17, 17, 30, 20, 18, 17 } }, { 'S', { 15, 16, 16, 14, 1, 1, 30 } },    { 'T', { 31, 4, 4, 4, 4, 4, 4 } },
    { 'U', { 17, 17, 17, 17, 17, 17, 14 } }, { 'V', { 17, 17, 17, 17, 17, 10, 4 } },  { 'Y', { 17, 17, 10, 4, 4, 4, 4 } },
    { '?', { 14, 17, 1, 2, 4, 0, 4 } },
};
static float block_w(const char* s, float px) { return (float)strlen(s) * 6.0f * px - px; }
static void block_text(float x, float y, const char* s, float px, unsigned int argb)
{
    unsigned int shadow = (argb & 0xFF000000) >> 1 & 0xFF000000;
    for (int pass = 0; pass < 2; pass++)
        for (const char* p = s; *p; p++) {
            float cx = x + (float)(p - s) * 6.0f * px + (pass ? 0.0f : px * 0.6f), cy = y + (pass ? 0.0f : px * 0.6f);
            for (int k = 0; k < (int)(sizeof(block_font) / sizeof(block_font[0])); k++) {
                if (block_font[k].c != *p) continue;
                for (int r = 0; r < 7; r++)
                    for (int c = 0; c < 5; c++)
                        if (block_font[k].row[r] & (16 >> c))
                            rect(cx + c * px, cy + r * px, cx + (c + 1) * px, cy + (r + 1) * px, pass ? argb : shadow, Z_FRONT);
                break;
            }
        }
}
/* vr145: adv.c, SAIR confirmed - the first press asks (it blinks red), the second quits */
void pc_title_exit(void)
{
    if (!exit_armed) { exit_armed = 1; CallSystemSe(0, 3); host_log("title: exit asked\n"); return; }
    host_log("title: exit to the desktop\n");
    CallSystemSe(0, 3);
    pc_quit_desktop();
}

void pc_title_font_leave(void) {}   /* (vr146-149's title font is gone; adv.c still calls it) */

/* DisplayGameModePlate with the MULTIPLAYER row (the game's 3 or 4 rows moved up to make room) */
void pc_title_plates(int cursor, int n, float fade, int flag)
{
    static const float vdef[2][4] = { { 64.0f, 96.0f, 128.0f, 224.0f }, { 64.0f, 96.0f, 224.0f, 128.0f } };
    ADV_WORK* ap = &AdvWork;
    int i, extra = ap->ExtraFlag != 0;
    for (i = 0; i < n; i++) {
        unsigned int alpha = fade, col = 0x7F7F7F;
        float y = title_row_y(i < n - 1 ? i : n - 2, n - 1);   /* vr150: SAIR is not a row of its own */
        if (i == cursor) { alpha += (unsigned int)ap->FlushCount2; col = ap->FontBaseColor; }
        alpha <<= 24;
        if (flag == 0) alpha = 0xFF000000;
        if (ap->ModeCommandId[i] == TITLE_MP) mp_word(320.0f, y, alpha | col, fade, 1.0f);
        else if (ap->ModeCommandId[i] == TITLE_EXIT) {
            exit_word(560.0f, y + 24.0f, alpha | (exit_armed && i == cursor ? 0x7F2010 : col), fade, 0.6f);
            if (exit_armed && i == cursor) {   /* vr153: say so (the red alone was not seen) */
                static const char* const m = "APERTE DE NOVO PARA SAIR";
                njColorBlendingMode(0, 8); njColorBlendingMode(1, 6);
                block_text(500.0f - block_w(m, 1.6f), y + 32.0f, m, 1.6f, (alpha & 0xFF000000) | 0xFF6040);
            }
        }
        else {
            QUAD q;
            SetQuadPos(64.0f, y, 513.0f, 33.0f, &q);
            SetQuadUv2(0, vdef[extra][i < 4 ? i : 3], 513.0f, 33.0f, 1, &q);
            AdvEasyDrawTextureS(1, alpha | col, &q, 0.5f, 1, fade);
        }
    }
    if (ap->ModeCommandId[cursor] != TITLE_EXIT) exit_armed = 0;   /* vr145: moved away - asked again */
    FlushPlate();
    DisplayTitleBg();
}

/* ---------------------------------------------------------------- mouse */
static MouseTake mt;                    /* the pointer (game 2D px) and what it did */

void pc_title_mouse_active(int on)
{
    g_menu_mouse.active = on;
}

void menu_mouse_take(MouseTake* t)
{
    int x = g_menu_mouse.x, y = g_menu_mouse.y;
    t->left = __atomic_exchange_n(&g_menu_mouse.left, 0, __ATOMIC_RELAXED);
    t->right = __atomic_exchange_n(&g_menu_mouse.right, 0, __ATOMIC_RELAXED);
    t->wheel = __atomic_exchange_n(&g_menu_mouse.wheel, 0, __ATOMIC_RELAXED);
    __atomic_exchange_n(&g_menu_mouse.left_up, 0, __ATOMIC_RELAXED);
    t->moved = (x != t->px || y != t->py) && t->seen;
    t->px = x; t->py = y; t->seen = 1;
    t->x = x / 16.0f; t->y = y / 16.0f;
    if (!menu_mouse_on()) t->left = t->right = t->wheel = t->moved = 0;
}

/* CheckButton (before it reads the pad): hovering a row moves the cursor to it, a click on it
   confirms (as Start), a right click cancels. Level 0 the game modes, 1 the difficulty, 2 the
   extra games. */
void pc_title_mouse(int level, int flag)
{
    ADV_WORK* ap = &AdvWork;
    int hit = -1, i, n = ap->CursorMax[level];
    menu_mouse_take(&mt);
    for (i = 0; i < n; i++) {
        float x0, y0, w;
        if (level == 0 && AdvWork.ModeCommandId[i] == TITLE_EXIT) { x0 = 500; w = 60; y0 = title_row_y(n - 2, n - 1) + 24; }   /* vr150 */
        else if (level == 0) { x0 = 140; w = 360; y0 = title_row_y(i, n - 1); }
        else if (level == 1) { x0 = 224; w = 193; y0 = 304.0f + 32.0f * i; }   /* DisplayNewGamePlate, DisplayExtraGamePlate */
        else { x0 = 64.0f + 256.0f * i; w = 256; y0 = 352; }
        if (mt.x >= x0 && mt.x < x0 + w && mt.y >= y0 && mt.y < y0 + (level == 0 && n - 1 >= 5 ? 30 : 32)) hit = i;
    }
    if (hit >= 0 && (!ap->CursorFlag[level][hit] || (level == 0 && hit == 0 && !flag))) hit = -1;   /* not selectable now */
    if (hit >= 0 && (mt.moved || mt.left) && hit != ap->Cursor[level]) {
        ap->Cursor[level] = hit;
        CallSystemSe(0, 2);
    }
    if (mt.left && hit >= 0) Pad[ap->PortId].press |= BTN_START;
    else if (mt.right) Pad[ap->PortId].press |= AdvGetCancelButton();
}

/* ---------------------------------------------------------------- previews */
/* Two models: ours (slot 0) and the partner's (slot 1), each a player part file loaded with
   bhReadPlayerData into buffers of its own (as coop_glue.c does for the partner in game) and,
   for an enemy/NPC model, the display actor of the same slot (actor_glue.c) posed from it. The
   actor slots are free at the title (co-op uses them only in game). The bodies have no hands: in
   game they are the weapon's pieces linked to hand nodes 9 and 13 (sys->obwp[0..1]), unarmed those
   of weapon file 0 (bhReadWeaponData). Here the pieces of the character the model is (appear_voice:
   the cast their own, other men Chris's, women Claire's), on a display actor where its hands are
   (actor_pose moves the hand nodes there) and it has none of its own (the story cast). */
typedef struct Preview {
    BH_PWORK w;
    O_WRK obj[3];                       /* [0], [1]: right/left hand, [2]: Claire's hair / Wesker's piece, as sys->obwp[0..2] */
    int chr, file, hands;               /* "You appear as" shown; part file - 10 loaded; whose hands (-1 none) */
    int tex_gone;
    int fail_chr, fails;                /* a choice whose part file could not be read: tried again after a pause */
    unsigned int fail_at;
    unsigned char exw[128], hdw[128];
} Preview;
static Preview pv[2] = { { .chr = -1, .file = -1, .hands = -1, .fail_chr = -1 }, { .chr = -1, .file = -1, .hands = -1, .fail_chr = -1 } };
static struct {
    unsigned char mdl[131072], bmt[393216], mth[12288], zmt[8192], etc[32768];
    unsigned char wr[32768], wl[32768], wmt[65536];   /* the hands (sys->wrmdlp, wlmdlp, plwmtp) */
    unsigned char hair0[0x4000], hair3[0x1000];
} pvb[2] __attribute__((aligned(64)));
static unsigned char pv_file[1 << 21] __attribute__((aligned(64)));

static void pv_hands_drop(Preview* p)
{
    int i;
    for (i = 0; i < 2; i++) {
        ML_WORK* m = p->obj[i].mlwP;
        if (!p->tex_gone && m && m->texP && (m->flg & 0x200)) njReleaseTexture(m->texP);
        memset(&p->obj[i], 0, sizeof(p->obj[i]));
    }
    p->hands = -1;
}

static void pv_release(Preview* p, int k)
{
    int i;
    if (!p->tex_gone)
        for (i = 0; i < p->w.mdl_n; i++)
            if (p->w.mdl[i].texP) njReleaseTexture(p->w.mdl[i].texP);
    pv_hands_drop(p);
    if (!p->tex_gone && p->w.mdl_n) bhGarbageTexture(NULL, 0);
    actor_set(k, -1);
    memset(&p->w, 0, sizeof(p->w));
    memset(p->obj, 0, sizeof(p->obj));
    p->chr = p->file = -1;
    p->tex_gone = 0;
}

static void pv_failed(Preview* p, int chr)
{
    p->fails = chr == p->fail_chr ? p->fails + 1 : 1;
    p->fail_chr = chr;
    p->fail_at = net_ms();
}

/* the hand pieces of player character c (weapon file 0's, as the game loads them unarmed) on slot k's model */
static void pv_hands(Preview* p, int k, int c)
{
    int i;
    if (c == p->hands) return;
    pv_hands_drop(p);
    bhGarbageTexture(NULL, 0);
    for (i = 0; i < 2; i++) p->obj[i].mlwP = p->obj[i].mdl;   /* (the loader releases this one's textures: none) */
    if (!afs_read(sys->sys_partid, c * 30 + 20, pv_file, sizeof(pv_file))) { host_log("lobby: cannot read weapon file %d\n", c * 30 + 20); return; }
    player_weapon_read(&p->w, p->obj, 0, pv_file, pvb[k].wr, pvb[k].wl, pvb[k].wmt);   /* (actor_glue.c) */
    p->hands = c;
}

/* "You appear as" chr (g_coop.chr) for preview slot k; `played`: the character "as played" means */
static void pv_set(int k, int chr, int played)
{
    Preview* p = &pv[k];
    int file = chr == 0 ? played : chr <= 4 ? chr - 1 : chr == 5 ? 4 : played & 3, i, ok = 1;
    if (chr == p->chr) return;
    if (chr < 0) { pv_release(p, k); return; }
    if (chr == p->fail_chr && net_ms() - p->fail_at < (1000u << (p->fails < 5 ? p->fails : 5))) return;   /* 2, 4 .. 32 s */
    if (file != p->file) {
        pv_release(p, k);
        if (!afs_read(sys->sys_partid, file + 10, pv_file, sizeof(pv_file))) { host_log("lobby: cannot read part file %d\n", file + 10); pv_failed(p, chr); return; }
        memset(&pvb[k], 0, sizeof(pvb[k]) - sizeof(pvb[k].hair0) - sizeof(pvb[k].hair3));
        player_model_read(&p->w, pv_file, pvb[k].mdl, pvb[k].bmt, pvb[k].mth, pvb[k].zmt);   /* (actor_glue.c) */
        for (i = 0; i < p->w.mdl_n; i++) ok &= p->w.mdl[i].owP != NULL;
        if (!p->w.mdl_n || !ok) { host_log("lobby: part file %d did not load\n", file + 10); pv_release(p, k); pv_failed(p, chr); return; }
        p->w.flg = 0x119;
        p->w.mdflg = 0x20;
        p->w.exp0 = p->exw; p->w.exp1 = p->hdw; p->w.exp3 = pvb[k].etc;
        p->w.mtx = (float(*)[16])p->w.mtxbuf;
        p->w.sx = p->w.sy = p->w.sz = p->w.sxb = p->w.syb = p->w.szb = 1.0f;
        p->w.mtn_tp = (unsigned char*)PlyFlip;
        p->w.mlwP->owP[7].flg |= 0x8;   /* as bhSetPlayer */
        p->w.mlwP->owP[11].flg |= 0x8;
        p->w.mtn_no = p->w.mnwP && p->w.mnwP[42].datP ? 42 : 0;   /* standing (PlMtnAct, as bhSetPlayer) */
        p->file = file;
    }
    p->chr = chr;
    p->fail_chr = -1;
    p->fails = 0;
    if (actor_set(k, chr >= COOP_CHR_ACTOR ? chr - COOP_CHR_ACTOR : -1)) p->obj[2].flg = 0;   /* no hair on another model */
    else player_hair(p->obj, &p->w, file & 3, pvb[k].hair0, pvb[k].hair3);   /* Claire's hair simulates in work of its own */
    pv_hands(p, k, appear_voice(chr) >= 0 ? appear_voice(chr) : file & 3);
    host_log("lobby: preview %d shows %s\n", k, chr >= COOP_CHR_ACTOR ? actor_models[chr - COOP_CHR_ACTOR].name : "a player model");
}

static void pv_pose(int k, float x, int ay)
{
    Preview* p = &pv[k];
    O_WRK* op = &p->obj[2];
    int i;
    if (p->file < 0) return;
    p->w.px = x; p->w.py = 0; p->w.pz = 0;
    p->w.ax = p->w.az = 0; p->w.ay = ay;
    if (p->w.mnwP && p->w.mnwP[p->w.mtn_no].datP) bhSetMotion(&p->w, 65536, 0, NULL);
    bhCalcModel(&p->w);
    if (op->flg & 0x1) {
        njCalcPoint(&p->w.mlwP->owP[op->lkono].mtx, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&op->px);
        if (op->id == 1211) bhObjClpn(op);
        bhCalcModel((BH_PWORK*)op);
    }
    if (actor_shown(k)) actor_pose(k, &p->w);   /* (moves hand nodes 9 and 13 to the model's hands) */
    for (i = 0; i < 2; i++)
        if ((p->obj[i].flg & 0x1) && p->obj[i].mlwP && p->obj[i].mlwP->objP) bhCalcModel((BH_PWORK*)&p->obj[i]);
}

static void pv_draw(int k)
{
    Preview* p = &pv[k];
    O_WRK* op;
    if (p->file < 0) return;
    if (actor_shown(k)) actor_draw(k);
    else bhPutModel(&p->w);
    for (op = p->obj; op < p->obj + 3; op++)   /* hands (on a display actor where it has none), hair (not there) */
        if ((op->flg & 0x1) && op->mlwP && op->mlwP->objP && (!actor_shown(k) || actor_hand_free(k, op->lkono))) bhPutModel((BH_PWORK*)op);
}

/* the model shown in slot k (the display actor, or the player model) */
static BH_PWORK* pv_shown(int k) { return actor_shown(k) && actor_work(k) ? actor_work(k) : &pv[k].w; }

/* the top of the posed model (world y), 0 when nothing is shown */
static float pv_top(int k)
{
    BH_PWORK* w = pv_shown(k);
    ML_WORK* m = w->mlwP;   /* the model bhPutModel draws */
    float top = 0;
    int i;
    if (pv[k].file < 0 || !m || !m->owP || !m->objP) return 0;
    for (i = 0; i < (int)m->obj_num; i++) if (m->owP[i].mtx[13] > top) top = m->owP[i].mtx[13];
    return top;
}

/* above the model's top: where its name goes (game 2D px) */
static NJS_POINT2 pv_head(int k)
{
    NJS_POINT3 h = { pv[k].w.px, pv_top(k) + 2.0f, pv[k].w.pz };
    NJS_POINT2 s;
    njProjectScreen((NJS_MATRIX*)cam.mtx, &h, &s);
    s.x -= 1728.0f; s.y -= 1808.0f;
    return s;
}

/* the scene: the camera looks along -z at models standing at the origin's height, lit by an
   ambient and one directional light; cam.mtx is the view while it is drawn */
/* the camera stands back for a tall model: the taller one of both spans y 110..370 */
static float cam_z = 34.0f;
#define CAM_Y (cam_z * 0.26f)
#define CAM_Z cam_z
static NJS_MATRIX view;
static float (*cam_keep)[16];           /* the game's view (none before a game ran) */

static float screen_to_world_x(float sx) { return (sx - 320.0f) * CAM_Z / 500.0f / 1.174f; }

static void scene_begin(void)
{
    NJS_SCREEN sc = { 500.0f, 640.0f, 480.0f, 320.0f, 240.0f };
    cam_keep = cam.mtx;
    cam.mtx = (float(*)[16])&view;   /* bhPutModel draws with cam.mtx */
    njSetScreen(&sc);
    njSetAspect(1.174f, 1.0f);
    njSetMatrix(NULL, cmat);
    njTranslate(NULL, 0, -CAM_Y, -CAM_Z);
    njGetMatrix(cam.mtx);
    njControl3D(0);
    njCnkSetEasyMultiAmbient(0.42f, 0.42f, 0.45f);
    njCnkSetSimpleMultiAmbient(0.42f, 0.42f, 0.45f);
    njCnkSetEasyMultiLightSwitch(0, 0);
    njCnkSetSimpleMultiLightSwitch(0, 0);
    njCnkSetEasyMultiLightColor(1, 0.95f, 0.9f, 0.85f);
    njCnkSetEasyMultiLightVector(-0.4f, -0.5f, -0.75f);
    njCnkSetEasyMultiLightSwitch(1, 1);
    njCnkSetSimpleMultiLightColor(1, 0.95f, 0.9f, 0.85f);
    njCnkSetSimpleMultiLightVector(-0.4f, -0.5f, -0.75f);
    njCnkSetSimpleMultiLightSwitch(1, 1);
    njCnkSetEasyLightColor(0.95f, 0.9f, 0.85f);
    njCnkSetEasyLightIntensity(1.0f, 0.42f);
    njCnkSetEasyLight(-0.4f, -0.5f, -0.75f);
    njCnkSetSimpleLightColor(0.95f, 0.9f, 0.85f);
    njCnkSetSimpleLightIntensity(1.0f, 0.42f);
    njCnkSetSimpleLight(-0.4f, -0.5f, -0.75f);
    njSetMatrix(NULL, cam.mtx);
    njCnkSetEasyMultiLightMatrices();
    njCnkSetSimpleMultiLightMatrices();
}

static void scene_end(void)
{
    cam.mtx = cam_keep;
    if (cam.mtx) njSetMatrix(NULL, cam.mtx);
}

/* njReleaseTextureAll took everything (ps2_NaTextureFunction.c) */
void pc_lobby_textures_released(void)
{
    int k;
    actor_textures_released();
    font_state = 0;   /* the game's own: whoever loads it next releases it */
    for (k = 0; k < 2; k++) {   /* forget them; the lobby loads them again when it is still open */
        pv[k].tex_gone = 1;
        pv_release(&pv[k], k);
    }
}

/* ---------------------------------------------------------------- saves */
typedef struct SaveInfo { int valid, ply_id, stg, rom, gm_mode, count; } SaveInfo;
static SaveInfo* saves;   /* by slot (lobby.h: the card's 0..14 or native save number - 1) */
static int n_saves;
static unsigned char save_buf[LOBBY_SAVE_BYTES];   /* the host's chosen save, read when the game starts */
static int quickload;   /* F10 pressed: the title loads save 15 (test aid, with F9's save anywhere) */
static volatile int testsave_req;   /* RECVX_TEST_SAVESPOT: X held for 2 s on the Touch controller (xr.c) asks for F9's save */
void pc_testsave_request(void) { testsave_req = 1; }
int pc_quickload_take(void)
{
    static int boot = 1;
    if (boot) {   /* RECVX_TEST_SAVESPOT (env.txt; a test aid): the first title after the start loads save 15 at once (a missing save: the title as usual) */
        boot = 0;
        if (getenv("RECVX_TEST_SAVESPOT")) quickload = 1;
    }
    if (!quickload) return 0;
    quickload = 0;
    if (!lobby_read_save(14, save_buf)) { host_log("quickload: save 15 cannot be read\n"); return 0; }
    host_log("quickload: loading save 15\n");
    { extern volatile int g_vr_recenter_on_start; g_vr_recenter_on_start = 1; }   /* vr_glue.c heading_update: the headset's forward where the head looks when the game starts */
    return 1;
}

/* read when the lobby opens (and when a chosen save cannot be read): no save is written at the title */
static void scan_saves(void)
{
    static unsigned char b[LOBBY_SAVE_BYTES];
    int i;
    n_saves = lobby_save_slots();
    saves = realloc(saves, sizeof(SaveInfo) * (n_saves ? n_saves : 1));
    memset(saves, 0, sizeof(SaveInfo) * (n_saves ? n_saves : 1));
    for (i = 0; i < n_saves; i++) {
        const SAVEFILE* sf = (const SAVEFILE*)b;
        SaveInfo* s = &saves[i];
        s->valid = lobby_read_save(i, b);
        if (!s->valid) continue;
        s->ply_id = sf->ply_id & 3; s->stg = sf->stg_no; s->rom = sf->rom_no; s->gm_mode = sf->gm_mode; s->count = sf->save_ct;
    }
}

static const char* chr_names[] = { "Claire", "Chris", "Steve", "Wesker" };

static void save_label(char* out, int n, int slot)
{
    const SaveInfo* s = &saves[slot];
    snprintf(out, n, "Save %02d: %s %d-%02d", slot + 1, chr_names[s->ply_id], s->stg, s->rom);
}

/* the choices as lists: host START WITH: 0..2 new game, 3 + slot; joining ITEMS FROM: saved with the host, new, slot */
static int start_valid(int v) { return v >= 0 && (v < 3 || (v - 3 < n_saves && saves[v - 3].valid)); }
static int items_valid(int v) { return v == COOP_ITEMS_HOST || v == COOP_ITEMS_NEW || (v >= 0 && v < n_saves && saves[v].valid); }

static int step_choice(int v, int dir, int lo, int hi, int (*valid)(int))
{
    int i;
    for (i = 0; i < hi - lo + 1; i++) {
        v += dir;
        if (v > hi) v = lo;
        if (v < lo) v = hi;
        if (valid(v)) return v;
    }
    return v;
}

static void start_label(char* out, int n, int v)
{
    static const char* diff[] = { "New game, normal", "New game, easy", "New game, very easy" };
    if (v < 3) snprintf(out, n, "%s", diff[v < 0 ? 0 : v]);
    else save_label(out, n, v - 3);
}

static void items_label(char* out, int n, int v)
{
    if (v == COOP_ITEMS_HOST)   /* the host says whether it has them once we are in its lobby */
        snprintf(out, n, "Saved with the host%s", g_coop.state == COOP_OFF || g_coop.host || g_coop.peer_rec < 0 ? ""
                 : g_coop.peer_rec == REC_HAVE ? " (available)" : g_coop.peer_rec == REC_OFF ? " (host: off)" : " (none)");
    else if (v < 0) snprintf(out, n, "New inventory");
    else save_label(out, n, v);
}

/* ---------------------------------------------------------------- font */
/* The title runs without the game's font (sys->ef_tlist: the boot's warning screen releases all
   textures; the option menu loads it for itself, as here). Loaded while the lobby is open. */
static unsigned char font_file[3 << 20] __attribute__((aligned(64)));

static void font_load(void)
{
    int n;
    if (sys->ss_flg & 0x40) return;
    n = GetInsideFileSize(sys->sys_partid, 1);
    if (n <= 0 || n > (int)sizeof(font_file)) { host_log("lobby: font file of %d bytes\n", n); return; }
    RequestReadInsideFile(sys->sys_partid, 1, font_file);
    font_state = 1;
}

static int font_ready(void)
{
    if (font_state == 1 && GetReadFileStatus() == 0) {
        bhSetFontTexture(font_file);
        font_state = 2;
    }
    return (sys->ss_flg & 0x40) != 0;
}

static void font_release(void)
{
    if (font_state == 2 && (sys->ss_flg & 0x40)) {
        njReleaseTexture(&sys->ef_tlist);
        bhGarbageTexture(NULL, 0);
        sys->ss_flg &= ~0x40;
    }
    font_state = 0;
}

/* ---------------------------------------------------------------- the lobby */
/* In steps (docs/COOP.md "Lobby"): 1 how to connect (Direct / Via server) and Host or Join,
   2 only that choice's rows plus the player and game rows, 3 the game rules (host: edits them,
   guest: the host's, read-only). BACK goes one step back; from step 1 it leaves the lobby.
   The rows in display order; a gap where the group changes. */
enum { R_MODE, R_HOSTSTEP, R_JOINSTEP,
       R_SERVER, R_PORT, R_ADDR, R_PW, R_RELAY,
       R_NAME, R_CHAR,
       R_START, R_ITEMS, R_FOLLOW, R_RULES,
       R_LOOT, R_IMP, R_TFREEZE, R_SKIPS, R_RIBBON, R_EXTRA, R_PLACE, R_RAND, R_SEED, R_RSEED,
       R_HOST, R_GO, R_COPY, R_RCOPY, R_KICK, R_STOP, R_JOIN,
       R_BACK, R_N };
static const unsigned char row_group[R_N] = { 0, 1, 1, 0, 0, 0, 0, 0, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6 };
enum { F_NONE, F_NAME, F_ADDR, F_PORT, F_SEED, F_SERVER, F_PW };

static struct {
    int open, row, role;                /* role 0 host, 1 join */
    int step;                           /* 1 connection and role, 2 its options, 3 game rules */
    int mode;                           /* 0 direct (P2P), 1 via a co-op server (g_coop.conn Automatic or Via server) */
    int edit;                           /* F_* being edited */
    char buf[128];
    int caret, max;
    int spin;
    int want_chr, chr_stable;           /* debounce while the model choice is cycled */
    int items_sent;                     /* the joining items handed to coop_glue.c */
    char msg[96];
    int msg_t;
} lb = { .items_sent = -2 };

static int session_on(void) { return g_coop.state != COOP_OFF; }
static int hosting(void) { return session_on() && g_coop.host; }
static int joining(void) { return session_on() && !g_coop.host; }

static int row_visible(int r)
{
    int host = lb.role == 0, srv = lb.mode;
    if (r == R_BACK) return 1;
    if (lb.step == 1) return r == R_MODE || r == R_HOSTSTEP || r == R_JOINSTEP;
    if (lb.step == 3) return (r >= R_EXTRA && r <= R_SEED) || ((r == R_LOOT || r == R_TFREEZE || r == R_SKIPS || r == R_RIBBON || r == R_RSEED) && host) ||
                             (r == R_IMP && host && !g_coop.loot);   /* only shared loot has it */
    switch (r) {
    case R_PORT: return host && !srv;
    case R_SERVER: case R_PW: case R_RELAY: return srv;   /* the co-op server (docs/SERVER.md) */
    case R_ADDR: case R_ITEMS: case R_FOLLOW: case R_JOIN: return !host;
    case R_NAME: case R_CHAR: return 1;
    case R_START: return host;
    case R_RULES: return host || g_coop.state == COOP_CONNECTED;   /* guest: the host's, once known */
    case R_HOST: return host && !session_on();
    case R_GO: case R_STOP: return host && hosting();
    case R_COPY: return host && hosting() && !srv;
    case R_RCOPY: return host && hosting() && srv;
    case R_KICK: return host && hosting() && (g_coop.state == COOP_CONNECTED || g_coop.state == COOP_LOST);   /* a partner is there (or was, silent) */
    default: return 0;
    }
}

/* shown, but not to be changed now */
static int row_locked(int r)
{
    switch (r) {
    case R_MODE: case R_PORT: case R_LOOT: case R_IMP: case R_ADDR: case R_RELAY: case R_SERVER: case R_PW: return session_on();
    case R_HOSTSTEP: return joining();
    case R_JOINSTEP: return hosting();
    case R_EXTRA: case R_PLACE: case R_RAND: case R_SEED: return lb.role != 0;   /* the host's choice */
    case R_RCOPY: return !g_coop.room[0];
    default: return 0;
    }
}

static int row_selectable(int r) { return row_visible(r) && !row_locked(r); }

static int first_row(void)
{
    int r;
    for (r = 0; r < R_N && !row_selectable(r); r++);
    return r < R_N ? r : R_BACK;
}

/* a settings table entry by its label (one not edited through a variable the glue sees: g_config) */
static const Setting* lset(const char* label)
{
    int i;
    for (i = 0; i < g_settings_n; i++) if (!strcmp(g_settings[i].label, label)) return &g_settings[i];
    return NULL;
}

static void say(const char* m) { snprintf(lb.msg, sizeof(lb.msg), "%s", m); lb.msg_t = 150; }

/* the enemy options' entries of the settings table (F1 and the OPTION pages edit the same) */
static const Setting* eset(const int* var)
{
    int i;
    for (i = 0; i < g_settings_n; i++) if (g_settings[i].val == var) return &g_settings[i];
    return NULL;
}

static int allowed(int field, int c)
{
    if (field == F_PORT || field == F_SEED) return c >= '0' && c <= '9';
    if (field == F_ADDR || field == F_SERVER) return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == ':' || c == '-' || (c == '#' && field == F_ADDR);
    if (field == F_PW) return c > 32 && c < 127;
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || strchr(" -_.!?'()+", c);
}

static const char* cycle_set(int field)
{
    return field == F_PORT || field == F_SEED ? "0123456789" : field == F_ADDR || field == F_SERVER ? "0123456789.:abcdefghijklmnopqrstuvwxyz-#ABCDEFGHJKLMNPQRSTUVWXYZ"
                                                          : " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.!?'";
}

static void edit_begin(int field)
{
    lb.edit = field;
    if (field == F_NAME) { snprintf(lb.buf, sizeof(lb.buf), "%s", g_coop.name); lb.max = COOP_NAME_MAX - 1; }
    else if (field == F_ADDR) { snprintf(lb.buf, sizeof(lb.buf), "%s", g_coop.join); lb.max = 79; }
    else if (field == F_SEED) { snprintf(lb.buf, sizeof(lb.buf), "%d", g_enemy_opts.seed); lb.max = 5; }
    else if (field == F_SERVER) { snprintf(lb.buf, sizeof(lb.buf), "%s", g_coop.server); lb.max = 79; }
    else if (field == F_PW) { snprintf(lb.buf, sizeof(lb.buf), "%s", g_coop.password); lb.max = 40; }
    else { snprintf(lb.buf, sizeof(lb.buf), "%d", g_coop.port); lb.max = 5; }
    lb.caret = (int)strlen(lb.buf);
    g_lobby_io.tail = g_lobby_io.head;   /* nothing typed before counts */
    g_lobby_io.text_on = 1;
    CallSystemSe(0, 3);
}

static void edit_end(int accept)
{
    if (accept) {
        if (lb.edit == F_NAME) {
            char* s = lb.buf;
            while (*s == ' ') s++;
            if (*s) { snprintf(g_coop.name, sizeof(g_coop.name), "%.*s", (int)sizeof(g_coop.name) - 1, s); }
        } else if (lb.edit == F_ADDR) {
            if (lb.buf[0]) snprintf(g_coop.join, sizeof(g_coop.join), "%s", lb.buf);
        } else if (lb.edit == F_SERVER) {
            snprintf(g_coop.server, sizeof(g_coop.server), "%s", lb.buf);   /* empty: no server */
        } else if (lb.edit == F_PW) {
            snprintf(g_coop.password, sizeof(g_coop.password), "%.63s", lb.buf);
        } else if (lb.edit == F_SEED) {
            if (lb.buf[0]) settings_set(eset(&g_enemy_opts.seed), atoi(lb.buf));   /* clamped to the table's range */
        } else {
            int port = atoi(lb.buf);
            if (port >= 1 && port <= 65535) g_coop.port = port;
        }
        lobby_settings_changed();
    }
    if (lb.edit == F_PW) rdv_wipe(lb.buf, sizeof(lb.buf));   /* the edit copy of the password */
    lb.edit = F_NONE;
    g_lobby_io.text_on = 0;
    CallSystemSe(0, accept ? 3 : 0);
}

static void edit_insert(int c)
{
    int n = (int)strlen(lb.buf);
    if (n >= lb.max || !allowed(lb.edit, c)) return;
    memmove(lb.buf + lb.caret + 1, lb.buf + lb.caret, n - lb.caret + 1);
    lb.buf[lb.caret++] = (char)c;
}

/* keyboard (typed text) and pad (up/down change the character at the caret) */
static void edit_input(unsigned int press, unsigned int rept, unsigned int ok, unsigned int cancel)
{
    while (g_lobby_io.tail != __atomic_load_n(&g_lobby_io.head, __ATOMIC_ACQUIRE)) {
        int k = g_lobby_io.keys[g_lobby_io.tail & 63], n = (int)strlen(lb.buf);
        g_lobby_io.tail++;
        if (k == LK_ENTER) { edit_end(1); return; }
        if (k == LK_ESC) { edit_end(0); return; }
        if (k == LK_BACK && lb.caret > 0) { memmove(lb.buf + lb.caret - 1, lb.buf + lb.caret, n - lb.caret + 1); lb.caret--; }
        else if (k == LK_DEL && lb.caret < n) memmove(lb.buf + lb.caret, lb.buf + lb.caret + 1, n - lb.caret);
        else if (k == LK_LEFT && lb.caret > 0) lb.caret--;
        else if (k == LK_RIGHT && lb.caret < n) lb.caret++;
        else if (k == LK_HOME) lb.caret = 0;
        else if (k == LK_END) lb.caret = n;
        else if (k >= 32 && k < 127) edit_insert(k);
    }
    if (press & ok) { edit_end(1); return; }
    if (press & cancel) { edit_end(0); return; }
    if (rept & (BTN_UP | BTN_DOWN)) {
        const char* set = cycle_set(lb.edit);
        const char* at;
        int n = (int)strlen(set), i;
        if (lb.caret == (int)strlen(lb.buf)) { if ((int)strlen(lb.buf) >= lb.max) return; lb.buf[lb.caret] = set[0]; lb.buf[lb.caret + 1] = 0; }
        at = strchr(set, lb.buf[lb.caret]);
        i = at ? (int)(at - set) : 0;
        i = (i + ((rept & BTN_UP) ? 1 : n - 1)) % n;
        lb.buf[lb.caret] = set[i];
        CallSystemSe(0, 2);
    }
    if ((rept & BTN_LEFT) && lb.caret > 0) lb.caret--;
    if ((rept & BTN_RIGHT) && lb.caret < (int)strlen(lb.buf)) lb.caret++;
}

static void change(int r, int dir)
{
    switch (r) {
    case R_MODE: lb.mode ^= 1; g_coop.conn = lb.mode ? COOP_CONN_AUTO : COOP_CONN_DIRECT; break;   /* the server's default: direct first */
    case R_RELAY: g_coop.conn = g_coop.conn == COOP_CONN_SERVER ? COOP_CONN_AUTO : COOP_CONN_SERVER; break;
    case R_CHAR: g_coop.chr = appear_step(g_coop.chr, dir); break;   /* the list without event doubles (actors.c) */
    case R_LOOT: g_coop.loot ^= 1; break;
    case R_TFREEZE: g_coop.trade_freeze ^= 1; break;
    case R_IMP: g_coop.important ^= 1; break;
    case R_SKIPS: g_coop.skip_sync ^= 1; break;
    case R_RIBBON: settings_step(lset("No ink ribbons needed"), dir); break;   /* the GAMEPLAY setting (g_config) */
    case R_START: g_coop.start = step_choice(g_coop.start, dir, 0, n_saves + 2, start_valid); break;
    case R_ITEMS: g_coop.items = step_choice(g_coop.items, dir, COOP_ITEMS_HOST, n_saves - 1, items_valid); break;
    case R_FOLLOW: g_coop.follow ^= 1; break;   /* the joining player's own choice, also during a session */
    case R_EXTRA: settings_step(eset(&g_enemy_opts.extra), dir); break;
    case R_PLACE: settings_step(eset(&g_enemy_opts.place), dir); break;
    case R_RAND: settings_step(eset(&g_enemy_opts.rand), dir); break;
    default: return;
    }
    CallSystemSe(0, 2);
    lobby_settings_changed();
}

static int has_value(int r)
{
    return r == R_MODE || r == R_RELAY || r == R_CHAR || r == R_LOOT || r == R_IMP || r == R_TFREEZE || r == R_SKIPS || r == R_RIBBON || r == R_START || r == R_ITEMS || r == R_FOLLOW ||
           r == R_EXTRA || r == R_PLACE || r == R_RAND;
}

/* the joining player's items for coop_glue.c (own inventory on join) */
static void hand_items(void)
{
    int v = g_coop.items, ok = coop_items_apply();
    lb.items_sent = v;
    host_log("lobby: joining with %s\n", v == COOP_ITEMS_HOST ? "the items saved with the host (new-game items if it has none)"
                                        : ok ? "the items of a save" : "the new-game items");
    if (ok) host_log("lobby: items from save file %d\n", v + 1);
}

/* the host's join address for the partner: the public one once the router mapped the port */
static void join_address(char* out, int n)
{
    PortmapStatus pm;
    portmap_status(&pm);
    if (pm.public_ip[0]) snprintf(out, n, "%s:%d#%s", pm.public_ip, pm.public_port, g_coop.code);
    else snprintf(out, n, "%s:%d#%s", pm.local_ip[0] ? pm.local_ip : "127.0.0.1", g_coop.port, g_coop.code);
}

static void step_to(int s)
{
    lb.step = s;
    lb.row = first_row();
    host_log("lobby: step %d (%s, %s)\n", s, lb.role ? "join" : "host", lb.mode ? "via server" : "direct");
    CallSystemSe(0, 3);
}

/* one step back; from the first, out of the lobby (1) */
static int back(void)
{
    CallSystemSe(0, 0);
    if (lb.step <= 1) return 1;
    lb.step--;
    lb.row = lb.step == 2 ? R_RULES : lb.role ? R_JOINSTEP : R_HOSTSTEP;
    if (!row_selectable(lb.row)) lb.row = first_row();
    host_log("lobby: back to step %d\n", lb.step);
    return 0;
}

/* via a server needs its address before opening or joining */
static int need_server(void)
{
    if (!lb.mode || g_coop.server[0]) return 0;
    say("Enter the co-op server's address first (SERVER)");
    CallSystemSe(0, 1);
    lb.row = R_SERVER;
    return 1;
}

/* 0 stay, 1 back to the menu, 2 new game, 3 the chosen save */
static int activate(int r)
{
    switch (r) {
    case R_HOSTSTEP: lb.role = 0; step_to(2); return 0;
    case R_JOINSTEP: lb.role = 1; step_to(2); return 0;
    case R_RULES: step_to(3); return 0;
    case R_NAME: edit_begin(F_NAME); return 0;
    case R_PORT: edit_begin(F_PORT); return 0;
    case R_SEED: edit_begin(F_SEED); return 0;
    case R_RSEED: {
        const Setting* st = eset(&g_enemy_opts.seed);
        char m[48];
        settings_set(st, (int)((net_ms() * 2654435761u >> 8) % (unsigned)(st->hi + 1)));
        snprintf(m, sizeof(m), "new seed %d", g_enemy_opts.seed);
        say(m);
        CallSystemSe(0, 3);
        return 0;
    }
    case R_ADDR: edit_begin(F_ADDR); return 0;
    case R_SERVER: edit_begin(F_SERVER); return 0;
    case R_PW: edit_begin(F_PW); return 0;
    case R_RCOPY:
        snprintf(g_lobby_io.clip, sizeof(g_lobby_io.clip), "%.4s-%.4s", g_coop.room, g_coop.room + 4);
        g_lobby_io.clip_req = 1;
        say("room code copied");
        CallSystemSe(0, 3);
        return 0;
    case R_HOST:
        if (need_server()) return 0;
        g_coop.request = COOP_REQ_HOST;
        CallSystemSe(0, 3);
        return 0;
    case R_JOIN:
        if (session_on()) { g_coop.request = COOP_REQ_STOP; CallSystemSe(0, 0); return 0; }
        if (need_server()) return 0;
        hand_items();
        g_coop.request = COOP_REQ_JOIN;
        CallSystemSe(0, 3);
        return 0;
    case R_STOP: g_coop.request = COOP_REQ_STOP; CallSystemSe(0, 0); return 0;
    case R_KICK: g_coop.request = COOP_REQ_KICK; say("partner removed: the same code works again"); CallSystemSe(0, 0); return 0;
    case R_COPY:
        join_address(g_lobby_io.clip, sizeof(g_lobby_io.clip));
        g_lobby_io.clip_req = 1;
        say("join address copied");
        CallSystemSe(0, 3);
        return 0;
    case R_GO:
        if (g_coop.start >= 3) {
            if (!lobby_read_save(g_coop.start - 3, save_buf)) { say("that save cannot be read"); CallSystemSe(0, 1); scan_saves(); return 0; }
            host_log("lobby: starting from save file %d\n", g_coop.start - 2);
            return 3;
        }
        sys->gm_mode = g_coop.start;
        host_log("lobby: starting a new game (mode %d)\n", g_coop.start);
        return 2;
    case R_BACK: return back();
    default:
        if (has_value(r)) change(r, 1);
        return 0;
    }
}

/* the rows' layout: a heading, then the rows from ROW_Y0 to the info panel (y 384) */
#define ROW_X 316.0f
#define VAL_X 410.0f
#define HEAD_Y 70.0f
#define ROW_Y0 94.0f
#define ROW_SC 0.62f
#define GAP 8.0f

static int gaps(void)
{
    int r, g = -1, n = 0;
    for (r = 0; r < R_N; r++)
        if (row_visible(r)) { n += g >= 0 && row_group[r] != g; g = row_group[r]; }
    return n;
}

/* 25 px apart, closer when the rows would reach the info panel */
static float row_h(void)
{
    int r, n = 0;
    float h;
    for (r = 0; r < R_N; r++) n += row_visible(r);
    h = (382.0f - ROW_Y0 - gaps() * GAP) / n;
    return h < 25.0f ? h : 25.0f;
}

static float row_y(int row)
{
    int r, g = -1, n = 0, k = 0;
    for (r = 0; r < row; r++)
        if (row_visible(r)) { k += g >= 0 && row_group[r] != g; g = row_group[r]; n++; }
    k += g >= 0 && row_group[row] != g;
    return ROW_Y0 + n * row_h() + k * GAP;
}

static int row_at(float x, float y)
{
    int r;
    if (x < ROW_X - 8 || x > 636) return -1;
    for (r = 0; r < R_N; r++) {
        float ry = row_y(r);
        if (row_visible(r) && y >= ry - 3 && y < ry + row_h() - 3) return r;
    }
    return -1;
}

static void row_move(int dir)
{
    int i = lb.row, k;
    for (k = 0; k < R_N; k++) {
        i = (i + dir + R_N) % R_N;
        if (row_selectable(i)) { lb.row = i; CallSystemSe(0, 2); return; }
    }
}

static const char* heading(void)
{
    static char h[64];
    if (lb.step == 1) return "HOW DO YOU WANT TO PLAY?";
    if (lb.step == 3) return lb.role ? "THE HOST'S GAME RULES" : "GAME RULES";
    snprintf(h, sizeof(h), "%s - %s", lb.role ? "JOIN A GAME" : "HOST A GAME", lb.mode ? "VIA SERVER" : "DIRECT (P2P)");
    return h;
}

static void row_text(int r, char* label, char* val, int n)
{
    static const char* loots[] = { "Shared (one pickup)", "Individual (each their own)" };
    const EnemyOpts* eo = lb.role ? enemy_opts() : &g_enemy_opts;   /* joining: the host's (EV_EOPTS) */
    *val = 0;
    switch (r) {
    case R_MODE: strcpy(label, "CONNECT"); snprintf(val, n, "%s", lb.mode ? "Via server" : "Direct (P2P)"); break;
    case R_HOSTSTEP: strcpy(label, hosting() ? "HOST A GAME (OPEN)" : "HOST A GAME"); break;
    case R_JOINSTEP: strcpy(label, joining() ? "JOIN A GAME (JOINED)" : "JOIN A GAME"); break;
    case R_RELAY: strcpy(label, "RELAY"); snprintf(val, n, "%s", g_coop.conn == COOP_CONN_SERVER ? "Always" : "Only if needed"); break;
    case R_NAME: strcpy(label, "NAME"); snprintf(val, n, "%s", lb.edit == F_NAME ? lb.buf : g_coop.name); break;
    case R_CHAR: strcpy(label, "MODEL"); snprintf(val, n, "%s", appear_name(g_coop.chr)); break;
    case R_PORT: strcpy(label, "PORT"); if (lb.edit == F_PORT) snprintf(val, n, "%s", lb.buf); else snprintf(val, n, "%d", g_coop.port); break;
    case R_LOOT: strcpy(label, "LOOT"); snprintf(val, n, "%s", loots[g_coop.loot & 1]); break;
    case R_TFREEZE: strcpy(label, "FREEZE ON TRADE"); snprintf(val, n, "%s", g_coop.trade_freeze ? "On" : "Off"); break;
    case R_IMP: strcpy(label, "IMPORTANT ITEMS"); snprintf(val, n, "%s", g_coop.important ? "For both" : "Shared"); break;
    case R_SKIPS: strcpy(label, "CUTSCENE SKIP"); snprintf(val, n, "%s", g_coop.skip_sync ? "Together" : "Each alone"); break;
    case R_RIBBON: strcpy(label, "INK RIBBONS"); snprintf(val, n, "%s", settings_get(lset("No ink ribbons needed")) ? "Not needed" : "Needed"); break;
    case R_START: strcpy(label, "START"); start_label(val, n, g_coop.start); break;
    case R_RULES: strcpy(label, lb.role ? "THE HOST'S RULES" : "GAME RULES"); break;
    case R_HOST: strcpy(label, "OPEN THE LOBBY"); break;
    case R_GO: strcpy(label, "START THE GAME"); break;
    case R_STOP: strcpy(label, "CLOSE THE LOBBY"); break;
    case R_KICK: strcpy(label, "REMOVE STUCK PLAYER"); break;
    case R_COPY: strcpy(label, "COPY JOIN ADDRESS"); break;
    case R_RCOPY: strcpy(label, g_coop.room[0] ? "COPY ROOM CODE" : "ROOM CODE: WAITING FOR THE SERVER"); break;
    case R_ADDR: strcpy(label, lb.mode ? "CODE" : "ADDRESS"); snprintf(val, n, "%.*s", n - 1, lb.edit == F_ADDR ? lb.buf : g_coop.join); break;
    case R_SERVER: strcpy(label, "SERVER"); snprintf(val, n, "%.*s", n - 1, lb.edit == F_SERVER ? lb.buf : g_coop.server[0] ? g_coop.server : "-"); break;
    case R_PW: {   /* shown as stars, also while typing */
        int k, len = (int)strlen(lb.edit == F_PW ? lb.buf : g_coop.password);
        strcpy(label, "PASSWORD");
        for (k = 0; k < len && k < n - 1; k++) val[k] = '*';
        val[k] = 0;
        if (!len && lb.edit != F_PW) snprintf(val, n, "-");
        break;
    }
    case R_ITEMS: strcpy(label, "ITEMS"); items_label(val, n, g_coop.items); break;
    case R_FOLLOW: strcpy(label, "FOLLOW"); snprintf(val, n, "%s", g_coop.follow ? "On" : "Off"); break;
    case R_EXTRA: strcpy(label, "EXTRA ENEMIES"); snprintf(val, n, "%s", eset(&g_enemy_opts.extra)->names[eo->extra & 3]); break;
    case R_PLACE: strcpy(label, "REARRANGED"); snprintf(val, n, "%s", eo->place ? "On" : "Off"); break;
    case R_RAND: strcpy(label, "RANDOMIZER"); snprintf(val, n, "%s", eo->rand ? "On" : "Off"); break;
    case R_SEED: strcpy(label, "SEED"); if (lb.edit == F_SEED) snprintf(val, n, "%s", lb.buf); else snprintf(val, n, "%d", eo->seed); break;
    case R_RSEED: strcpy(label, "NEW RANDOM SEED"); break;
    case R_JOIN: strcpy(label, session_on() ? "LEAVE" : "JOIN"); break;
    case R_BACK: strcpy(label, "BACK"); break;
    }
}

/* the help line: what the selected row is for */
static const char* row_help(int r)
{
    int host = lb.role == 0;
    switch (r) {
    case R_MODE:
        return lb.mode ? "Via server: a co-op server (recvx-server) puts you together by a room code. It tries a direct connection first and relays only if that fails; for when no port can be opened."
                       : "Direct (P2P): the partner connects straight to the host's address (ip:port#CODE). The host's port must be reachable: same network, UPnP or port forwarding.";
    case R_HOSTSTEP: return "Host a game: open a lobby, your partner joins it, and you start the game - a new game or one of your saves.";
    case R_JOINSTEP: return "Join a game: enter what the host gives you (its address, or its room code via a server) and join; the host starts the game.";
    case R_SERVER: return "The co-op server's address: host or host:port (port 7141 if none). Both players use the same server.";
    case R_PORT: return "The UDP port this game listens on (7140). A partner outside your network needs it reachable; UPnP asks the router.";
    case R_ADDR:
        return lb.mode ? "CODE: the room code the host's lobby shows (XXXX-XXXX; case and dash do not matter). Ctrl+V pastes."
                       : "ADDRESS: the host's join address, ip:port#CODE, as its COPY JOIN ADDRESS gives it. Ctrl+V pastes.";
    case R_PW: return host ? "Optional room password: your partner has to type the same one. Not saved." : "The room's password, if the host set one; otherwise leave it empty.";
    case R_RELAY: return "Only if needed: a direct connection first (about 2 s), then the server's relay. Always: everything goes through the relay (slower, predictable).";
    case R_NAME: return "What your partner sees above your head and in the lobby. Type it, or change the letter at the caret with up/down.";
    case R_CHAR: return "The model you appear as: your partner sees it, and so do you.";
    case R_START: return "What the game starts from: a new game (normal, easy, very easy) or one of your saves.";
    case R_ITEMS: return "The inventory you join with: the one the host kept for you from your last visit, a new inventory, or that of one of your saves.";
    case R_FOLLOW: return eset(&g_coop.follow)->help;
    case R_RULES: return host ? "Loot, enemies freezing while you trade, cutscene skips, ink ribbons and the enemy options (extra enemies, rearranged, randomizer, seed)."
                              : "The host's enemy options (read-only).";
    case R_LOOT: return "Shared: each item lies there once - whoever takes it has it, so you split the supplies; one item box. Individual: each finds their own copy and has their own box. Keys and story go to both.";
    case R_TFREEZE: return eset(&g_coop.trade_freeze)->help;
    case R_IMP: return "For both: weapons and ink ribbons lie there for each of you, each takes their own copy. Shared: they are one pickup like ammo and herbs - whoever takes it has it.";
    case R_SKIPS: return "Together: a cutscene both of you watch is skipped when both press skip; the partner sees that you want to. Each alone: as in single player.";
    case R_RIBBON: return "Not needed: typewriters save without an ink ribbon and use none up (for both players; also in single player, ENHANCED SETTINGS GAMEPLAY).";
    case R_EXTRA: return eset(&g_enemy_opts.extra)->help;
    case R_PLACE: return eset(&g_enemy_opts.place)->help;
    case R_RAND: return eset(&g_enemy_opts.rand)->help;
    case R_SEED: return eset(&g_enemy_opts.seed)->help;
    case R_RSEED: return "Picks a new random seed.";
    case R_HOST: return lb.mode ? "Opens your lobby and a room on the co-op server; give your partner the room code that appears."
                                : "Opens your lobby: your partner can join now. START and the rules can still change while you wait.";
    case R_GO: return "Starts the game; your partner enters it once you have control.";
    case R_COPY: return "Copies ip:port#CODE for your partner (the public address once the router forwards the port, else the LAN one).";
    case R_RCOPY: return "Copies the room code for your partner: Join a game, Via server, CODE.";
    case R_STOP: return "Closes your lobby: the partner is told, the port is freed.";
    case R_KICK: return "Ends the session with your partner (a crashed or stuck game) and frees its place; the same address or room code works again.";
    case R_JOIN: return session_on() ? "Leaves the host's lobby." : "Joins the host's lobby; the host starts the game.";
    case R_BACK:
        return lb.step == 1 ? "Back to the title menu; an open lobby or session closes."
             : lb.step == 2 ? "Back to choosing how to connect (an open lobby stays open)." : "Back to the lobby.";
    default: return "";
    }
}

/* 0 stay, else activate()'s result */
static int lobby_input(void)
{
    ADV_WORK* ap = &AdvWork;
    unsigned int press = Pad[ap->PortId].press, rept = Pad[ap->PortId].Rept;
    unsigned int ok = AdvGetOkButton() | BTN_START, cancel = AdvGetCancelButton();
    int hit;
    menu_mouse_take(&mt);
    if (press || rept || mt.left || mt.right || mt.wheel || mt.moved || g_lobby_io.head != g_lobby_io.tail) StopAdvScreenSaver(0);
    if (lb.edit) {
        if (mt.left && row_at(mt.x, mt.y) != lb.row) edit_end(1);   /* a click elsewhere takes the text */
        else if (mt.right) edit_end(0);
        else edit_input(press, rept, ok, cancel);
        return 0;
    }
    g_lobby_io.tail = g_lobby_io.head;
    if (!row_selectable(lb.row)) {   /* the row went (hosting started or stopped): its successor */
        lb.row = lb.row == R_HOST || lb.row == R_KICK ? R_GO : lb.row == R_GO || lb.row == R_STOP || lb.row == R_COPY || lb.row == R_RCOPY ? R_HOST : lb.row;
        if (!row_selectable(lb.row)) lb.row = first_row();
    }
    hit = row_at(mt.x, mt.y);
    if (hit >= 0 && !row_selectable(hit)) hit = -1;
    if (hit >= 0 && (mt.moved || mt.left || mt.wheel) && hit != lb.row) { lb.row = hit; CallSystemSe(0, 2); }
    if (mt.wheel && hit >= 0 && has_value(hit)) change(hit, mt.wheel > 0 ? -1 : 1);
    if (mt.left && hit >= 0) {
        if (!has_value(hit)) return activate(hit);
        change(hit, mt.x < VAL_X + 14 ? -1 : 1);   /* the "<" arrow: back */
    }
    if (mt.right) return back();
    if (rept & BTN_UP) row_move(-1);
    else if (rept & BTN_DOWN) row_move(1);
    else if ((rept & BTN_LEFT) && has_value(lb.row)) change(lb.row, -1);
    else if ((rept & BTN_RIGHT) && has_value(lb.row)) change(lb.row, 1);
    else if (press & ok) return activate(lb.row);
    else if (press & cancel) return back();
    return 0;
}

static void draw_rows(void)
{
    int r;
    char label[64], val[96];
    text(ROW_X - 4, HEAD_Y, heading(), 0.5f, 0xFFFFE080);
    for (r = 0; r < R_N; r++) {
        int sel = r == lb.row, locked = row_locked(r);
        float y = row_y(r);
        unsigned int col = locked ? 0xFF707070 : sel ? 0xFFFFFFFF : 0xFFB0B0B0;
        if (!row_visible(r)) continue;
        row_text(r, label, val, sizeof(val));
        if (sel) rect(ROW_X - 8, y - 2, 634, y + row_h() - 4, lb.edit ? 0x60406080 : 0x50FFFFFF, Z_FRONT);
        {   /* a long label next to a value: narrower (EXTRA ENEMIES); a long one alone: to the panel's edge */
            float lw = text_w(label, ROW_SC), room = (has_value(r) || val[0]) ? VAL_X - ROW_X - 8 : 634 - ROW_X - 4,
                  ls = lw > room ? ROW_SC * room / lw : ROW_SC;
            text(ROW_X, y + (ROW_SC - ls) * 14, label, ls, col);
        }
        if (has_value(r)) {
            float w = text_w(val, ROW_SC), sc = w > 190 ? ROW_SC * 190 / w : ROW_SC;   /* long model names: narrower */
            if (sc < ROW_SC) w = 190;
            if (!locked) { arrow(VAL_X, y + 9, -1, col); arrow(VAL_X + 14 + w + 12, y + 9, 1, col); }
            text(VAL_X + 12, y + (ROW_SC - sc) * 14, val, sc, col);
        } else if (val[0] || lb.edit) {
            float w = text_w(val, ROW_SC), sc = w > 220 ? ROW_SC * 220 / w : ROW_SC;   /* a long address: narrower */
            text(VAL_X, y + (ROW_SC - sc) * 14, val, sc, col);
            if (sel && lb.edit && (lb.spin & 16)) {   /* caret */
                char pre[128];
                snprintf(pre, sizeof(pre), "%.*s", lb.caret, lb.buf);
                if (lb.edit == F_PW) memset(pre, '*', strlen(pre));
                rect(VAL_X + text_w(pre, sc), y, VAL_X + text_w(pre, sc) + 2, y + 18, 0xFFFFFFFF, Z_FRONT + 0.01f);
            }
        }
    }
}

/* under the rows: a message or the help line, the status, and while hosting what the partner needs */
static void draw_info(void)
{
    PortmapStatus pm;
    char line[200];
    float y = 390;
    unsigned int col = 0xFFC8C8C8;
    rect(0, 384, 640, 480, 0x60000000, Z_FRONT);
    if (lb.msg_t > 0) { lb.msg_t--; text(24, y, lb.msg, 0.55f, 0xFFFFE080); y += 17; }
    else y += 15 * text_wrap(24, y, 584, row_help(lb.row), 0.5f, 0xFFA0D0FF, 2);
    /* without a session a status is why (busy port, the host left): highlighted */
    y += 16 * text_wrap(24, y, 592, g_coop.status[0] ? g_coop.status : "Not connected", 0.52f, !session_on() && g_coop.status[0] ? 0xFFFFC060 : col, 2);
    if (!hosting()) return;
    if (g_coop.room[0]) {
        snprintf(line, sizeof(line), "ROOM CODE %.4s-%.4s (through the co-op server)", g_coop.room, g_coop.room + 4);
        text(24, y, line, 0.52f, 0xFFFFFFFF); y += 16;
    }
    portmap_status(&pm);
    if (lb.mode || !(pm.local_ip[0] || pm.msg[0])) return;
    snprintf(line, sizeof(line), "LAN %s:%d   PUBLIC %s", pm.local_ip[0] ? pm.local_ip : "?", g_coop.port,
             pm.public_ip[0] ? pm.public_ip : pm.state == 1 ? "looking..." : "unknown");
    if (pm.public_ip[0]) snprintf(line + strlen(line), sizeof(line) - strlen(line), ":%d", pm.public_port);
    if (g_coop.code[0]) snprintf(line + strlen(line), sizeof(line) - strlen(line), "   CODE %.4s-%.4s", g_coop.code, g_coop.code + 4);
    text(24, y, line, 0.52f, col); y += 16;
    if (pm.msg[0] && y < 470) text_wrap(24, y, 592, pm.msg, 0.48f, pm.state == 3 ? 0xFFFFC060 : 0xFFA0A0A0, y < 455 ? 2 : 1);   /* UPnP */
}

/* our model and, at the title with us, the partner's, with their names */
static void draw_previews(void)
{
    int partner = g_coop.state == COOP_CONNECTED && !g_coop.peer_in_game, k;
    float sx[2] = { partner ? 92.0f : 120.0f, 222.0f };
    int played = g_coop.start >= 3 && lb.role == 0 && start_valid(g_coop.start) ? saves[g_coop.start - 3].ply_id : 0;
    /* the chosen model loads once it has stayed for a few frames (cycling through the list) */
    if (g_coop.chr != lb.want_chr) { lb.want_chr = g_coop.chr; lb.chr_stable = 0; }
    if (++lb.chr_stable >= 8 || pv[0].chr < 0) pv_set(0, lb.want_chr, played);
    pv_set(1, partner && g_coop.peer_lchr >= 0 && g_coop.peer_lchr <= COOP_CHR_MAX ? g_coop.peer_lchr : -1, 0);
    lb.spin += 1;
    {
        float h = pv_top(0) > pv_top(1) ? pv_top(0) : pv_top(1), want = 500.0f / 1.0f * (h + 3.0f) / 260.0f;
        if (want < 30) want = 30;
        if (want > 90) want = 90;
        cam_z = h > 0 ? cam_z + (want - cam_z) * 0.25f : 34.0f;
    }
    scene_begin();
    for (k = 0; k < 2; k++) {
        pv_pose(k, screen_to_world_x(sx[k]), (lb.spin * 160) & 0xFFFF);
        pv_draw(k);
    }
    for (k = 0; k < 2; k++) {
        NJS_POINT2 h;
        const char* nm = k ? (g_coop.peer_name[0] ? g_coop.peer_name : "PARTNER") : g_coop.name;
        if (pv[k].file < 0) continue;
        h = pv_head(k);
        text_c(h.x, h.y - 22, nm, 0.62f, k ? 0xFFFFE0A0 : 0xFFFFFFFF);
        if (k && g_coop.peer_note[0] && g_coop.peer_lobby) text_c(h.x, 354, g_coop.peer_note, 0.45f, 0xFFC0C0C0);
    }
    scene_end();
    if (g_coop.state == COOP_CONNECTED && g_coop.peer_in_game) {
        char t[64];
        snprintf(t, sizeof(t), "%s is in the game", g_coop.peer_name[0] ? g_coop.peer_name : "The partner");
        text_c(232, 210, t, 0.5f, 0xFFFFE0A0);
    } else if (!partner) text_c(232, 210, joining() ? "Waiting for the host" : hosting() ? "Waiting for a partner" : "", 0.5f, 0xFFA0A0A0);
    else if (!g_coop.peer_lobby) text_c(222, 372, "(in the title menu)", 0.45f, 0xFFA0A0A0);
}

static void lobby_open(void)
{
    memset(&lb, 0, sizeof(lb));
    lb.open = 1;
    lb.want_chr = -1;
    lb.items_sent = -2;
    lb.role = joining() ? 1 : 0;
    /* Automatic without a server is direct (docs/COOP.md "Connection modes") */
    lb.mode = !(g_coop.conn == COOP_CONN_DIRECT || (g_coop.conn == COOP_CONN_AUTO && !g_coop.server[0]));
    lb.step = session_on() ? 2 : 1;   /* a session hosted or joined through F3: its options */
    lb.row = first_row();
    scan_saves();
    font_load();
    if (!start_valid(g_coop.start)) g_coop.start = 0;
    if (!items_valid(g_coop.items)) g_coop.items = COOP_ITEMS_HOST;
    g_coop.lobby = 1;
    host_log("lobby: open\n");
}

/* keep: the game starts from the lobby (ours, or the host's joined from it); anything else (BACK,
   cancel, right click, a soft reset) closes the session too: hosting stops, the partner hears it */
static void lobby_close(int keep)
{
    int k;
    if (!lb.open) return;
    if (!keep && (session_on() || g_coop.request)) {
        g_coop.request = COOP_REQ_STOP;
        host_log("lobby: left, the session closes\n");
    }
    if (lb.edit) edit_end(0);
    for (k = 0; k < 2; k++) pv_release(&pv[k], k);
    font_release();
    lb.open = 0;
    g_coop.lobby = 0;
    g_coop.note[0] = 0;
    host_log("lobby: closed\n");
}

void pc_lobby_close(void) { lobby_close(0); }

/* Adv_BioCvTitle mode 30, once per frame */
int pc_lobby(void)
{
    int r;
    char note[64];
    if (!lb.open) lobby_open();
    if (session_on()) lb.role = !g_coop.host;   /* F3 may host or join meanwhile */
    font_ok = font_ready();
    r = lobby_input();
    /* what we chose, for the partner's screen; the joining items as they change */
    if (lb.role == 0) { start_label(note, sizeof(note), g_coop.start); snprintf(g_coop.note, sizeof(g_coop.note), "Starts: %.*s", (int)sizeof(g_coop.note) - 9, note); }
    else if (g_coop.items == COOP_ITEMS_HOST) snprintf(g_coop.note, sizeof(g_coop.note), "Items saved with you");
    else if (g_coop.items < 0) snprintf(g_coop.note, sizeof(g_coop.note), "New inventory");
    else { items_label(note, sizeof(note), g_coop.items); snprintf(g_coop.note, sizeof(g_coop.note), "Items: %.*s", (int)sizeof(g_coop.note) - 8, note); }
    if (joining() && lb.items_sent != g_coop.items) hand_items();

    AdvEasySetTextureList(0);
    AdvDwawOnePictureEx(0, 0xFFFFFFFF);   /* the title picture, darkened */
    rect(0, 0, 640, 480, 0xB0000000, Z_DIM);
    rect(304, 64, 640, 384, 0x70000000, Z_DIM + 0.0005f);
    mp_word(320.0f, 16.0f, 0xFFFFFFFF, 192, 1.0f);
    draw_previews();
    draw_rows();
    draw_info();
    AdvEasySetTextureList(0);   /* the title's texture list again (the font and the models set theirs) */
    if (r) lobby_close(r != 1);
    return r;
}

/* bhSysCallFirstmovie, return code 8: the save chosen in the lobby, as LOAD GAME reads it */
void pc_lobby_load_save(void)
{
    memcpy(&sys->version, save_buf, (char*)&sys->save_end - (char*)&sys->version);
}

/* coop_glue.c: the host's game is joined from the title (its menu or the lobby). What the
   title does when it ends (Adv_BioCvTitle mode -1), before the game state is loaded. */
void pc_title_leave(void)
{
    ADV_WORK* ap = &AdvWork;
    lobby_close(1);
    StopVibrationEx();
    AdvEasyReleaseTexture();
    StopAdx(0);
    ap->DepthLevel = 0;
    ap->Mode = 0;
    ap->Active = 0;
    sys->ss_flg &= ~0x20000;
    g_menu_mouse.active = 0;
}

/* coop_glue.c: the guest starts the host's game (from the title, the lobby, the attract demo, a rejoin).
   The title's own game choices set AppMode 1; left at the title's 0, the in-game OPTION screen (Select)
   ended the title's way - AdvEasyReleaseAllTexture, no AdvPopRoomTexture - and the room was drawn
   without its textures (a crash on leaving it). */
void pc_adv_in_game(void) { AdvWork.AppMode = 1; }

/* Test aid (frame hand-off, pc/src/ps2/libgraph.c): RECVX_TEST_SAVE="gameframe:slot[,...]" writes the
   game in progress to memory card save slot `slot` (0..14) as a typewriter save holds it (the
   player's live position, angle, health, weapon and status first, as coop_glue.c send_blob does),
   at that frame of the game task. */
void lobby_frame(void)
{
    static const char* env = (const char*)-1;
    static int clock, prev = -1;
    static unsigned char keep[LOBBY_SAVE_BYTES];
    int f, slot, n = (int)((char*)&sys->save_end - (char*)&sys->version);
    static const char* join = (const char*)-1, *stop = (const char*)-1, *title = (const char*)-1;
    static unsigned int hand;
    hand++;
    if (join == (const char*)-1) join = getenv("RECVX_TEST_JOIN");
    if (join && *join && hand >= (unsigned)atoi(join)) {   /* RECVX_TEST_JOIN="n": F3's Join at the n-th frame hand-off */
        host_log("test: join (F3) at hand-off %u\n", hand);
        g_coop.request = COOP_REQ_JOIN;
        join = strchr(join, ',') ? strchr(join, ',') + 1 : NULL;   /* "n,n,..." */
    }
    if (stop == (const char*)-1) stop = getenv("RECVX_TEST_STOP");
    if (stop && *stop && hand >= (unsigned)atoi(stop)) {   /* RECVX_TEST_STOP="n": F3's Disconnect at the n-th hand-off */
        host_log("test: disconnect (F3) at hand-off %u\n", hand);
        g_coop.request = COOP_REQ_STOP;
        stop = strchr(stop, ',') ? strchr(stop, ',') + 1 : NULL;
    }
    if (title == (const char*)-1) title = getenv("RECVX_TEST_TITLE");
    if (title && *title && hand >= (unsigned)atoi(title) && sys && (sys->tk_flg & 0x80)) {   /* RECVX_TEST_TITLE="n": as game over "No" */
        host_log("test: back to the title at hand-off %u\n", hand);
        bhReturnTitle();
        title = NULL;
    }
    {   /* F9: the game in progress written now to save 15, as RECVX_TEST_SAVE does (a test aid: a save anywhere) */
        static int was;
        extern const _Bool* SDL_GetKeyboardState(int* numkeys);   /* (SDL3; this file does not include SDL) */
        const _Bool* ks = SDL_GetKeyboardState(NULL);
        int down = ks && ks[66 /* SDL_SCANCODE_F9 */];
        int now = down && !was && sys && plp && (sys->tk_flg & 0x80) && !(sys->ss_flg & 0x400000);
        was = down;
        if (now) { slot = 14; goto write_save; }
    }
    if (testsave_req) {   /* the same save asked for by X held on the Touch controller; kept for a few seconds while the game is not in play */
        static int waited;
        if (sys && plp && (sys->tk_flg & 0x80) && !(sys->ss_flg & 0x400000)) { testsave_req = 0; waited = 0; slot = 14; goto write_save; }
        if (++waited > 300) { testsave_req = 0; waited = 0; host_log("test: save here refused (the game is not in play)\n"); }
    }
    {   /* F10: back to the title, which then loads save 15 (pc_quickload_take, adv.c) */
        static int was;
        extern const _Bool* SDL_GetKeyboardState(int* numkeys);
        const _Bool* ks = SDL_GetKeyboardState(NULL);
        int down = ks && ks[67 /* SDL_SCANCODE_F10 */];
        if (down && !was && !quickload) {
            quickload = 1;
            host_log("quickload: F10\n");
            if (sys && (sys->tk_flg & 0x80)) bhReturnTitle();
        }
        was = down;
    }
    if (env == (const char*)-1) env = getenv("RECVX_TEST_SAVE");
    if (!env || !sys || !plp || !(sys->tk_flg & 0x80) || (sys->ss_flg & 0x400000)) return;
    if (sys->gfrm_ct != prev) { prev = sys->gfrm_ct; clock++; }
    if (sscanf(env, "%d:%d", &f, &slot) != 2 || clock < f) return;
    env = strchr(env, ',') ? strchr(env, ',') + 1 : NULL;   /* "f:slot,f:slot,..." */
write_save:
    memcpy(keep, &sys->version, n);
    sys->version = 10;
    sys->flr_no = plp->flr_no;
    sys->ply_stflg[(int)sys->ply_id] = plp->stflg & 0x78280000;
    sys->ply_pos.x = plp->px; sys->ply_pos.y = plp->py; sys->ply_pos.z = plp->pz;
    sys->ply_ang = plp->ay;
    host_log("test: facing saved %d (%.0f degrees)\n", (int)(short)plp->ay, (double)(short)plp->ay * (180.0 / 32768.0));
    sys->ply_wno[(int)sys->ply_id] = plp->wpnr_no;
    sys->ply_hp[(int)sys->ply_id] = plp->hp;
    coop_save_written(slot);   /* as the memory card's save: the host's partner items go with it */
    host_log("test: %s save file %d (stage %d room %d, items %08x %08x %08x)\n", lobby_write_save(slot, &sys->version, n) ? "wrote" : "cannot write",
             slot + 1, sys->stg_no, sys->rom_no, sys->itm[sys->ply_id * 16 + 2], sys->itm[sys->ply_id * 16 + 3], sys->itm[sys->ply_id * 16 + 4]);
    memcpy(&sys->version, keep, n);
}

/* the lobby's drawing helpers for the ENHANCED SETTINGS pages of the OPTION menu (pc/glue/options_glue.c) */
void pc_ui_text(float x, float y, const char* s, float sc, unsigned int argb) { font_ok = (sys->ss_flg & 0x40) != 0; text(x, y, s, sc, argb); }
float pc_ui_text_w(const char* s, float sc) { return text_w(s, sc); }
int pc_ui_text_wrap(float x, float y, float w, const char* s, float sc, unsigned int argb, int max_lines)
{
    font_ok = (sys->ss_flg & 0x40) != 0;
    return text_wrap(x, y, w, s, sc, argb, max_lines);
}
void pc_ui_rect(float x0, float y0, float x1, float y1, unsigned int argb, float z) { rect(x0, y0, x1, y1, argb, z); }
void pc_ui_arrow(float x, float y, int dir, unsigned int argb) { arrow(x, y, dir, argb); }

/* ENHANCED SETTINGS in the game's OPTION menu (docs/ARCHITECTURE.md "Settings"): an eighth item on
 * the option screen, right of EXIT, set in the screen's own lettering, opens pages of the port's settings drawn
 * like the co-op lobby (the game's font, the option screen's background and title, its cursor
 * sounds). Game thread, from adv.c (Adv_GameOptionScreen and DisplayOptionPlate*): option
 * level 4 is ours. Values come from the shared settings table (pc/src/host/settings.c) and the
 * control bindings (pc/src/input/bindings.c), the same ones the F1 overlay edits.
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/adv.h"
#include "../../decomp/include/ps2/veronica/prog/padman.h"
#include "../../decomp/include/ps2/veronica/prog/sdfunc.h"
#include "../../decomp/include/ps2/veronica/prog/ps2_NaDraw.h"
#include "../src/host/settings.h"
#include "../src/host/bindings.h"
#include "../src/host/menu_mouse.h"
#include "../src/host/xr.h"
#include <stdio.h>
#include <string.h>

extern ADV_WORK AdvWork;
extern QUAD Qtex[16];
extern unsigned int* pad_type[4];
extern OPTION OptionDef[];
void host_log(const char* fmt, ...);
void pc_ui_text(float x, float y, const char* s, float sc, unsigned int argb);
float pc_ui_text_w(const char* s, float sc);
int pc_ui_text_wrap(float x, float y, float w, const char* s, float sc, unsigned int argb, int max_lines);
void pc_ui_rect(float x0, float y0, float x1, float y1, unsigned int argb, float z);
void pc_ui_arrow(float x, float y, int dir, unsigned int argb);
int bind_any_down(void);
void config_save(void);
uint64_t SDL_GetTicks(void);

enum { P_UP = 0x1000, P_RIGHT = 0x2000, P_DOWN = 0x4000, P_LEFT = 0x8000, P_START = 0x800, P_SELECT = 0x100, P_L1 = 0x4, P_R1 = 0x8 };   /* Pad[] bits */

/* ---------------------------------------------------------------- glue for the host */
int pc_in_game(void) { return sys && (sys->tk_flg & 0x80); }
int pc_keytype(void) { return sys ? sys->keytype & 3 : 0; }

/* the key type's DS2 buttons for virtual pad bit v (pad.c bhSetPad), in pad report bits */
int pc_keytype_mask(int v)
{
    unsigned int n;
    if (!sys || v < 0 || v > 17) return -1;
    n = pad_type[sys->keytype & 3][v];
    return (int)(((n & 0xFF) << 8) | ((n >> 8) & 0xFF));
}

/* ---------------------------------------------------------------- mouse */
static MouseTake mt;                    /* the pointer (game 2D px) and what it did */


static unsigned int ok_button(void) { return (sys->keytype & 3) < 3 ? AdvGetOkButton() : 0xC0; }
static unsigned int cancel_button(void) { return (sys->keytype & 3) < 3 ? AdvGetCancelButton() : 0x20; }

/* Adv_GameOptionScreen, every frame: the OS cursor shows while the option screen is up */
void pc_options_frame(void) { g_menu_mouse.active = 1; }

/* ---------------------------------------------------------------- level 0: ENHANCED SETTINGS
   On EXIT's row, at the right, like STEREO / MONO and ON / OFF: left/right move between EXIT and
   it (adv.c DisplayOptionPlateLevel0), up goes to DEFAULT, down wraps to the top as from EXIT. */
#define ROW8_Y 352.0f
#define ROW8_RIGHT 584.0f
/* "ENHANCED SETTINGS" from the row labels' own letters (texture 0: AUDIO SETUP at v 0, KEY CONFIG
   32, SCREEN SETUP 64, DEFAULT 160, EXIT 192, SETUP 224; the only H is in the narrower key config
   caption CHANGE AIM at 256, drawn wider). u = left edge, w = width (measured from the texture),
   dw = drawn width. */
typedef struct { short u, v, w, dw; } Row8Letter;
static const Row8Letter row8_us[] = {
    { 16, 224, 16, 16 }, { 88, 64, 19, 19 }, { 302, 256, 13, 18 }, { 0, 0, 18, 18 },   /* E (SETUP) N (SCREEN) H (CHANGE) A (AUDIO) */
    { 88, 64, 19, 19 }, { 70, 32, 17, 17 }, { 16, 224, 16, 16 }, { 0, 160, 18, 18 },  /* N C (CONFIG) E D (DEFAULT) */
    { -1, 0, 12, 12 },                                                                  /* space */
    { 0, 224, 15, 15 }, { 16, 224, 16, 16 }, { 33, 224, 18, 18 }, { 33, 224, 18, 18 }, /* S E T T (SETUP) */
    { 38, 192, 11, 11 }, { 88, 64, 19, 19 }, { 158, 32, 19, 19 }, { 0, 224, 15, 15 },  /* I (EXIT) N G (CONFIG) S */
};

/* vr153: the Brazilian translation's texture 0 has no H or G: "MAIS AJUSTES" from its own SAIR (v 192), MONO (256) and
   the whole AJUSTES (224), measured from the disc */
static const Row8Letter row8_br[] = {
    { 152, 256, 21, 21 }, { 16, 192, 18, 18 }, { 35, 192, 11, 11 }, { 0, 192, 15, 15 },   /* M (MONO) A I S (SAIR) */
    { -1, 0, 12, 12 },                                                                      /* space */
    { 0, 224, 118, 118 },                                                                   /* AJUSTES */
};
extern int g_plates_br;   /* the disc's language (font_glue.c plates_from_iso) */
#define row8 (g_plates_br ? row8_br : row8_us)
#define ROW8_N (g_plates_br ? (int)(sizeof(row8_br) / sizeof(row8_br[0])) : (int)(sizeof(row8_us) / sizeof(row8_us[0])))

static float row8_x(void)   /* right-aligned with the values */
{
    float w = 0;
    for (int i = 0; i < ROW8_N; i++) w += row8[i].dw + 1;
    return ROW8_RIGHT - w;
}

/* DisplayOptionPlateLevel0, with the other rows: drawn as they are (dim, or pulsing when selected) */
void pc_options_row8(int selected)
{
    ADV_WORK* ap = &AdvWork;
    unsigned int col = selected ? (((unsigned int)ap->FlushCount2 + 255) << 24) | 0xFFFFFF : 0x7FAFAFAF;
    float x = row8_x();
    QUAD* q = &Qtex[15];
    for (int i = 0; i < ROW8_N; i++) {
        if (row8[i].u >= 0) {
            SetQuadPos(x, ROW8_Y, row8[i].dw, 32.0f, q);
            SetQuadUv2(row8[i].u, row8[i].v, row8[i].w, 32.0f, 0, q);
            AdvEasyDrawTexture(0, col, q, 0.05f, 1);
        }
        x += row8[i].dw + 1;
    }
}

/* DisplayOptionPlateLevel0 before it reads the pad: hovering a row selects it, a click confirms
   it (on STEREO/MONO, ON/OFF: picks that value), a right click leaves the option screen */
void pc_options_mouse0(int port)
{
    ADV_WORK* ap = &AdvWork;
    int hit = -1;
    menu_mouse_take(&mt);
    if (mt.x >= 32 && mt.x < 608 && mt.y >= 160 && mt.y < 384) {   /* rows 32 px apart, their values on the right */
        hit = (int)(mt.y - 160) / 32;
        if (hit == 6 && mt.x >= row8_x() - 4) hit = 7;   /* EXIT's row: ENHANCED SETTINGS on the right */
        else if (mt.x >= 320 && hit != 0 && hit != 4 && hit != 1) hit = -1;
    }
    if (hit >= 0 && (mt.moved || mt.left) && hit != ap->OptIndex) { ap->OptIndex = hit; CallSystemSe(0, 2); }
    if (mt.left && hit >= 0) {
        if ((hit == 0 || hit == 4) && mt.x >= 336) Pad[port].press |= mt.x < 472 ? P_LEFT : P_RIGHT;   /* STEREO / MONO, ON / OFF */
        else Pad[port].press |= ok_button();
    } else if (mt.right) Pad[port].press |= cancel_button();
}

/* levels 1-3 (KEY CONFIG, SCREEN SETUP, MONITOR ADJUSTMENT): a right click goes back */
void pc_options_mouse_back(int port, int level)
{
    menu_mouse_take(&mt);
    if (mt.right) Pad[port].press |= level == 2 || level == 3 ? 0x10 : P_START;
}

/* ---------------------------------------------------------------- the pages */
enum { K_PAGE, K_SET, K_DEVICE, K_BIND, K_HEAD, K_GROUP, K_RESET, K_BACK };   /* K_HEAD, K_GROUP: headings */
typedef struct Row { int kind, idx; } Row;

#define MAX_ROWS 80
#define VISIBLE 9                         /* rows shown below the page row; more scroll */
#define X_LABEL 44.0f
#define X_VALUE 360.0f
#define Y_ROW0 184.0f
#define ROW_H 21.0f
#define Y_HELP (Y_ROW0 + VISIBLE * ROW_H + 6)   /* the help lines, below a rule */
#define HELP_LINES 3
#define Y_BOTTOM (Y_HELP + HELP_LINES * 14 + 4)
#define SC_ROW 0.55f
#define Z_PANEL 0.02f
#define Z_HI 0.3f

static struct {
    int page, sel, top, dev, slot;
    int capture;                 /* waiting for a key: the bind row + 1 */
    int wait_release;            /* after a capture: no input until everything is let go */
    char msg[96];
    int msg_t;
    unsigned char pending[256];  /* actions to run when the menu closes */
    int any_pending;
} st;

static Row rows[MAX_ROWS];
static int nrows;

static const char* const heads[] = { "", "THIRD PERSON (KEYBOARD AND MOUSE)", "HOTKEYS" };

/* every row of the page (none is left out: what F1 shows of a page, this shows too), headings first */
static void page_rows(void)
{
    for (int i = 0; i < g_settings_n && nrows < MAX_ROWS - 3; i++)
        if (g_settings[i].page == st.page) {
            if (g_settings[i].group) rows[nrows++] = (Row){ K_GROUP, i };
            rows[nrows++] = (Row){ K_SET, i };
        }
}

static void build_rows(void)
{
    nrows = 0;
    rows[nrows++] = (Row){ K_PAGE, 0 };
    page_rows();
    if (st.page == SP_CONTROLS) {   /* then the bindings */
        int last = -1;
        rows[nrows++] = (Row){ K_DEVICE, 0 };
        for (int i = 0; i < B_COUNT && nrows < MAX_ROWS - 3; i++) {
            int r = bind_order[i], g = bind_rows[r].group;
            if (!bind_row_shown(r, st.dev)) continue;
            if (g != last) { rows[nrows++] = (Row){ K_HEAD, g }; last = g; }
            rows[nrows++] = (Row){ K_BIND, r };
        }
        rows[nrows++] = (Row){ K_RESET, 0 };
    }
    rows[nrows++] = (Row){ K_BACK, 0 };
    if (st.sel >= nrows) st.sel = nrows - 1;
}

static void say(const char* m) { snprintf(st.msg, sizeof(st.msg), "%s", m); st.msg_t = 120; }

static void move_sel(int dir)
{
    for (int k = 0; k < nrows; k++) {
        st.sel = (st.sel + dir + nrows) % nrows;
        if (rows[st.sel].kind != K_HEAD && rows[st.sel].kind != K_GROUP) break;
    }
    CallSystemSe(0, 2);
}

static void set_page(int p)
{
    int dir = p < st.page ? -1 : 1;
    st.page = (p + SP_COUNT) % SP_COUNT;
    if (st.page == SP_EXTRAS && !g_extras_unlocked) st.page = (st.page + dir + SP_COUNT) % SP_COUNT;   /* vr218: hidden until unlocked */
    st.sel = 0; st.top = 0;
    CallSystemSe(0, 2);
}

static const Setting* row_set(const Row* r) { return r->kind == K_SET ? &g_settings[r->idx] : NULL; }

/* left/right or a click on an arrow */
static void change(const Row* r, int dir)
{
    const Setting* s = row_set(r);
    switch (r->kind) {
    case K_PAGE: set_page(st.page + dir); return;
    case K_DEVICE: st.dev ^= 1; st.slot = 0; CallSystemSe(0, 2); return;
    case K_BIND: st.slot = dir > 0; CallSystemSe(0, 2); return;
    case K_SET:
        if (s->flags & SET_ACTION) return;
        if (settings_locked(s)) { CallSystemSe(0, 1); say(settings_locked(s)); return; }
        settings_step(s, dir);
        CallSystemSe(0, 2);
        return;
    }
}

static void capture_begin(int bind_row)
{
    st.capture = bind_row + 1;
    bind_capture_start(st.dev);
    CallSystemSe(0, 3);
}

/* 0 stay, 1 back to the option screen */
static int activate(const Row* r)
{
    const Setting* s = row_set(r);
    switch (r->kind) {
    case K_PAGE: set_page(st.page + 1); return 0;
    case K_DEVICE: change(r, 1); return 0;
    case K_BIND: capture_begin(r->idx); return 0;
    case K_RESET:
        bind_reset(st.dev);
        settings_save_later();
        say(st.dev ? "Gamepad bindings reset to the defaults" : "Keyboard bindings reset to the defaults");
        CallSystemSe(0, 3);
        return 0;
    case K_BACK: CallSystemSe(0, 0); return 1;
    case K_SET:
        if (settings_locked(s)) { CallSystemSe(0, 1); say(settings_locked(s)); return 0; }
        if (s->flags & SET_ACTION) {
            if (s->flags & SET_NOW) settings_act(s);
            else { st.pending[r->idx] = 1; st.any_pending = 1; say("Done when you leave the menu"); }
            CallSystemSe(0, 3);
            return 0;
        }
        if (settings_is_bool(s) || s->names || s->name) { settings_step(s, 1); CallSystemSe(0, 2); }
        else CallSystemSe(0, 1);   /* numbers: left/right */
        return 0;
    }
    return 0;
}

static void capture_end(int code)
{
    int row = st.capture - 1;
    char nm[64];
    st.capture = 0;
    st.wait_release = 1;
    if (code == -2) { CallSystemSe(0, 0); say("Not changed"); return; }
    bind_set(row, st.dev, st.slot, code == -3 ? 0 : code);
    settings_save_later();
    CallSystemSe(0, 3);
    if (bind_swapped()) { snprintf(nm, sizeof(nm), "Swapped with %s", bind_swapped()); say(nm); }
    host_log("options: %s %s slot %d = %s\n", st.dev ? "pad" : "key", bind_rows[row].ini, st.slot + 1, bind_code_name(st.dev, code < 0 ? 0 : code, nm, sizeof(nm)));
}

/* ---------------------------------------------------------------- layout */
static float row_y(int i) { return Y_ROW0 + (i - st.top) * ROW_H; }

static int row_at(float x, float y)
{
    if (x < 30 || x > 610) return -1;
    if (y >= 150 && y < 174) return 0;   /* the page row */
    for (int i = st.top; i < nrows && i < st.top + VISIBLE; i++)
        if (i > 0 && y >= row_y(i) - 3 && y < row_y(i) + ROW_H - 3) return rows[i].kind == K_HEAD || rows[i].kind == K_GROUP ? -1 : i;
    return -1;
}

static void scroll_to_sel(void)
{
    int want = st.sel;
    if (want > 1 && (rows[want - 1].kind == K_HEAD || rows[want - 1].kind == K_GROUP)) want--;   /* with its heading */
    if (st.top < 1) st.top = 1;
    if (want > 0 && want < st.top) st.top = want;
    if (st.sel >= st.top + VISIBLE) st.top = st.sel - VISIBLE + 1;
    if (st.top > nrows - VISIBLE) st.top = nrows - VISIBLE;
    if (st.top < 1) st.top = 1;
}

/* text no wider than w (long names get narrower) */
static void text_fit(float x, float y, const char* s, float sc, float w, unsigned int col)
{
    float tw = pc_ui_text_w(s, sc), k = tw > w ? w / tw : 1.0f;
    pc_ui_text(x, y + (1.0f - k) * 9.0f, s, sc * k, col);
}

static unsigned int pulse(void) { return ((unsigned int)AdvWork.FlushCount2 + 255) << 24 | 0xFFFFFF; }

static void draw_row(int i, int sel)
{
    const Row* r = &rows[i];
    const Setting* s = row_set(r);
    float y = i ? row_y(i) : 152.0f;
    unsigned int dim = 0xFFA0A0A0, col = sel ? pulse() : dim;
    char label[128], val[128];
    const char* lock = s ? settings_locked(s) : NULL;
    if (lock) col = sel ? 0xFF909090 : 0xFF686868;
    if (sel) pc_ui_rect(36, y - 3, 604, y + ROW_H - 4, st.capture ? 0x50FFC040 : 0x40FFFFFF, Z_HI);
    switch (r->kind) {
    case K_PAGE: {
        char t[48];
        float w;
        snprintf(t, sizeof(t), "%s", settings_pages[st.page]);
        w = pc_ui_text_w(t, 0.72f);
        pc_ui_text(320 - w / 2, y - 2, t, 0.72f, sel ? pulse() : 0xFFE8E0C0);
        pc_ui_arrow(320 - w / 2 - 14, y + 9, -1, col);
        pc_ui_arrow(320 + w / 2 + 14, y + 9, 1, col);
        snprintf(t, sizeof(t), "%d/%d", st.page + 1, g_extras_unlocked ? SP_COUNT : SP_COUNT - 1);   /* vr218 */
        pc_ui_text(560, y + 2, t, 0.45f, 0xFF909090);
        pc_ui_text(48, y + 2, "L1  R1", 0.45f, 0xFF909090);
        return;
    }
    case K_GROUP: pc_ui_text(X_LABEL - 6, y + 2, g_settings[r->idx].group, 0.45f, 0xFFE0C080); return;
    case K_HEAD: {
        char t[64];
        if (r->idx == BG_GAME) snprintf(t, sizeof(t), "GAME (KEY CONFIG TYPE %c)", 'A' + pc_keytype());
        else snprintf(t, sizeof(t), "%s", heads[r->idx]);
        pc_ui_text(X_LABEL - 6, y + 2, t, 0.45f, 0xFFE0C080);
        return;
    }
    case K_DEVICE:
        pc_ui_text(X_LABEL, y, "DEVICE", SC_ROW, col);
        snprintf(val, sizeof(val), "%s", st.dev ? "Gamepad" : "Keyboard and mouse");
        break;
    case K_RESET: pc_ui_text(X_LABEL, y, "RESET TO DEFAULTS", SC_ROW, col); return;
    case K_BACK: pc_ui_text(X_LABEL, y, "BACK", SC_ROW, col); return;
    case K_BIND: {
        int conflict = bind_conflict(r->idx, st.dev);
        text_fit(X_LABEL, y, bind_row_label(r->idx, label, sizeof(label)), SC_ROW, 300, col);
        for (int k = 0; k < BIND_SLOTS; k++) {
            float x = 360 + k * 124;
            unsigned int c = conflict ? 0xFFFF6060 : k == st.slot && sel ? col : dim;
            const char* nm = st.capture == r->idx + 1 && k == st.slot ? "..." : bind_code_name(st.dev, bind_get(r->idx, st.dev, k), val, sizeof(val));
            if (sel && k == st.slot) pc_ui_rect(x - 4, y - 2, x + 116, y + ROW_H - 5, 0x40FFFFFF, Z_HI + 0.01f);
            text_fit(x, y, nm, SC_ROW, 110, c);
        }
        return;
    }
    case K_SET:
        snprintf(label, sizeof(label), "%s%s", s->label, (s->flags & SET_RESTART) ? " (next start)" : "");
        text_fit(X_LABEL, y, label, SC_ROW, 296, col);
        if (s->flags & SET_ACTION) {
            if (st.pending[r->idx]) pc_ui_text(X_VALUE, y, "(when you leave)", 0.5f, 0xFFE0C080);
            return;
        }
        settings_value(s, val, sizeof(val));
        break;
    }
    {   /* a value between arrows */
        float w = pc_ui_text_w(val, SC_ROW);
        if (w > 210) w = 210;
        if (!lock) { pc_ui_arrow(X_VALUE - 6, y + 9, -1, col); pc_ui_arrow(X_VALUE + 12 + w + 8, y + 9, 1, col); }
        text_fit(X_VALUE + 6, y, val, SC_ROW, 210, col);
    }
    if (sel && s && (s->flags & SET_CALIB)) {   /* the test pattern, right on the row: drawn into the picture, so calibrated with it */
        float x = 466;
        for (int k = 0; k < 2; k++, x += 5 * 12 + 4 + 6) {
            pc_ui_rect(x, y - 1, x + 5 * 12 + 4, y + ROW_H - 6, k ? 0xFFFFFFFF : 0xFF000000, Z_HI + 0.02f);
            for (int j = 0; j < CALIB_STEPS; j++)
                pc_ui_rect(x + 4 + j * 12, y + 3, x + 12 + j * 12, y + ROW_H - 10, 0xFF000000u | settings_calib[k][j] * 0x010101u, Z_HI + 0.03f);
        }
    }
}

static void draw_help(void)
{
    const Row* r = &rows[st.sel];
    const Setting* s = row_set(r);
    char t[512], nm[64];
    float y = Y_HELP;
    unsigned int col = 0xFFC0C0C0;
    t[0] = 0;
    if (st.capture) {
        snprintf(t, sizeof(t), "Press a %s for %s ... (Esc cancels, Delete clears, %d s)", st.dev ? "gamepad button" : "key or mouse button",
                 bind_rows[st.capture - 1].name, (bind_capture_left_ms() + 999) / 1000);
        col = 0xFFFFE080;
    } else if (st.msg_t > 0) { st.msg_t--; snprintf(t, sizeof(t), "%s", st.msg); col = 0xFFFFE080; }
    else switch (r->kind) {
    case K_PAGE: snprintf(t, sizeof(t), "Left/right or L1/R1 change the page, up/down choose a setting. Changes are saved at once; F1 shows the same settings."); break;
    case K_DEVICE: snprintf(t, sizeof(t), "%s", st.dev ? bind_pad_desc() : "The game's buttons, third-person keys and hotkeys."); break;
    case K_RESET: snprintf(t, sizeof(t), "All %s bindings back to the defaults.", st.dev ? "gamepad" : "keyboard and mouse"); break;
    case K_BACK: snprintf(t, sizeof(t), "Back to the OPTION screen (also cancel or a right click)."); break;
    case K_BIND: {
        int c = bind_conflict(r->idx, st.dev);
        if (c) { snprintf(t, sizeof(t), "Also bound to: %s. Bind one of them to something else.", bind_rows[c - 1].name); col = 0xFFFF8080; }
        else if (r->idx == B_QUICKTURN || r->idx == B_TPS_QUICKTURN) snprintf(t, sizeof(t), "Tap it: holds back, then presses run, as the game's own quick turn.");
        else snprintf(t, sizeof(t), "Confirm: change. Left/right: first or second binding (%s).", bind_code_name(st.dev, bind_get(r->idx, st.dev, st.slot), nm, sizeof(nm)));
        break;
    }
    case K_SET: {
        const char* lock = settings_locked(s);
        snprintf(t, sizeof(t), "%s%s%s%s", lock ? lock : "", lock ? " " : "", (s->flags & SET_RESTART) ? "Takes effect at the next start. " : "", s->help ? s->help : "");
        if (lock) col = 0xFFFFC060;
        if (s->flags & SET_ACTION && !lock && !(s->flags & SET_NOW)) snprintf(t + strlen(t), sizeof(t) - strlen(t), "%sRuns when you leave the menu.", s->help ? " " : "");
        break;
    }
    }
    pc_ui_text_wrap(X_LABEL - 4, y, 560, t, 0.45f, col, HELP_LINES);
}

static void draw(void)
{
    QUAD* q = &Qtex[1];
    SetQuadPos(96.0f, 96.0f, 448.0f, 48.0f, q);   /* the OPTION title, as level 0 */
    SetQuadUv2(0, 320.0f, 448.0f, 48.0f, 2, q);
    AdvEasyDrawTexture(1, -1, q, 0.05f, 1);
    FlushPlate();
    pc_ui_rect(28, 146, 612, Y_BOTTOM, 0xA0000000, Z_PANEL);
    pc_ui_rect(36, 176, 604, 177, 0x60FFFFFF, Z_PANEL + 0.001f);
    pc_ui_rect(36, Y_HELP - 4, 604, Y_HELP - 3, 0x60FFFFFF, Z_PANEL + 0.001f);
    draw_row(0, st.sel == 0);
    for (int i = st.top; i < nrows && i < st.top + VISIBLE; i++) draw_row(i, i == st.sel);
    if (st.top > 1) {   /* more above / below */
        NJS_POLYGON_VTX v[3] = { { 594, 187, 0.3f, 0xFFA0A0A0 }, { 606, 187, 0.3f, 0xFFA0A0A0 }, { 600, 180, 0.3f, 0xFFA0A0A0 } };
        njDrawPolygon(v, 3, 1);
    }
    if (nrows > st.top + VISIBLE) {
        NJS_POLYGON_VTX v[3] = { { 594, Y_HELP - 14, 0.3f, 0xFFA0A0A0 }, { 606, Y_HELP - 14, 0.3f, 0xFFA0A0A0 }, { 600, Y_HELP - 7, 0.3f, 0xFFA0A0A0 } };
        njDrawPolygon(v, 3, 1);
    }
    draw_help();
    AdvEasySetTextureList(0);   /* the option screen's textures again (the font set its own) */
}

/* vr218: the EXTRAS page (cheats: infinite health and ammo, items, teleport...) is hidden; in these pages the Touch
   controllers' left grip twice, X, Y, X (each within 3 s of the last) unlocks it until the game closes */
static void extras_code(void)
{
    static const int seq[5] = { 1, 1, 2, 3, 2 };   /* 1 left grip, 2 X, 3 Y */
    static int pos, was;
    static uint64_t last;
    VrInput in;
    int now, b;
    if (g_extras_unlocked || !xr_input(&in)) return;
    now = (in.lgrip > 0.5f ? 1 : 0) | ((in.buttons & VRB_X) ? 2 : 0) | ((in.buttons & VRB_Y) ? 4 : 0);
    b = now & ~was;
    was = now;
    if (!b) return;
    if (pos && SDL_GetTicks() - last > 3000) pos = 0;
    last = SDL_GetTicks();
    b = (b & 1) ? 1 : (b & 2) ? 2 : 3;
    if (b == seq[pos]) pos++;
    else pos = b == seq[0] ? 1 : 0;
    if (pos < 5) return;
    pos = 0;
    g_extras_unlocked = 1;   /* vr219: until the game closes - not kept in recvx.ini */
    host_log("options: EXTRAS unlocked (the code)\n");
    CallSystemSe(0, 3);
    st.page = SP_EXTRAS; st.sel = 0; st.top = 0;
    say("EXTRA MENU");
}

/* 0 stay, 1 back */
static int input(int port)
{
    unsigned int press = Pad[port].press, rept = Pad[port].Rept;
    unsigned int ok = ok_button() | P_START, cancel = cancel_button() | P_SELECT;
    int hit;
    menu_mouse_take(&mt);
    if (st.capture) {
        int c = bind_capture_poll();
        if (c != -1) capture_end(c);
        return 0;
    }
    if (st.wait_release) {
        if (bind_any_down() || g_menu_mouse.held) return 0;
        st.wait_release = 0;
    }
    extras_code();   /* vr218 */
    hit = row_at(mt.x, mt.y);
    if (hit >= 0 && (mt.moved || mt.left) && hit != st.sel) { st.sel = hit; CallSystemSe(0, 2); }
    if (rows[st.sel].kind == K_BIND && hit == st.sel && (mt.moved || mt.left)) st.slot = mt.x >= 360 + 120;
    if (mt.wheel) move_sel(mt.wheel > 0 ? -1 : 1);   /* as the d-pad, like the game's menus */
    if (mt.left && hit >= 0) {
        const Row* r = &rows[hit];
        if (r->kind == K_PAGE) change(r, mt.x < 320 ? -1 : 1);
        else if ((r->kind == K_SET && !(g_settings[r->idx].flags & SET_ACTION)) || r->kind == K_DEVICE) {
            if (mt.x >= X_VALUE - 16) change(r, mt.x < X_VALUE + 4 ? -1 : 1);   /* the "<" arrow: back */
            else activate(r);
        } else return activate(r);
        return 0;
    }
    if (mt.right) { CallSystemSe(0, 0); return 1; }
    if (press & P_L1) set_page(st.page - 1);
    else if (press & P_R1) set_page(st.page + 1);
    else if (rept & P_UP) move_sel(-1);
    else if (rept & P_DOWN) move_sel(1);
    else if (rept & P_LEFT) change(&rows[st.sel], -1);
    else if (rept & P_RIGHT) change(&rows[st.sel], 1);
    else if (press & ok) return activate(&rows[st.sel]);
    else if (press & cancel) { CallSystemSe(0, 0); return 1; }
    return 0;
}

/* DisplayOptionPlate level 4: returns the next level (4 stay, 0 back) */
int pc_options_pc(int port, int flag)
{
    int back = 0;
    build_rows();
    if (flag) back = input(port);
    build_rows();
    scroll_to_sel();
    draw();
    if (back) { st.capture = 0; st.wait_release = 0; }
    return back ? 0 : 4;
}

/* Adv_GameOptionScreen ends: the actions chosen run now (the game is back in control), the
   cursor goes */
void pc_options_close(void)
{
    if (st.any_pending)
        for (int i = 0; i < g_settings_n && i < (int)sizeof(st.pending); i++)
            if (st.pending[i]) { st.pending[i] = 0; settings_act(&g_settings[i]); }
    st.any_pending = 0;
    st.capture = 0;
    g_menu_mouse.active = 0;
}

/* ---------------------------------------------------------------- vr145: the Select menu */
/* In play, Select (the left menu button in VR) no longer opens the OPTION screen straight away: the
   game is paused with its own pause (system.c bhSysCallEvent: sp_flg 0, the PAUSE screen of the
   screen saver task, mode 10) and this small menu is drawn in place of the PAUSE message:
   CONTINUE, OPTIONS (the OPTION screen as before) and RETURN TO TITLE (asked again first).
   Input: the pad as the pause reads it (Ps2_pad) and the Touch controllers directly (in play they
   do not reach the pad). Start still ends the pause as the game's own does: the menu goes with it. */
#include "../../decomp/include/ps2/veronica/prog/ps2_sg_pad.h"
#include "../src/host/xr.h"
extern unsigned char Pause_Flag;
extern int coop_on;
void bhReturnTitle(void);

static struct { int open, sel, ask, act; unsigned int prev_pad, prev_vr; } sm;
enum { SM_CONTINUE, SM_OPTIONS, SM_TITLE, SM_N };
static const char* const sm_names[SM_N] = { "CONTINUE", "OPTIONS", "RETURN TO TITLE" };

static int sm_can_open(void)
{
    return sys && plp && (sys->tk_flg & 0x80) && !coop_on && !(sys->ss_flg & 0xA0400000) && !(sys->cb_flg & 0x7)
        && !(sys->st_flg & 0x1C040208) && !(sys->st_flg & 0x4);
}

static void sm_pause(void)   /* as bhSysCallEvent's pause */
{
    sys->ss_flg |= 0x80000000;
    Pause_Flag = 1;
    sys->pau_spbak = sys->sp_flg;
    sys->pau_pad_on = sys->pad_on; sys->pau_pad_ps = sys->pad_ps; sys->pau_pad_old = sys->pad_old;
    sys->sp_flg = 0;
    sys->ssv_md0 = 10;
}

static void sm_unpause(void)   /* as its end of the pause (Start) */
{
    sys->ss_flg &= ~0x80000000;
    sys->sp_flg = sys->pau_spbak;
    sys->pad_on = sys->pau_pad_on; sys->pad_ps = sys->pau_pad_ps; sys->pad_old = sys->pau_pad_old;
    sys->ssv_md0 = 0; sys->ssv_tim = 0; sys->ssv_an = 0;
    Pause_Flag = 0;
}

/* pad.c bhSetPad (after trade_pad): Select opens the menu; the action chosen runs here, with the game back */
unsigned int selmenu_pad(unsigned int pad)
{
    static unsigned int held;
    if (sm.act) {
        int a = sm.act - 1;
        sm.act = 0;
        if (a == SM_OPTIONS) { host_log("selmenu: options\n"); return pad | 0x8000; }   /* system.c opens the OPTION screen */
        if (a == SM_TITLE && sys && (sys->tk_flg & 0x80)) { host_log("selmenu: return to title\n"); bhReturnTitle(); return 0; }
    }
    if (held) { held &= pad; return pad & ~0x8000; }   /* Select still held after the menu: nothing more */
    if ((pad & 0x8000) && !(sys->pad_old & 0x8000) && sm_can_open()) {
        sm.open = 1; sm.sel = 0; sm.ask = 0; sm.prev_pad = ~0u; sm.prev_vr = ~0u;
        held = pad & 0x8000;
        sm_pause();
        host_log("selmenu: open (paused)\n");
        return pad & ~0x8000;
    }
    return pad;
}

/* system.c bhSysCallScreenSaver, mode 10 (the pause): 1 = the menu drew here (no PAUSE message) */
int pc_selmenu_draw(void)
{
    unsigned int pad, press, vr = 0, vpress;
    VrInput in;
    int up, down, ok, back, i;
    float y0 = 170.0f;
    if (!sm.open) return 0;
    if (!(sys->ss_flg & 0x80000000)) { sm.open = 0; host_log("selmenu: closed by Start\n"); return 0; }
    pad = Ps2_pad.pad1.on;
    if (xr_input(&in)) {
        vr = (in.buttons & (VRB_A | VRB_B | VRB_MENU)) | (in.ly > 0.5f ? 0x100 : 0) | (in.ly < -0.5f ? 0x200 : 0);
    }
    press = sm.prev_pad == ~0u ? 0 : pad & ~sm.prev_pad;   /* (what was held when it opened is not a press) */
    vpress = sm.prev_vr == ~0u ? 0 : vr & ~sm.prev_vr;
    sm.prev_pad = pad; sm.prev_vr = vr;
    up = (press & 0x1000) || (vpress & 0x100);
    down = (press & 0x4000) || (vpress & 0x200);
    ok = (press & ok_button()) || (vpress & VRB_A);
    back = (press & (cancel_button() | 0x100)) || (vpress & (VRB_B | VRB_MENU));
    if (sm.ask) {
        if (up || down) { sm.ask = sm.ask == 1 ? 2 : 1; CallSystemSe(0, 2); }
        if (ok && sm.ask == 1) { CallSystemSe(0, 3); sm.open = 0; sm_unpause(); sm.act = 1 + SM_TITLE; }
        else if (back || ok) { CallSystemSe(0, 1); sm.ask = 0; }
    } else {
        if (up) { sm.sel = (sm.sel + SM_N - 1) % SM_N; CallSystemSe(0, 2); }
        if (down) { sm.sel = (sm.sel + 1) % SM_N; CallSystemSe(0, 2); }
        if (ok) {
            CallSystemSe(0, 3);
            if (sm.sel == SM_TITLE) sm.ask = 2;   /* NO first */
            else { sm.open = 0; sm_unpause(); if (sm.sel == SM_OPTIONS) sm.act = 1 + SM_OPTIONS; host_log("selmenu: %s\n", sm_names[sm.sel]); }
        } else if (back) { CallSystemSe(0, 1); sm.open = 0; sm_unpause(); host_log("selmenu: closed\n"); }
    }
    if (!sm.open) return 1;
    njColorBlendingMode(0, 8); njColorBlendingMode(1, 6);
    pc_ui_rect(180, y0 - 20, 460, y0 + 150, 0xC0000000, 1.3f);
    if (!sm.ask) {
        pc_ui_text(320 - pc_ui_text_w("PAUSE", 0.8f) / 2, y0 - 10, "PAUSE", 0.8f, 0xFFC0C0C0);
        for (i = 0; i < SM_N; i++)
            pc_ui_text(320 - pc_ui_text_w(sm_names[i], 0.7f) / 2, y0 + 30 + 32 * i, sm_names[i], 0.7f, i == sm.sel ? pulse() : 0xFF808080);
    } else {
        static const char* const q = "RETURN TO TITLE?", * const q2 = "Progress not saved is lost.";
        pc_ui_text(320 - pc_ui_text_w(q, 0.7f) / 2, y0, q, 0.7f, 0xFFFFFFFF);
        pc_ui_text(320 - pc_ui_text_w(q2, 0.5f) / 2, y0 + 30, q2, 0.5f, 0xFFC0C0C0);
        pc_ui_text(320 - pc_ui_text_w("YES", 0.7f) / 2, y0 + 66, "YES", 0.7f, sm.ask == 1 ? pulse() : 0xFF808080);
        pc_ui_text(320 - pc_ui_text_w("NO", 0.7f) / 2, y0 + 98, "NO", 0.7f, sm.ask == 2 ? pulse() : 0xFF808080);
    }
    return 1;
}

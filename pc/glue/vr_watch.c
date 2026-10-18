/*
 * VR watch (vr108, ini vr_debug_log = 1): rules that must always hold where the VR code changes
 * the game (her place, her facing, the inventory, the fire button), checked once a game frame.
 * A rule broken: one "VIGIA" line in recvx.log with what was happening, and the first times the
 * last 15 frames; at exit a summary of how often each rule broke and what the checks cost.
 *
 * Cheap on purpose: only comparisons on values the VR code already has, on the game thread (not
 * the headset's), nothing written unless a rule breaks, each rule's lines capped. The checks time
 * themselves (the summary), so they can be ruled out as a cause of slowness.
 */
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "vr_watch.h"

extern void host_log(const char* fmt, ...);
extern uint64_t SDL_GetTicksNS(void);   /* SDL3 (the game glue does not see its headers) */

enum { R_JUMP, R_TURN, R_MAG, R_FIRE, R_UNDO, R_PULL, R_AMMO, R_COUNT, R_N };
static const char* const r_name[R_N] = {
    "posicao: pulo sem porta/cena",
    "direcao: giro brusco sem cena",
    "pente tirado com B pendurado (troca de sala/personagem)",
    "gatilho: atirar apertado pelo VR sem o dedo",
    "mira (vr95): movimento grande do jogo desfeito",
    "duas armas: puxao perdido",
    "municao: item que nao e arma nem municao ganhou balas",
    "municao: mudou pelo jogo durante o jogo (sem cena/mensagem)",
};
static unsigned r_hits[R_N];
#define R_LINES 12   /* lines a rule writes at most */
#define R_DUMPS 2    /* times a rule writes the last frames */

#define TRACE_N 15
static VwFrame trace[TRACE_N];
static int trace_at, trace_len;
static VwFrame last;
static int last_ok, fire_frames;
static uint64_t cost_ns, cost_max, cost_n;

static void summary(void)
{
    int any = 0;
    host_log("vr-watch: summary - checks: %u frames, %.1f us each on average, %.1f us at most\n",
             (unsigned)cost_n, cost_n ? cost_ns / 1e3 / cost_n : 0.0, cost_max / 1e3);
    for (int r = 0; r < R_N; r++)
        if (r_hits[r]) { host_log("vr-watch:   VIGIA %-60s %u time(s)\n", r_name[r], r_hits[r]); any = 1; }
    if (!any) host_log("vr-watch:   no rule broken\n");
}

static void dump(void)
{
    for (int k = 0; k < trace_len; k++) {
        const VwFrame* f = &trace[(trace_at - trace_len + k + TRACE_N) % TRACE_N];
        host_log("vr-watch:     frame -%2d: pos %.2f %.2f m, facing %.0f, mode %d/%d/%d, room RM_%04d, char %d, %s, gun rounds %d, mag out %d, fire VR %d finger %d\n",
                 trace_len - k, f->x, f->z, f->ay * (360.0f / 65536.0f), f->m0, f->m1, f->m2, f->room, f->chr,
                 f->event ? "event" : f->message ? "message" : "play", f->gun_rounds, f->mag_pending, f->fire_vr, f->fire_finger);
    }
}

/* a rule broken: the line (capped), and the last frames the first times */
static void broke(int r, const char* fmt, ...)
{
    char b[300];
    va_list ap;
    if (++r_hits[r] > R_LINES) return;
    va_start(ap, fmt);
    vsnprintf(b, sizeof(b), fmt, ap);
    va_end(ap);
    host_log("vr-watch: VIGIA %s: %s%s\n", r_name[r], b, r_hits[r] == R_LINES ? " (no more lines of this rule)" : "");
    if (r_hits[r] <= R_DUMPS) dump();
}

void vw_frame(const VwFrame* f)
{
    static int reg;
    uint64_t t0 = SDL_GetTicksNS(), t;
    if (!reg) { atexit(summary); reg = 1; }
    trace[trace_at] = *f; trace_at = (trace_at + 1) % TRACE_N; if (trace_len < TRACE_N) trace_len++;
    {   /* vr109: frames since the game last placed her itself - a room or character change (her place
           is set a few frames after the room number), a scene, a state not her own walking (mode0
           not 1: a grab, a scripted move), the stairs (skipped: a step of 1.2 m, a snap at the end).
           The vr108 log: all its jumps and turns came from these, or from the game turning her to
           the enemy as the twin guns come up (mode2 64/164) */
        static int quiet;
        int placed = !last_ok || f->room != last.room || f->chr != last.chr || f->event || f->m0 != 1 || f->m2 == 14 || f->m2 == 15;
        quiet = placed ? 0 : quiet + 1;
        if (quiet > (f->room != last.room ? 60 : 15)) {
            float dx = f->x - last.x, dz = f->z - last.z, d = sqrtf(dx * dx + dz * dz);
            float da = abs((short)(f->ay - last.ay)) * (360.0f / 65536.0f);
            if (d > 0.5f) broke(R_JUMP, "%.2f m in one frame (room RM_%04d, mode %d/%d/%d)", d, f->room, f->m0, f->m1, f->m2);
            if (da > 90.0f && f->m2 != 64 && f->m2 != 164)   /* (walking turns of ~50 a frame are the game's own) */
                broke(R_TURN, "%.0f degrees in one frame (room RM_%04d, mode %d/%d/%d)", da, f->room, f->m0, f->m1, f->m2);
        }
    }
    if (last_ok && f->mag_pending && last.mag_pending && (f->room != last.room || f->chr != last.chr || !f->mag_inv_ok))
        broke(R_MAG, "room RM_%04d -> RM_%04d, character %d -> %d, same inventory %d", last.room, f->room, last.chr, f->chr, f->mag_inv_ok);
    fire_frames = f->fire_vr && !f->fire_finger ? fire_frames + 1 : 0;
    if (fire_frames == 30) broke(R_FIRE, "1 s without a trigger pulled (mode %d/%d/%d)", f->m0, f->m1, f->m2);
    last = *f; last_ok = 1;
    if (cost_n && cost_n % 9000 == 0) summary();   /* vr109: also every 5 minutes (the vr108 game was closed without the exit summary) */
    t = SDL_GetTicksNS() - t0;
    cost_ns += t; cost_n++; if (t > cost_max) cost_max = t;
}

void vw_undo(float metres, int m2)
{
    if (metres > 0.3f) broke(R_UNDO, "%.2f m of the game's move undone in one frame while aiming (mode2 %d) - a push, a grab?", metres, m2);
}

void vw_pull_dropped(int side, int rounds)
{
    if (rounds == 0) return;   /* vr109: an empty gun only clicks - no shot to come (the vr108 log's drops) */
    broke(R_PULL, "%s trigger: its gun did not fire within 700 ms (%d rounds)", side ? "left" : "right", rounds);
}

void vw_inv(int slot, unsigned item0, unsigned item1, unsigned n0, unsigned n1, unsigned type1, const char* who, int play)
{
    if (n1 > n0 && n1 > 0 && item1 && !(type1 & 0x3))   /* (itemdata type: 0x1 weapon, 0x2 ammo - read from sub1.c, a deduction) */
        broke(R_AMMO, "slot %d item %u (type %03x) %u -> %u rounds, by %s", slot, item1, type1, n0, n1, who ? who : "the game");
    else if (!who && play && item0 == item1 && n1 != n0 && !(n1 + 1 == n0))   /* (one less: a shot) */
        broke(R_COUNT, "slot %d item %u (type %03x) %u -> %u", slot, item1, type1, n0, n1);
}

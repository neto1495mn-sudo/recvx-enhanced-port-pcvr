/* Modern controls for the fixed cameras (docs/USER_GUIDE.md "Modern controls"), as the
   alternate controls of the RE HD remasters: the left stick or the d-pad points where the
   character walks on screen. Each frame the direction becomes the game's own input - forward
   (and run when the stick is pushed far) - and the character turns towards it after the mode
   handler, so the player state machine, its motions, doors, stairs and pushing stay the game's.
   Aiming, the quick-turn row and third person keep the original input. Game thread; the
   direction math is pc/src/input/modern.c. */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../src/host/modern.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int pad_move_dir(float* x, float* y);   /* pad.c: screen direction (x right, y up), 1 = analog */
int pad_quickturn_held(void);
uint64_t gs_frame_count(void);

#define RUN_PUSH 0.7f                   /* the stick pushed this far runs */

static ModernHold hold;
static float target;                    /* the angle to turn to this frame */
static int steer;                       /* modern_pad asked modern_player to turn */

/* normal play: our player in control; no event, door, menu, first person or attract demo */
static int in_play(void)
{
    return plp == &ply && plp->mnwP && plp->mlwP && (sys->sp_flg & 0x40) && !(sys->ss_flg & 0x400000) &&
           !(sys->st_flg & 0x800001) && !(sys->gm_flg & 0x49) && !(sys->cb_flg & 0x5) && !(sys->ts_flg & 0x80) &&
           !*(int*)&sys->mn_mode0 && !*(int*)&sys->mn_md0 && plp->mode0 == 1;
}

static void trace(float x, float y, int analog, int held_old, unsigned pad)
{
    static int on = -1;
    const float* m = (const float*)cam.mtx;
    float px, py, pz;
    if (on < 0) on = getenv("RECVX_MODERN_TRACE") != NULL;
    if (!on) return;
    /* the player's place on screen, x right, y up (camera space: x right, y down, z ahead) */
    px = plp->px * m[0] + plp->py * m[4] + plp->pz * m[8] + m[12];
    py = plp->px * m[1] + plp->py * m[5] + plp->pz * m[9] + m[13];
    pz = plp->px * m[2] + plp->py * m[6] + plp->pz * m[10] + m[14];
    printf("modern: frame %u dir %.2f %.2f%s cut %d%s target %.1f ay %.1f mode %d/%d/%d pad %x at (%.1f %.1f) view (%.3f %.3f) cam (%.0f %.0f %.0f) ay %.0f az %.0f\n",
           (unsigned)gs_frame_count(), x, y, analog ? " analog" : "", cam.ncut, held_old ? " (held)" : "",
           target * (360.0f / 65536.0f), (short)plp->ay * (360.0f / 65536.0f), plp->mode1, plp->mode2, plp->mode3, pad,
           plp->px, plp->pz, pz != 0 ? px / pz : 0, pz != 0 ? -py / pz : 0, cam.px, cam.py, cam.pz, cam.ay * (360.0f / 65536.0f), cam.az * (360.0f / 65536.0f));
}

/* pad.c bhSetPad, before pad_ps is derived (virtual bits: 0x1 forward, 0x2 back, 0x4/0x8 turn,
   0x20/0x40 aim up/down, 0x400 run) */
unsigned int modern_pad(unsigned int pad)
{
    const float* m = (const float*)cam.mtx;
    float x, y, rx, rz, d;
    int analog, cut;
    steer = 0;
    if (!g_modern_on || tps_on || !m || !sys || !plp || !in_play() || plp->mode1 != 0) {
        hold.held = 0;   /* aiming: the original input */
        return pad;
    }
    if (pad_quickturn_held()) {   /* the quick-turn row: back, then run, as without; the stick does not walk */
        hold.held = 0;
        return (pad & ~0x6F) | 0x2;
    }
    analog = pad_move_dir(&x, &y);
    if (x == 0 && y == 0) { hold.held = 0; return pad; }

    /* the camera on screen: its right axis (first column of the view matrix) on the ground */
    rx = m[0]; rz = m[8];
    cut = cam.ncut | cam.camver << 8;
    modern_hold(&hold, atan2f(x, y) * (32768.0f / 3.14159265f), cut, &rx, &rz);
    target = modern_target(rx, rz, x, y);
    d = modern_wrap(target - (short)plp->ay);

    pad &= ~0x6F;
    if (plp->mode2 > 13 || plp->mode2 == 12) {   /* stairs, steps, pushing, ...: on while it points ahead */
        if (fabsf(d) < MODERN_DEG(90.0f)) pad |= 0x1;
    } else {
        if (fabsf(d) > MODERN_DEG(100.0f) && !(plp->flg & 0x100000)) pad |= d > 0 ? 0x4 : 0x8;   /* turn on the spot first */
        else pad |= 0x1;
        if (analog && x * x + y * y >= RUN_PUSH * RUN_PUSH) pad |= 0x400;
        steer = 1;
    }
    trace(x, y, analog, hold.cut != cut, pad);
    return pad;
}

/* player.c bhControlPlayer, after the mode handler: turn towards the direction; not in the
   game's own turns (the quick turn, auto-aim) or special moves */
void modern_player(void)
{
    int s = steer;
    steer = 0;
    if (!s || plp != &ply || plp->mode0 != 1 || plp->mode1 != 0 || plp->mode2 > 13 || plp->mode2 == 12 || (plp->flg & 0x100000))
        return;
    plp->ay += (int)modern_turn((short)plp->ay, target);
}

/* Modern controls: the direction math (pc/src/host/modern.h), pure; the game side is
   pc/glue/modern_glue.c */
#include "../host/modern.h"
#include <math.h>

int g_modern_on;

#define UNITS (32768.0f / 3.14159265f)

float modern_wrap(float a)
{
    a = fmodf(a, 65536.0f);
    return a > 32768.0f ? a - 65536.0f : a < -32768.0f ? a + 65536.0f : a;
}

/* ponytail: by the camera's orientation, as the HD remasters, not the exact on-screen direction
   at the player's place: far off-centre in a wide shot, "up" runs a few degrees off screen-up
   under perspective; project the ground axes at the player if that ever matters */
float modern_target(float rx, float rz, float sx, float sy)
{
    float n = sqrtf(rx * rx + rz * rz), dx, dz;
    if (n < 1e-6f) rx = 1, rz = 0, n = 1;   /* no camera: as one looking along -z */
    rx /= n; rz /= n;
    /* screen up on the ground: the right axis turned a quarter towards the view, (rz, -rx) */
    dx = sx * rx + sy * rz;
    dz = sx * rz - sy * rx;
    return atan2f(-dx, -dz) * UNITS;
}

void modern_hold(ModernHold* h, float stick, int cut, float* rx, float* rz)
{
    if (!h->held || h->cut == cut || fabsf(modern_wrap(stick - h->ref)) > MODERN_RELEASE) {
        h->held = 1; h->cut = cut; h->rx = *rx; h->rz = *rz; h->ref = stick;
        return;
    }
    *rx = h->rx; *rz = h->rz;
}

float modern_turn(float ay, float target)
{
    float d = modern_wrap(target - ay) * 0.4f, max = MODERN_DEG(30.0f);
    return d > max ? max : d < -max ? -max : d;
}

/* Test aid RECVX_POSE_TRACE=file: one line per game frame with the player's body as drawn (the
   model the player is drawn as, "You appear as" included): the player's state and every node's
   position in the frame of the drawn model's base matrix (root position and facing taken out), so
   a twitch shows as nodes jumping back and forth while the character holds still.
   game.c bhAllDrawModel marks the player's draw (pose_trace_mark), MdlPut.c bhPutModel logs it. */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int pose_trace_mark;

void pose_trace(BH_PWORK* p)
{
    static FILE* f;
    static int init, last = -1;
    const float* b;
    float ax[3][3];
    int i, j, k, n;
    if (!init++ && getenv("RECVX_POSE_TRACE")) f = fopen(getenv("RECVX_POSE_TRACE"), "w");
    if (!f || !p || !p->mlwP || !p->mlwP->owP || !p->mtx || !plp || sys->gfrm_ct == last) return;
    last = sys->gfrm_ct;
    b = (const float*)p->mtx;
    for (i = 0; i < 3; i++) {
        float l = sqrtf(b[i * 4] * b[i * 4] + b[i * 4 + 1] * b[i * 4 + 1] + b[i * 4 + 2] * b[i * 4 + 2]);
        for (j = 0; j < 3; j++) ax[i][j] = l > 0 ? b[i * 4 + j] / l : 0;
    }
    n = p->mlwP->obj_num < 64 ? p->mlwP->obj_num : 64;
    fprintf(f, "%d %d %d %d %d %d %d %d %x %d %d %.3f %d", sys->gfrm_ct, plp->mode0, plp->mode1, plp->mode2, plp->mode3, plp->wpnr_no,
            (int)plp->mtn_no, plp->frm_no >> 16, plp->at_flg, (short)plp->ay, plp->wax, plp->py, n);
    for (i = 0; i < n; i++) {
        const float* w = &p->mlwP->owP[i].mtx[12];
        for (k = 0; k < 3; k++)
            fprintf(f, " %.3f", (w[0] - b[12]) * ax[k][0] + (w[1] - b[13]) * ax[k][1] + (w[2] - b[14]) * ax[k][2]);
    }
    fputc('\n', f);
    fflush(f);
}

/* Debug hooks that need the game's types. Called on the game thread once per game frame (stream_submit). */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"

static const char* task_names[23] = {
    "Init", "Warning", "Ipl", "Firstmovie", "Title", "Opening", "Pad", "Game", "Event", "Itemselect", "Map",
    "Doordemo", "Movie", "Ending", "Gameover", "Typewriter", "Option", "CompEvent", "DiscChange", "SoundMuseum",
    "Monitor", "SndMonitor", "ScreenSaver" };

void glue_debug_frame(uint64_t frame)
{
    static int checked, on;
    static unsigned int last_tk = 0xFFFFFFFF, last_ts;
    if (!checked++) on = getenv("RECVX_TRACE_TASKS") != NULL;
    if (!on) return;
    if (sys->tk_flg != last_tk || sys->ts_flg != last_ts) {
        char buf[512] = "";
        for (int i = 0; i < 23; i++)
            if (sys->tk_flg & (1u << i)) { strcat(buf, task_names[i]); strcat(buf, (sys->ts_flg & (1u << i)) ? "(susp) " : " "); }
        fprintf(stderr, "[frame %u] tasks: %s(tk=%08x ts=%08x)\n", (unsigned)frame, buf, sys->tk_flg, sys->ts_flg);
        last_tk = sys->tk_flg; last_ts = sys->ts_flg;
    }
}

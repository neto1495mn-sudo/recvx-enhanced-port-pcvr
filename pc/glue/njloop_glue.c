/* The decomp's main loop (decomp njloop.c), here instead: that file only sees ninjapad.h's declaration of njUserMain,
   whose Sint32 is `long` - 64 bits in this build (pc_prelude.h) - so the compare "njUserMain() < NJD_USER_CONTINUE" read the
   upper half of the result (edx here, r1 on ARM), left over from the callee: a random exit of the game (seen on the Quest
   when a Battle Game ended, and at the start). njUserMain returns a 32-bit int. */
void njUserInit(void);
int njUserMain(void);
void njWaitVSync(void);
void njUserExit(void);

int main(int argc, char* argv[])   /* (main = recvx_game_main, as the decomp's) */
{
    njUserInit();
    while (1) {
        if (njUserMain() < 0) break;   /* NJD_USER_CONTINUE is 0 */
        njWaitVSync();
    }
    njUserExit();
    return 0;
}

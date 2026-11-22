#include "../../../ps2/veronica/prog/njloop.h"

// 100% matching!
int main(int argc, char *argv[])
{
    njUserInit(); 
    
    while (TRUE) 
    { 
        int um = (int)njUserMain();   /* (long is 64 bits here and this file only sees ninjapad.h's declaration, which returns Sint32 = long: the compare read r1, left over from the callee - a random exit) */
        if (um < NJD_USER_CONTINUE) 
        { 
            break;
        } 
        
        njWaitVSync();
    } 
    
    njUserExit(); 
} 

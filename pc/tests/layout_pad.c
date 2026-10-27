/* Test aid (-DRECVX_LAYOUT_PAD=n, pc/tools/layout_check.sh): n unused bytes at the start of the
   image's code, data and bss, linked first, so every game and platform symbol moves. Frames must
   not change: a difference means something depends on where code or data lie. */
#define S_(x) #x
#define S(x) S_(x)
__asm__(".text\n.skip " S(RECVX_LAYOUT_PAD) "\n"
        ".data\n.skip " S(RECVX_LAYOUT_PAD) "\n"
        ".bss\n.skip " S(RECVX_LAYOUT_PAD) "\n.text\n");

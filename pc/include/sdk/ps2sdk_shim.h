/*
 * Minimal stand-ins for the Sony PS2 SDK headers, for the PC build.
 * Only the types/prototypes the kept decomp sources reference are declared;
 * implementations live in pc/src/sdk/. No SDK code is used.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- eetypes.h ---- */
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
/* EE long is 64-bit. Decomp code sees 'u_long' as a macro for this type (pc_prelude.h),
   so it never clashes with glibc's 32-bit u_long typedef. */
typedef uint64_t       ps2_u_long;
/* 128-bit quadword. Only used through pointers/casts and explicitly aligned arrays, so the
   type itself is 4-aligned: copies through possibly misaligned pointers (MWCC kept stack
   buffers 16-aligned, GCC does not) must not compile to aligned SSE moves. */
typedef struct { uint32_t w[4]; } u_long128;
typedef u_long128 Ps2Int128;     /* replaces __int128 (not available on i686) */

/* ---- libvu0.h ---- */
typedef float sceVu0FVECTOR[4] __attribute__((aligned(16)));
typedef float sceVu0FMATRIX[4][4] __attribute__((aligned(16)));
typedef int   sceVu0IVECTOR[4] __attribute__((aligned(16)));
typedef int   sceVu0IMATRIX[4][4] __attribute__((aligned(16)));

void sceVu0UnitMatrix(sceVu0FMATRIX m);
void sceVu0AddVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
void sceVu0ScaleVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s);
float sceVu0InnerProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1);
void sceVu0OuterProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2);
void sceVu0Normalize(sceVu0FVECTOR v0, sceVu0FVECTOR v1);
void sceVu0FTOI0Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1);
void sceVu0FTOI4Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1);
void sceVu0ITOF4Vector(sceVu0FVECTOR v0, sceVu0IVECTOR v1);
void sceVpu0Reset(void);

/* ---- libgraph.h ---- */
typedef u_long128 long128;
typedef struct { ps2_u_long NLOOP:15, EOP:1, pad16:16, id:14, PRE:1, PRIM:11, FLG:2, NREG:4; ps2_u_long REGS0:4, REGS1:4, REGS2:4, REGS3:4, REGS4:4, REGS5:4, REGS6:4, REGS7:4, REGS8:4, REGS9:4, REGS10:4, REGS11:4, REGS12:4, REGS13:4, REGS14:4, REGS15:4; } sceGifTag;
typedef struct { ps2_u_long TBP0:14, TBW:6, PSM:6, TW:4, TH:4, TCC:1, TFX:2, CBP:14, CPSM:4, CSM:1, CSA:5, CLD:3; } sceGsTex0;
typedef struct { ps2_u_long LCM:1, pad01:1, MXL:3, MMAG:1, MMIN:3, MTBA:1, pad10:9, L:2, pad21:11, K:12, pad44:20; } sceGsTex1;
typedef struct { ps2_u_long EN1:1, EN2:1, CRTMD:3, MMOD:1, AMOD:1, SLBG:1, ALP:8, p0:16, p1:32; } sceGsPmode;
typedef struct { ps2_u_long INT:1, FFMD:1, DPMS:2, p0:60; } sceGsSmode2;
typedef struct { ps2_u_long FBP:9, FBW:6, PSM:5, p0:12, DBX:11, DBY:11, p1:10; } sceGsDispfb;
typedef struct { ps2_u_long DX:12, DY:11, MAGH:4, MAGV:2, p0:3, DW:12, DH:11, p1:9; } sceGsDisplay;
typedef struct { ps2_u_long R:8, G:8, B:8, p0:40; } sceGsBgcolor;
typedef struct { ps2_u_long FBP:9, p0:7, FBW:6, p1:2, PSM:6, p2:2, FBMSK:32; } sceGsFrame;
typedef struct { ps2_u_long ZBP:9, p0:15, PSM:4, p1:4, ZMSK:1, p2:31; } sceGsZbuf;
typedef struct { ps2_u_long OFX:16, p0:16, OFY:16, p1:16; } sceGsXyoffset;
typedef struct { ps2_u_long SCAX0:11, p0:5, SCAX1:11, p1:5, SCAY0:11, p2:5, SCAY1:11, p3:5; } sceGsScissor;
typedef struct { ps2_u_long AC:1, p0:63; } sceGsPrmodecont;
typedef struct { ps2_u_long CLAMP:1, p0:63; } sceGsColclamp;
typedef struct { ps2_u_long DTHE:1, p0:63; } sceGsDthe;
typedef struct { ps2_u_long ATE:1, ATST:3, AREF:8, AFAIL:2, DATE:1, DATM:1, ZTE:1, ZTST:2, p0:45; } sceGsTest;
typedef struct { ps2_u_long SBP:14, p0:2, SBW:6, p1:2, SPSM:6, p2:2, DBP:14, p3:2, DBW:6, p4:2, DPSM:6, p5:2; } sceGsBitbltbuf;
typedef struct { ps2_u_long SSAX:11, p0:5, SSAY:11, p1:5, DSAX:11, p2:5, DSAY:11, DIR:2, p3:3; } sceGsTrxpos;
typedef struct { ps2_u_long RRW:12, p0:20, RRH:12, p1:20; } sceGsTrxreg;
typedef struct { ps2_u_long XDR:2, p0:62; } sceGsTrxdir;
typedef struct { ps2_u_long SBP:14, p0:2, SBW:6, p1:2, SPSM:6, p2:2, DBP:14, p3:2, DBW:6, p4:2, DPSM:6, p5:2; } sceGsBitbltbufReg;
typedef struct { sceGsPmode pmode; sceGsSmode2 smode2; sceGsDispfb dispfb; sceGsDisplay display; sceGsBgcolor bgcolor; } sceGsDispEnv;
typedef struct {
    sceGsFrame frame1; ps2_u_long frame1addr; sceGsZbuf zbuf1; ps2_u_long zbuf1addr;
    sceGsXyoffset xyoffset1; ps2_u_long xyoffset1addr; sceGsScissor scissor1; ps2_u_long scissor1addr;
    sceGsPrmodecont prmodecont; ps2_u_long prmodecontaddr; sceGsColclamp colclamp; ps2_u_long colclampaddr;
    sceGsDthe dthe; ps2_u_long dtheaddr; sceGsTest test1; ps2_u_long test1addr;
} __attribute__((aligned(16))) sceGsDrawEnv1;
typedef struct {
    sceGsFrame frame2; ps2_u_long frame2addr; sceGsZbuf zbuf2; ps2_u_long zbuf2addr;
    sceGsXyoffset xyoffset2; ps2_u_long xyoffset2addr; sceGsScissor scissor2; ps2_u_long scissor2addr;
    sceGsPrmodecont prmodecont; ps2_u_long prmodecontaddr; sceGsColclamp colclamp; ps2_u_long colclampaddr;
    sceGsDthe dthe; ps2_u_long dtheaddr; sceGsTest test2; ps2_u_long test2addr;
} __attribute__((aligned(16))) sceGsDrawEnv2;
typedef struct {
    ps2_u_long testa, testaaddr, prim, primaddr, rgbaq, rgbaqaddr, xyz2a, xyz2aaddr, xyz2b, xyz2baddr, testb, testbaddr;
} __attribute__((aligned(16))) sceGsClear;
typedef struct {
    sceGsDispEnv disp[2];
    sceGifTag giftag0; sceGsDrawEnv1 draw01; sceGsDrawEnv2 draw02; sceGsClear clear0;
    sceGifTag giftag1; sceGsDrawEnv1 draw11; sceGsDrawEnv2 draw12; sceGsClear clear1;
} __attribute__((aligned(16))) sceGsDBuffDc;
typedef struct {
    sceGifTag giftag0;
    sceGsBitbltbuf bitbltbuf; ps2_u_long bitbltbufaddr; sceGsTrxpos trxpos; ps2_u_long trxposaddr;
    sceGsTrxreg trxreg; ps2_u_long trxregaddr; sceGsTrxdir trxdir; ps2_u_long trxdiraddr;
    sceGifTag giftag1;
} __attribute__((aligned(16))) sceGsLoadImage;
typedef struct {
    u_int vifcode[4];
    sceGifTag giftag;
    sceGsBitbltbuf bitbltbuf; ps2_u_long bitbltbufaddr; sceGsTrxpos trxpos; ps2_u_long trxposaddr;
    sceGsTrxreg trxreg; ps2_u_long trxregaddr; ps2_u_long finish; ps2_u_long finishaddr;
    sceGsTrxdir trxdir; ps2_u_long trxdiraddr;
} __attribute__((aligned(16))) sceGsStoreImage;
int  sceGsSyncV(int mode);
int  sceGsSyncPath(int mode, u_short timeout);
void sceGsResetPath(void);
void sceGsResetGraph(short mode, short inter, short omode, short ffmode);
void sceGsSetDefDBuffDc(sceGsDBuffDc* db, short psm, short w, short h, short ztest, short zpsm, short clear);
int  sceGsSwapDBuffDc(sceGsDBuffDc* db, int id);
void sceGsSetDefLoadImage(sceGsLoadImage* lp, short dbp, short dbw, short dpsm, short x, short y, short w, short h);
int  sceGsExecLoadImage(sceGsLoadImage* lp, u_long128* srcaddr);
void sceGsSetDefStoreImage(sceGsStoreImage* sp, short sbp, short sbw, short spsm, short x, short y, short w, short h);
int  sceGsExecStoreImage(sceGsStoreImage* sp, u_long128* dstaddr);
int  sceGsSyncVCallback(int (*func)(int));

/* ---- libdma.h ---- */
typedef struct { volatile u_int chcr, p0[3], madr, p1[3], qwc, p2[3], tadr, p3[3], as0, p4[3], as1, p5[3], p6[8], sadr, p7[3]; } sceDmaChan;

/* ---- libcdvd.h ---- */
typedef struct { u_int lsn; u_int size; char name[16]; u_char date[8]; } sceCdlFILE;
typedef struct { u_char trycount, spindlctrl, datapattern, pad; } sceCdRMode;
typedef struct { u_char stat, second, minute, hour, pad, day, month, year; } sceCdCLOCK;
#define SCECdINIT 0x00
#define SCECdEXIT 0x05
#define SCECdDVD  0x02
#define SCECdErNO 0x00
#define SCECdErEOM 0x32
#define SCECdErREAD 0x30
#define SCECdErCUD 0x14
#define SCECdErCMD 0x10
#define SCECdErOPENS 0x31
#define SCECdErNODISC 0x13
int sceCdInit(int mode);
int sceCdMmode(int media);
int sceCdSearchFile(sceCdlFILE* fp, const char* name);
int sceCdRead(u_int lbn, u_int sectors, void* buf, sceCdRMode* mode);
int sceCdSync(int mode);
int sceCdGetError(void);
int sceCdBreak(void);
int sceCdDiskReady(int mode);
int sceCdPause(void);
int sceCdReadClock(sceCdCLOCK* rtc);
int sceCdStInit(u_int bufmax, u_int bankmax, u_int iop_bufaddr);
int sceCdStStart(u_int lbn, sceCdRMode* mode);
int sceCdStRead(u_int size, u_int* buf, u_int mode, u_int* err);
int sceCdStStop(void);

/* ---- sifdev.h ---- */
#define SCE_RDONLY 0x0001
#define SCE_WRONLY 0x0002
#define SCE_RDWR   0x0003
#define SCE_CREAT  0x0200
#define SCE_TRUNC  0x0400
#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2
#define SCE_FS_EXECUTING 0x1
int sceOpen(const char* name, int flags, ...);
int sceClose(int fd);
int sceRead(int fd, void* buf, int n);
int sceWrite(int fd, const void* buf, int n);
int sceLseek(int fd, int off, int whence);
int sceIoctl(int fd, int req, void* arg);
int sceFsReset(void);

/* ---- sif.h / sifrpc.h ---- */
typedef void* (*sceSifRpcFunc)(unsigned int, void*, int);
typedef void (*sceSifEndFunc)(void*);
typedef struct { int pkt_addr; int rpc_id; int sema_id; u_int mode; } sceSifRpcData;
struct _sif_serve_data;
typedef struct {
    sceSifRpcData rpcd;
    u_int command;
    void* buff;
    void* cbuff;
    sceSifEndFunc func;
    void* para;
    struct _sif_serve_data* serve;
} sceSifClientData;
typedef struct _sif_serve_data {
    int command;
    sceSifRpcFunc func;
    void* buff;
    int size;
    sceSifRpcFunc cfunc;
    void* cbuff;
    int csize;
    sceSifClientData* client;
    void* paddr;
    u_int fno;
    void* receive;
    int rsize;
    int rmode;
    u_int rid;
    struct _sif_serve_data* link;
    struct _sif_serve_data* next;
    struct _sif_queue_data* base;
} sceSifServeData;
typedef struct _sif_queue_data { int key, active; sceSifServeData* link; sceSifServeData* start; sceSifServeData* end; struct _sif_queue_data* next; } sceSifQueueData;
typedef struct { u_int data, addr; int size, mode; } sceSifDmaData;
typedef struct { sceSifRpcData rpcd; void* src; void* dest; int size; } sceSifReceiveData;
#define SIF_RPCM_NOWAIT 0x01
#define SIF_RPCM_NOWBDC 0x02
void sceSifInitRpc(u_int mode);
int  sceSifBindRpc(sceSifClientData* cd, u_int command, u_int mode);
int  sceSifCallRpc(sceSifClientData* cd, u_int fno, u_int mode, void* send, int ssize, void* receive, int rsize, sceSifEndFunc end_func, void* end_para);
int  sceSifCheckStatRpc(sceSifRpcData* cd);
int  sceSifGetOtherData(sceSifReceiveData* rd, void* src, void* dest, int size, u_int mode);
u_int sceSifSetDma(sceSifDmaData* sdd, int len);
int  sceSifDmaStat(u_int id);
int  sceSifInitIopHeap(void);
void* sceSifAllocIopHeap(u_int size);
int  sceSifFreeIopHeap(void* addr);
int  sceSifLoadModule(const char* name, int args, const char* argp);
int  sceSifRebootIop(const char* img);
int  sceSifSyncIop(void);
void sceSifSetRpcQueue(sceSifQueueData* qd, int thread_id);
void sceSifRegisterRpc(sceSifServeData* sd, u_int command, sceSifRpcFunc func, void* buff, sceSifRpcFunc cfunc, void* cbuff, sceSifQueueData* qd);
void sceSifRpcLoop(sceSifQueueData* qd);

/* ---- libpad.h ---- */
#define scePadDmaBufferMax 16
#define scePadStateStable   6
#define scePadStateFindCTP1 7
#define InfoModeCurID 1
#define InfoModeCurExID 2
#define InfoModeCurExOffs 3
#define InfoModeIdTable 4
int scePadInit(int mode);
int scePadPortOpen(int port, int slot, u_long128* addr);
int scePadPortClose(int port, int slot);
int scePadRead(int port, int slot, unsigned char* rdata);
int scePadGetState(int port, int slot);
int scePadGetReqState(int port, int slot);
int scePadInfoMode(int port, int slot, int term, int offs);
int scePadSetMainMode(int port, int slot, int offs, int lock);
int scePadInfoPressMode(int port, int slot);
int scePadEnterPressMode(int port, int slot);
int scePadSetActDirect(int port, int slot, const unsigned char* data);
int scePadSetActAlign(int port, int slot, const unsigned char* data);

/* ---- libmc.h ---- */
typedef struct { u_char Resv2, Sec, Min, Hour, Day, Month; u_short Year; } sceMcStDateTime;
typedef struct {
    sceMcStDateTime _Create;
    sceMcStDateTime _Modify;
    u_int FileSizeByte;
    u_short AttrFile;
    u_short Reserve1;
    u_int Reserve2;
    u_int PdaAplNo;
    u_char EntryName[32];
} sceMcTblGetDir;
#define sceMcFuncNoCardInfo 1
#define sceMcFuncNoOpen 2
#define sceMcFuncNoClose 3
#define sceMcFuncNoSeek 4
#define sceMcFuncNoRead 5
#define sceMcFuncNoWrite 6
#define sceMcFuncNoMkdir 0x0b
#define sceMcFuncNoChDir 0x0c
#define sceMcFuncNoGetDir 0x0d
#define sceMcFuncNoFormat 0x10
#define sceMcFuncNoDelete 0x0f
#define sceMcExecRun 0
#define sceMcExecIdle -1
#define sceMcExecFinish 1
#define sceMcInitSucceed 0
#define sceMcIniSucceed 0
int sceMcInit(void);
int sceMcGetInfo(int port, int slot, int* type, int* free, int* format);
int sceMcOpen(int port, int slot, const char* name, int mode);
int sceMcClose(int fd);
int sceMcRead(int fd, void* buf, int size);
int sceMcWrite(int fd, const void* buf, int size);
int sceMcSeek(int fd, int off, int whence);
int sceMcMkdir(int port, int slot, const char* name);
int sceMcChdir(int port, int slot, const char* newdir, char* olddir);
int sceMcGetDir(int port, int slot, const char* name, unsigned mode, int maxent, sceMcTblGetDir* table);
int sceMcFormat(int port, int slot);
int sceMcUnformat(int port, int slot);
int sceMcDelete(int port, int slot, const char* name);
int sceMcSync(int mode, int* cmd, int* result);

/* ---- eekernel.h ---- */
struct ThreadParam { int status; void* entry; void* stack; int stackSize; void* gpReg; int initPriority, currentPriority; u_int attr, option; int waitType, waitId, wakeupCount; };
struct SemaParam { int currentCount, maxCount, initCount, numWaitThreads; u_int attr, option; };
void FlushCache(int op);
void iFlushCache(int op);
int  DIntr(void);
int  EIntr(void);
int  CreateSema(struct SemaParam* p);
int  DeleteSema(int id);
int  SignalSema(int id);
int  iSignalSema(int id);
int  WaitSema(int id);
int  PollSema(int id);
int  GetThreadId(void);
int  ChangeThreadPriority(int tid, int prio);
int  RotateThreadReadyQueue(int prio);
int  SleepThread(void);
int  WakeupThread(int tid);
int  iWakeupThread(int tid);
int  ReferThreadStatus(int tid, struct ThreadParam* p);
int  StartThread(int tid, void* arg);
int  CreateThread(struct ThreadParam* p);
int  SetAlarm(u_short time, void (*cb)(int, u_short, void*), void* arg);
void ExitHandler(void);

/* ---- opaque types of subsystems the PC layer replaces (movie/IPU) ---- */
typedef struct { int dummy[16]; } sceIpuDmaEnv;
typedef struct { int width, height, frameCount; long long pts, dts; int flags; long long pts2nd, dts2nd; int flags2nd; void* sys; } sceMpeg;
typedef struct { int type; } sceMpegCbData;
typedef struct { int type; u_char* header; u_char* data; u_int len; long long pts, dts; } sceMpegCbDataStr;
typedef struct { int type; int errMessage; } sceMpegCbDataError;
typedef struct { int type; long long pts, dts; } sceMpegCbDataTimeStamp;
typedef int (*sceMpegCallback)(sceMpeg* mp, sceMpegCbData* cbdata, void* anyData);

#define PREFETCH(x) ((void)0)

#ifdef __cplusplus
}
#endif

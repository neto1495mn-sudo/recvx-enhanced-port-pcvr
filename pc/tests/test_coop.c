/* Co-op packet checks (pc/src/net/coop_proto.c, docs/COOP.md "Security"): every event type
   with malformed fields, bad datagrams, transfers, the event ring, the join code and the room
   script checks of world events - each rejected without changing anything - then a fuzz loop of
   random and mutated datagrams. */
#include "../src/net/coop_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static const char* CODE = "ABCD2345";

/* ---------------------------------------------------------------- events */
static Event ev(int type)
{
    Event e;
    memset(&e, 0, sizeof(e));
    e.id = 1;
    e.type = type;
    switch (type) {
    case EV_ENE_HIT: { int k; for (k = 0; k < 6; k++) e.u.eh.jnt[k] = -1; e.u.eh.jnt[0] = 3; e.u.eh.dam[0] = 10; e.u.eh.wpnr_no = 3; e.u.eh.flg = 4; break; }
    case EV_PLY_HIT: e.u.ph.hp_delta = 10; e.u.ph.mode[0] = 2; break;
    case EV_BLOB: case EV_ESTATE: e.u.blob.tid = 1; e.u.blob.total = 2100; e.u.blob.off = 64; e.u.blob.len = 64; break;
    case EV_FLAGS: e.u.fl.n = 2; e.u.fl.word[0] = 3; e.u.fl.word[1] = 119; e.u.fl.set[0] = 1; break;
    case EV_PICKUP: e.u.pk.item = 12; e.u.pk.bit = 146; break;
    case EV_LOST: e.u.lost.item = 82; break;
    case EV_GRAB: case EV_UNGRAB: e.idx = 1; e.u.gr.child = 255; e.u.gr.state = 1; e.u.gr.mdl = 0; e.u.gr.cmdl = -1; e.u.gr.mnw = 400; e.u.gr.cmnw = MNW_KEEP; e.u.gr.tid = 3; break;
    case EV_TRIG: e.u.tr.set = 1; e.u.tr.bit = 110; e.u.tr.n = 20; e.u.tr.last = 1; e.u.tr.start = 0x290; break;
    case EV_TRADE: e.u.td.op = TD_OFFER; e.u.td.tid = 7; e.u.td.entry = 12 << 16 | 15; break;
    case EV_BOX: case EV_BOXFIX: e.u.bx.nrm = 1; e.u.bx.nadd = 1; e.u.bx.w[0] = 5 << 16 | 30; e.u.bx.w[1] = 0x1006 << 16 | 6; break;
    case EV_SPAWN: e.idx = 40; e.u.sp.room = 1040; e.u.sp.px = 1; break;
    case EV_EOPTS: e.u.eo.extra = 2; e.u.eo.seed = 123; break;
    case EV_ATR: e.u.at.kind = ATR_WAL; e.u.at.n = 2; e.u.at.ix[0] = 8; e.u.at.ix[1] = 46; e.u.at.r[1].type = 1; break;
    case EV_GRANT: e.stg = 0; e.rom = 3; e.u.gt.item = 9; e.u.gt.count = 12; e.u.gt.p[0] = 22; e.u.gt.p[2] = -93.5f; break;
    case EV_TSCREEN: e.u.ts.op = TS_STATE; e.u.ts.slots = 8; e.u.ts.cell = 2; e.u.ts.pcell = TSC_NONE; e.u.ts.eq = 0; e.u.ts.sid = 9;
                     e.u.ts.give = 12 << 16 | 15; e.u.ts.inv[0] = 0x1005 << 16 | 6; e.u.ts.inv[2] = 12 << 16 | 15; break;
    case EV_OWN: e.u.blob.total = COOP_OWN_SIZE; e.u.blob.off = 1536; e.u.blob.len = 28; e.u.blob.pad = 1; break;
    case EV_GONE: e.u.gn.n = 2; e.u.gn.slot[0] = 3; e.u.gn.id[0] = 1; e.u.gn.slot[1] = 30; e.u.gn.id[1] = 1; break;
    case EV_SKIP: e.stg = 0; e.rom = 2; e.u.sk.op = SKIP_GO; break;
    case EV_FX: e.idx = FX_NONE; e.u.fx.id = 350; e.u.fx.owner = 4; e.u.fx.txp = 0; e.u.fx.mlw = -1; e.u.fx.exp0 = FX_NONE; e.u.fx.s[0] = 896; break;
    }
    return e;
}
static int host_sends(int t) { return t != EV_UNGRAB && t != EV_GONE && t != EV_OWN; }
static int guest_sends(int t) { return t != EV_PLY_HIT && t != EV_BLOB && t != EV_GRAB && t != EV_BOXFIX && t != EV_SPAWN && t != EV_EOPTS && t != EV_SKIP; }

/* a malformed variant of a valid event must be rejected */
#define BAD(type, from_host, stmt) do { Event e_ = ev(type); Event* e = &e_; stmt; CHECK(coop_check_event(e, from_host) != COOP_OK); } while (0)

static void test_events(void)
{
    int t;
    for (t = 1; t < EV_TYPES; t++) {   /* every type: the sample passes from the side that sends it, not from the other */
        Event e = ev(t);
        if (host_sends(t)) CHECK(coop_check_event(&e, 1) == COOP_OK);
        if (guest_sends(t)) CHECK(coop_check_event(&e, 0) == COOP_OK);
        if (!host_sends(t)) CHECK(coop_check_event(&e, 1) == COOP_BAD_DIR);
        if (!guest_sends(t)) CHECK(coop_check_event(&e, 0) == COOP_BAD_DIR);
    }
    BAD(0, 1, (void)0);
    BAD(EV_TYPES, 1, (void)0);
    BAD(200, 0, (void)0);
    /* B2: hit joints 0..63 (dam[64]) or the absent sentinel, damage bounded, only the hit bits */
    BAD(EV_ENE_HIT, 0, e->u.eh.jnt[0] = 64);
    BAD(EV_ENE_HIT, 0, e->u.eh.jnt[2] = 127);
    BAD(EV_ENE_HIT, 0, e->u.eh.jnt[5] = -2);
    BAD(EV_ENE_HIT, 0, e->u.eh.dam[0] = -5);
    BAD(EV_ENE_HIT, 0, e->u.eh.dam[0] = 1 << 30);
    BAD(EV_ENE_HIT, 0, e->u.eh.dam[1] = 5);              /* damage on an absent joint */
    BAD(EV_ENE_HIT, 0, e->u.eh.flg = 0x80);
    BAD(EV_ENE_HIT, 0, e->u.eh.flg = 4 | COOP_HIT_AIM | 0x100);   /* the aim height is at_flg 0xE << 8, nothing else */
    BAD(EV_ENE_HIT, 0, e->u.eh.flg = 4 | 0x2000);
    { Event e = ev(EV_ENE_HIT); e.u.eh.flg = 4 | COOP_HIT_AIM | 0xE00; CHECK(coop_check_event(&e, 0) == COOP_OK); }
    BAD(EV_ENE_HIT, 0, e->u.eh.comb_flg = 0x81);
    BAD(EV_ENE_HIT, 0, e->u.eh.wpnr_no = 23);
    BAD(EV_ENE_HIT, 0, e->u.eh.djnt_no = 64);
    BAD(EV_ENE_HIT, 0, e->idx = 128);
    BAD(EV_ENE_HIT, 0, e->u.eh.dv[1] = NAN);
    BAD(EV_ENE_HIT, 0, e->u.eh.dp[0] = INFINITY);
    BAD(EV_PLY_HIT, 1, e->u.ph.hp_delta = -1);
    BAD(EV_PLY_HIT, 1, e->u.ph.hp_delta = 5000);
    BAD(EV_PLY_HIT, 1, e->u.ph.dv[2] = NAN);
    /* B6: chunk lengths and offsets */
    BAD(EV_BLOB, 1, e->u.blob.len = 65);
    BAD(EV_BLOB, 1, e->u.blob.len = 0);
    BAD(EV_BLOB, 1, e->u.blob.off = 30);
    BAD(EV_BLOB, 1, e->u.blob.off = 2100);
    BAD(EV_BLOB, 1, (e->u.blob.off = 2048, e->u.blob.len = 64));   /* past the end */
    BAD(EV_BLOB, 1, e->u.blob.len = 10);                          /* short, not the last chunk */
    BAD(EV_BLOB, 1, e->u.blob.total = 0);
    BAD(EV_ESTATE, 0, e->u.blob.len = 200);
    /* B1: flag words */
    BAD(EV_FLAGS, 0, e->u.fl.n = 0);
    BAD(EV_FLAGS, 0, e->u.fl.n = 7);
    BAD(EV_FLAGS, 0, e->u.fl.n = 255);
    BAD(EV_FLAGS, 0, e->u.fl.word[1] = 120);
    BAD(EV_FLAGS, 1, e->u.fl.word[0] = 255);
    BAD(EV_PICKUP, 0, e->u.pk.item = 156);
    BAD(EV_PICKUP, 0, e->u.pk.bit = 512);
    BAD(EV_PICKUP, 0, e->u.pk.bit = -2);
    BAD(EV_PICKUP, 0, e->u.pk.undo = 1);                  /* only the host settles a spot both took */
    BAD(EV_PICKUP, 1, e->u.pk.undo = 2);
    BAD(EV_PICKUP, 1, (e->u.pk.undo = 1, e->u.pk.bit = -1));
    { Event u = ev(EV_PICKUP); u.u.pk.undo = 1; u.u.pk.item = 0; CHECK(coop_check_event(&u, 1) == COOP_OK); }
    BAD(EV_LOST, 0, e->u.lost.item = 0);
    BAD(EV_LOST, 0, e->u.lost.item = 200);
    /* B3: grabs */
    BAD(EV_GRAB, 1, e->idx = 128);
    BAD(EV_GRAB, 1, e->u.gr.child = 200);
    BAD(EV_GRAB, 1, e->u.gr.mdl = 16);
    BAD(EV_GRAB, 1, e->u.gr.cmdl = -2);
    BAD(EV_GRAB, 1, e->u.gr.mnw = 512);
    BAD(EV_GRAB, 1, e->u.gr.mnw = -100);
    BAD(EV_GRAB, 1, e->u.gr.state = 2);
    BAD(EV_UNGRAB, 0, e->u.gr.cmnw = 0x7000);
    /* B5: world events */
    BAD(EV_TRIG, 0, e->u.tr.set = 4);                 /* rm_flg: no world flag */
    BAD(EV_TRIG, 0, e->u.tr.set = 0);
    BAD(EV_TRIG, 0, e->u.tr.bit = 1024);
    BAD(EV_TRIG, 0, (e->u.tr.set = 8, e->u.tr.bit = 256));
    BAD(EV_TRIG, 0, (e->u.tr.set = 2, e->u.tr.bit = 600));
    BAD(EV_TRIG, 0, e->u.tr.val = 2);
    BAD(EV_TRIG, 0, e->u.tr.n = 0);
    BAD(EV_TRIG, 0, e->u.tr.n = COOP_TRIG_OPS + 1);
    BAD(EV_TRIG, 0, e->u.tr.first = 5);
    BAD(EV_TRIG, 0, (e->u.tr.first = 24, e->u.tr.n = 20));
    BAD(EV_TRIG, 0, e->u.tr.last = 0);
    /* B9: item words */
    BAD(EV_TRADE, 0, e->u.td.op = 0);
    BAD(EV_TRADE, 0, e->u.td.op = 5);
    BAD(EV_TRADE, 0, e->u.td.yes = 3);
    BAD(EV_TRADE, 0, e->u.td.entry = 0);
    BAD(EV_TRADE, 0, e->u.td.entry = 156 << 16 | 1);
    BAD(EV_TRADE, 0, e->u.td.entry = 12 << 16 | 1000);
    /* EV_OWN (protocol 16): chunks of the guest's COOP_OWN_SIZE items, pad 0/1; EV_BLOB's pad 0/1, EV_ESTATE's 0 */
    BAD(EV_OWN, 0, e->u.blob.total = COOP_OWN_SIZE + 64);
    BAD(EV_OWN, 0, e->u.blob.total = 2100);
    BAD(EV_OWN, 0, e->u.blob.pad = 2);
    BAD(EV_OWN, 0, e->u.blob.off = 1600);
    BAD(EV_OWN, 0, e->u.blob.off = 1500);
    BAD(EV_OWN, 0, (e->u.blob.off = 64, e->u.blob.len = 28));   /* short, not the last chunk */
    BAD(EV_BLOB, 1, e->u.blob.pad = 2);
    BAD(EV_ESTATE, 1, e->u.blob.pad = 1);
    { Event e = ev(EV_BLOB); e.u.blob.pad = 1; CHECK(coop_check_event(&e, 1) == COOP_OK); }
    /* EV_SKIP (protocol 19): the host's "skip now", SKIP_GO only, in a room of the save format */
    BAD(EV_SKIP, 1, e->u.sk.op = 0);
    BAD(EV_SKIP, 1, e->u.sk.op = SKIP_GO + 1);
    BAD(EV_SKIP, 1, e->stg = 10);
    BAD(EV_SKIP, 1, e->rom = 100);
    BAD(EV_SKIP, 0, (void)0);                            /* the guest never decides */
    /* EV_GONE: 1..COOP_GONE_MAX enemy slots below 128, a room case */
    BAD(EV_GONE, 0, e->u.gn.n = 0);
    BAD(EV_GONE, 0, e->u.gn.n = COOP_GONE_MAX + 1);
    BAD(EV_GONE, 0, e->u.gn.slot[1] = 128);
    BAD(EV_GONE, 0, e->idx = 10);
    /* EV_TSCREEN: ops, cells 0..9, grid sizes 8/10, item words; the host commits (SWAP, QUERY), the guest answers (DONE) */
    BAD(EV_TSCREEN, 0, e->u.ts.op = 0);
    BAD(EV_TSCREEN, 1, e->u.ts.op = TS_DECLINE + 1);
    BAD(EV_TSCREEN, 0, e->u.ts.op = TS_SWAP);
    BAD(EV_TSCREEN, 0, e->u.ts.op = TS_QUERY);
    BAD(EV_TSCREEN, 1, e->u.ts.op = TS_DONE);
    BAD(EV_TSCREEN, 0, e->u.ts.cell = 10);
    BAD(EV_TSCREEN, 1, e->u.ts.pcell = 200);
    BAD(EV_TSCREEN, 0, e->u.ts.eq = 12);
    BAD(EV_TSCREEN, 0, e->u.ts.slots = 9);
    BAD(EV_TSCREEN, 1, e->u.ts.ready = 2);
    BAD(EV_TSCREEN, 0, e->u.ts.yes = 2);
    BAD(EV_TSCREEN, 0, e->u.ts.why = TS_WHY_MAX);
    BAD(EV_TSCREEN, 1, e->u.ts.give = 156 << 16 | 1);
    BAD(EV_TSCREEN, 0, e->u.ts.take = 5 << 16 | 1000);
    BAD(EV_TSCREEN, 1, e->u.ts.inv[9] = 0xFF0001);
    {   /* the other ops pass from their side; an empty word (0) is no item */
        Event e = ev(EV_TSCREEN);
        e.u.ts.op = TS_SWAP; e.u.ts.slots = 0; e.u.ts.take = 0; CHECK(coop_check_event(&e, 1) == COOP_OK);
        e.u.ts.op = TS_DONE; e.u.ts.why = 3; CHECK(coop_check_event(&e, 0) == COOP_OK);
        e.u.ts.op = TS_REQ; CHECK(coop_check_event(&e, 0) == COOP_OK && coop_check_event(&e, 1) == COOP_OK);
        /* protocol 17: an item request (either side) names a cell and its item, or withdraws with neither; a decline names both */
        e = ev(EV_TSCREEN);
        e.u.ts.op = TS_WANT; e.u.ts.cell = 3; e.u.ts.take = 0x170001; CHECK(coop_check_event(&e, 0) == COOP_OK && coop_check_event(&e, 1) == COOP_OK);
        e.u.ts.op = TS_DECLINE; CHECK(coop_check_event(&e, 0) == COOP_OK && coop_check_event(&e, 1) == COOP_OK);
        e.u.ts.op = TS_WANT; e.u.ts.cell = TSC_NONE; e.u.ts.take = 0; CHECK(coop_check_event(&e, 1) == COOP_OK);
        e.u.ts.take = 0x170001; CHECK(coop_check_event(&e, 1) == COOP_BAD_FIELD);                     /* withdrawn, but an item */
        e.u.ts.cell = 3; e.u.ts.take = 0; CHECK(coop_check_event(&e, 0) == COOP_BAD_FIELD);          /* a cell, no item */
        e.u.ts.cell = 10; e.u.ts.take = 0x170001; CHECK(coop_check_event(&e, 0) == COOP_BAD_FIELD);  /* no such cell */
        e.u.ts.cell = 3; e.u.ts.take = 156 << 16 | 1; CHECK(coop_check_event(&e, 1) == COOP_BAD_FIELD); /* no such item */
        e.u.ts.take = 0x17 << 16 | 1000; CHECK(coop_check_event(&e, 1) == COOP_BAD_FIELD);          /* count */
        e.u.ts.op = TS_DECLINE; e.u.ts.cell = TSC_NONE; e.u.ts.take = 0x170001; CHECK(coop_check_event(&e, 0) == COOP_BAD_FIELD);
        e.u.ts.cell = 2; e.u.ts.take = 0; CHECK(coop_check_event(&e, 1) == COOP_BAD_FIELD);
    }
    BAD(EV_BOX, 0, e->u.bx.nrm = 0 * (e->u.bx.nadd = 0));
    BAD(EV_BOX, 0, e->u.bx.nadd = 16);
    BAD(EV_BOX, 0, e->u.bx.w[0] = 0);
    BAD(EV_BOX, 0, e->u.bx.w[1] = 0xFF0001);
    BAD(EV_BOX, 1, e->u.bx.w[0] = 5 << 16 | 0xFFFF);
    BAD(EV_SPAWN, 1, e->idx = 128);
    BAD(EV_SPAWN, 1, e->u.sp.pz = NAN);
    BAD(EV_EOPTS, 1, e->u.eo.extra = 4);
    BAD(EV_EOPTS, 1, e->u.eo.seed = -1);
    BAD(EV_EOPTS, 1, e->u.eo.rand = 2);
    /* EV_FX: an effect number the game has, references to an enemy slot or FX_*, finite floats; bursts bounded */
    /* EV_GRANT: an item the game has, a floor, a count, a room, a position the save region can hold */
    BAD(EV_GRANT, 0, e->u.gt.item = 0);
    BAD(EV_GRANT, 1, e->u.gt.item = 156);
    BAD(EV_GRANT, 0, e->u.gt.flr_no = -1);
    BAD(EV_GRANT, 0, e->u.gt.flr_no = 64);
    BAD(EV_GRANT, 1, e->u.gt.count = 1000);
    BAD(EV_GRANT, 0, e->stg = 10);
    BAD(EV_GRANT, 0, e->rom = 100);
    BAD(EV_GRANT, 1, e->u.gt.p[1] = NAN);
    BAD(EV_GRANT, 0, e->u.gt.p[2] = 321);
    BAD(EV_GRANT, 0, e->u.gt.p[0] = -400);
    BAD(EV_FX, 0, e->u.fx.id = 400);
    BAD(EV_FX, 1, e->idx = 128);
    BAD(EV_FX, 1, e->u.fx.owner = 252);
    BAD(EV_FX, 1, e->u.fx.exp0 = 200);
    BAD(EV_FX, 0, e->u.fx.lkono = 64);
    BAD(EV_FX, 0, e->u.fx.txp = 16);
    BAD(EV_FX, 0, e->u.fx.mlw = -2);
    BAD(EV_FX, 0, e->u.fx.p[1] = NAN);
    BAD(EV_FX, 0, e->u.fx.o[2] = INFINITY);
    BAD(EV_FX, 0, e->u.fx.n[0] = NAN);
    {   /* a particle burst */
        Event e = ev(EV_FX);
        float pos[3] = { 1, 2, 3 };
        memset(&e.u, 0, sizeof(e.u));
        e.u.fxr.id = FX_BURST; e.u.fxr.nw = FX_RAW_WORDS; e.u.fxr.w[0] = 12; e.u.fxr.w[4] = 9; memcpy(&e.u.fxr.w[9], pos, sizeof(pos));
        CHECK(coop_check_event(&e, 0) == COOP_OK && coop_check_event(&e, 1) == COOP_OK);
        e.u.fxr.w[0] = 33; CHECK(coop_check_event(&e, 0) != COOP_OK); e.u.fxr.w[0] = 12;
        e.u.fxr.w[4] = 0xFFFFFFFF; CHECK(coop_check_event(&e, 0) != COOP_OK); e.u.fxr.w[4] = 9;
        e.u.fxr.nw = 17; CHECK(coop_check_event(&e, 0) != COOP_OK); e.u.fxr.nw = FX_RAW_WORDS;
        e.idx = 200; CHECK(coop_check_event(&e, 0) != COOP_OK); e.idx = FX_NONE;
        { float f = NAN; memcpy(&e.u.fxr.w[13], &f, 4); } CHECK(coop_check_event(&e, 0) != COOP_OK);
    }
    {   /* a Bandersnatch's arm */
        Event e = ev(EV_FX);
        memset(&e.u, 0, sizeof(e.u));
        e.idx = 3; e.u.fxa.id = FX_ARM; e.u.fxa.len[3] = 12.5f;
        CHECK(coop_check_event(&e, 1) == COOP_OK);
        e.idx = FX_NONE; CHECK(coop_check_event(&e, 1) != COOP_OK); e.idx = 3;
        e.u.fxa.len[1] = NAN; CHECK(coop_check_event(&e, 1) != COOP_OK);
    }
    {   /* a particle call */
        Event e = ev(EV_FX);
        memset(&e.u, 0, sizeof(e.u));
        e.idx = 3; e.u.fxp.id = FX_PARTICLE; e.u.fxp.typ = 5; e.u.fxp.lnk = 9; e.u.fxp.dir[2] = 1;
        CHECK(coop_check_event(&e, 1) == COOP_OK);
        e.u.fxp.typ = FX_PTCL_TYPES; CHECK(coop_check_event(&e, 1) != COOP_OK); e.u.fxp.typ = 5;
        e.u.fxp.lnk = 64; CHECK(coop_check_event(&e, 1) != COOP_OK); e.u.fxp.lnk = 9;
        e.u.fxp.mk2 = 2; CHECK(coop_check_event(&e, 1) != COOP_OK); e.u.fxp.mk2 = 1;
        e.u.fxp.dir[0] = NAN; CHECK(coop_check_event(&e, 1) != COOP_OK);
    }
    BAD(EV_ATR, 0, e->u.at.kind = 4);
    BAD(EV_ATR, 0, e->u.at.n = 0);
    BAD(EV_ATR, 0, e->u.at.n = 3);
    BAD(EV_ATR, 0, e->u.at.r[0].p[3] = NAN);
    BAD(EV_ATR, 0, e->u.at.r[1].type = 9);
    BAD(EV_ATR, 0, (e->u.at.kind = ATR_OBJ, e->u.at.ix[0] = 3));    /* objects 0..3 are not synced */
    BAD(EV_ATR, 0, (e->u.at.kind = ATR_OBJ, e->u.at.ix[1] = 32));   /* sys->obwp has 32 */
    BAD(EV_ATR, 0, (e->u.at.kind = ATR_OBJ, e->u.at.r[0].p[4] = 3e9f));   /* an angle no int holds */
    {   /* objects 4..31 with int angles pass */
        Event e = ev(EV_ATR);
        e.u.at.kind = ATR_OBJ; e.u.at.ix[0] = 4; e.u.at.ix[1] = 31; e.u.at.r[0].p[3] = -65536; e.u.at.r[1].p[5] = 6553600;
        CHECK(coop_check_event(&e, 0) == COOP_OK);
    }
}

static void test_snaps(void)
{
    EneSnap s;
    memset(&s, 0, sizeof(s));
    s.idx = 5; s.mdl = 0; s.mnw = 12; s.mtn_no = 40;
    CHECK(coop_check_snap(&s) == COOP_OK);
    s.mnw = MNW_ROOM; CHECK(coop_check_snap(&s) == COOP_OK);
    s.mnw = MNW_KEEP; s.mdl = -1; CHECK(coop_check_snap(&s) == COOP_OK);
#define BADS(stmt) do { EneSnap t = s; stmt; CHECK(coop_check_snap(&t) != COOP_OK); } while (0)
    BADS(t.idx = 128);
    BADS(t.mdl = 16);          /* B4: the model index into mdl[16] */
    BADS(t.mdl = -2);
    BADS(t.mdl = 127);
    BADS(t.mnw = 512);         /* B4: motion sets only as offsets 0..511 */
    BADS(t.mnw = -1);
    BADS(t.mnw = 0x7000);
    BADS(t.mtn_no = 512);
    BADS(t.mtn_no = -1);
    BADS(t.frm_no = -1);
    BADS(t.px = NAN);
    BADS(t.watr[6] = INFINITY);
}

/* ---------------------------------------------------------------- datagrams */
static int dgram(unsigned char* buf, int nev, int nene, int ext, uint32_t sid, const char* code)
{
    Pose p;
    int n = sizeof(Pose), k;
    memset(&p, 0, sizeof(p));
    p.magic = COOP_MAGIC; p.version = COOP_VERSION; p.type = PKT_POSE; p.sid = sid; p.auth = coop_auth(code, sid);
    p.nev = nev; p.nene = nene; p.flags = ext ? PF_EXT : 0; p.mset = MSET_OWN;
    memcpy(buf, &p, sizeof(p));
    for (k = 0; k < nev; k++) { Event e = ev(EV_FLAGS); e.id = k + 1; memcpy(buf + n, &e, sizeof(e)); n += sizeof(e); }
    for (k = 0; k < nene; k++) { EneSnap s; memset(&s, 0, sizeof(s)); s.idx = k; s.mdl = -1; s.mnw = MNW_KEEP; memcpy(buf + n, &s, sizeof(s)); n += sizeof(s); }
    if (ext) { memset(buf + n, 0, sizeof(PoseExt)); n += sizeof(PoseExt); }
    return n;
}

static void test_dgrams(void)
{
    static unsigned char b[4096];
    int n, nev, nene, ext;
    Pose p;
    n = dgram(b, 3, 4, 1, 77, CODE);
    CHECK(n <= COOP_DGRAM_MAX);
    CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == COOP_OK && nev == 3 && nene == 4 && ext == 1);
    CHECK(coop_check_all(b, n, "ABCD2346", 1, &nev, &nene, &ext) == COOP_BAD_AUTH);   /* B12: wrong join code */
    CHECK(coop_check_all(b, n, "", 1, &nev, &nene, &ext) == COOP_BAD_AUTH);
    CHECK(coop_check_all(b, n - 1, CODE, 1, &nev, &nene, &ext) == COOP_BAD_SIZE);
    CHECK(coop_check_all(b, n + 1, CODE, 1, &nev, &nene, &ext) == COOP_BAD_SIZE);
    CHECK(coop_check_all(b, 10, CODE, 1, &nev, &nene, &ext) == COOP_BAD_SIZE);
    memcpy(&p, b, sizeof(p));
#define BADP(stmt, want) do { Pose q = p; stmt; memcpy(b, &q, sizeof(q)); CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == want); memcpy(b, &p, sizeof(p)); } while (0)
    BADP(q.magic = 1, COOP_BAD_HDR);
    BADP(q.version = 10, COOP_BAD_VERSION);
    BADP(q.type = 9, COOP_BAD_TYPE);
    BADP(q.sid = 78, COOP_BAD_AUTH);              /* the proof belongs to its session */
    BADP(q.nev = 17, COOP_BAD_COUNT);             /* oversized counts */
    BADP(q.nene = 49, COOP_BAD_COUNT);
    BADP(q.nev = 255, COOP_BAD_COUNT);
    BADP(q.nev = 2, COOP_BAD_SIZE);               /* counts that do not match the size */
    BADP(q.flags = 0, COOP_BAD_SIZE);             /* no PoseExt flagged, one there */
    BADP(q.px = NAN, COOP_BAD_FLOAT);
    BADP(q.mset = 300, COOP_BAD_FIELD);
    BADP(q.busy = 7, COOP_BAD_FIELD);
    BADP(q.flr = 33, COOP_BAD_FIELD);             /* flr_no 30: past rom->grand[32] */
    BADP(q.flr = 255, COOP_BAD_FIELD);
    BADP(q.flr = 32, COOP_OK);                    /* floors -2..29, 0 = not sent */
    BADP(q.flr = 1, COOP_OK);
    BADP(q.flags |= PF_CINE | PF_SKIP, COOP_OK);   /* protocol 19: in a cutscene, wants to skip it */
    BADP(q.flags |= 128, COOP_BAD_FIELD);         /* a flag this protocol does not have */
    BADP(q.opts = OPT_ALL, COOP_OK);              /* every game rule */
    BADP(q.opts = OPT_ALL + 1, COOP_BAD_FIELD);
    BADP(q.opts = 0x80, COOP_BAD_FIELD);
    {   /* the PoseExt's enumerations (protocol 16) */
        PoseExt x;
        memset(&x, 0, sizeof(x));
        x.items = ITEMS_CURRENT; x.rec = REC_OFF;
        memcpy(b + n - sizeof(x), &x, sizeof(x));
        CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == COOP_OK);
        x.items = ITEMS_CURRENT + 1;
        memcpy(b + n - sizeof(x), &x, sizeof(x));
        CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == COOP_BAD_FIELD);
        x.items = 0; x.rec = REC_OFF + 1;
        memcpy(b + n - sizeof(x), &x, sizeof(x));
        CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == COOP_BAD_FIELD);
        memset(b + n - sizeof(x), 0, sizeof(x));
    }
    {   /* one bad event spoils the datagram */
        Event e = ev(EV_ENE_HIT);
        e.u.eh.jnt[0] = 100;
        memcpy(b + sizeof(Pose) + sizeof(Event), &e, sizeof(e));
        CHECK(coop_check_all(b, n, CODE, 0, &nev, &nene, &ext) == COOP_BAD_FIELD);
        n = dgram(b, 3, 4, 1, 77, CODE);
    }
    {   /* one bad snapshot too */
        EneSnap s;
        memcpy(&s, b + sizeof(Pose) + 3 * sizeof(Event), sizeof(s));
        s.mdl = 40;
        memcpy(b + sizeof(Pose) + 3 * sizeof(Event), &s, sizeof(s));
        CHECK(coop_check_all(b, n, CODE, 1, &nev, &nene, &ext) == COOP_BAD_FIELD);
    }
    {   /* a goodbye and a rejection are bare poses; a rejection carries no proof */
        Pose q = p;
        q.type = PKT_BYE; q.nev = q.nene = 0; q.flags = 0;
        memcpy(b, &q, sizeof(q));
        CHECK(coop_check_all(b, sizeof(Pose), CODE, 1, &nev, &nene, &ext) == COOP_OK);
        CHECK(coop_check_all(b, sizeof(Pose) + 1, CODE, 1, &nev, &nene, &ext) == COOP_BAD_SIZE);
        q.type = PKT_REJECT; q.auth = 0;
        memcpy(b, &q, sizeof(q));
        CHECK(coop_check_all(b, sizeof(Pose), CODE, 1, &nev, &nene, &ext) == COOP_OK);
        q.type = PKT_BYE;
        memcpy(b, &q, sizeof(q));
        CHECK(coop_check_all(b, sizeof(Pose), CODE, 1, &nev, &nene, &ext) == COOP_BAD_AUTH);
    }
    /* B10: the largest frame part still fits a datagram */
    CHECK(sizeof(Pose) + sizeof(PoseExt) + sizeof(Event) <= COOP_DGRAM_MAX && sizeof(Pose) + sizeof(EneSnap) <= COOP_DGRAM_MAX);
}

/* ---------------------------------------------------------------- transfers */
static void test_xfer(void)
{
    static unsigned char buf[0x900 + 16], copy[sizeof(buf)], data[64];
    CoopXfer x, xc;
    int off, r, total = 2100;
    memset(buf, 0xEE, sizeof(buf));
    memset(data, 0x5A, sizeof(data));
    coop_xfer_init(&x, buf, 0x900);
    /* B6: a chunk ending at the total does not complete a transfer missing others */
    CHECK(coop_xfer_chunk(&x, 1, total, 2048, total - 2048, data) == 0 && !x.done);
#define XBAD(...) do { memcpy(copy, buf, sizeof(buf)); xc = x; CHECK(coop_xfer_chunk(&x, __VA_ARGS__) == -1); \
                       CHECK(!memcmp(copy, buf, sizeof(buf)) && !memcmp(&xc, &x, sizeof(x))); } while (0)
    XBAD(1, total, 0, 65, data);            /* longer than a chunk */
    XBAD(1, total, 32, 64, data);           /* not on a chunk boundary */
    XBAD(1, total, 2048, 64, data);         /* past the end */
    XBAD(1, total, 0, 10, data);            /* short, not the last */
    XBAD(1, 1000, 0, 64, data);             /* the same transfer with another size */
    XBAD(1, 0x901, 0, 64, data);            /* bigger than the buffer */
    XBAD(0, total, 0, 64, data);            /* no transfer id */
    XBAD(1, total, -64, 64, data);
    CHECK(buf[0x900] == 0xEE);              /* nothing beyond the buffer */
    for (off = 0, r = 0; off < 2048; off += 64) r = coop_xfer_chunk(&x, 1, total, off, 64, data);
    CHECK(r == 1 && x.done);                /* every chunk: complete */
    CHECK(coop_xfer_chunk(&x, 2, total, 0, 64, data) == 0 && !x.done);   /* a new transfer starts over */
    coop_xfer_reset(&x);
    CHECK(!x.done && !x.tid);
    {   /* estate size: 4576 bytes in 72 chunks */
        static unsigned char big[4608];
        coop_xfer_init(&x, big, sizeof(big));
        for (off = 0, r = 0; off < 4576; off += 64) r = coop_xfer_chunk(&x, 9, 4576, off, 4576 - off < 64 ? 4576 - off : 64, data);
        CHECK(r == 1);
    }
}

/* ---------------------------------------------------------------- the event ring */
static void test_ring(void)
{
    static CoopRing r, c;
    Event e = ev(EV_FLAGS);
    int k, ok = 1;
    coop_ring_reset(&r);
    for (k = 0; k < COOP_RING; k++) ok &= coop_ring_push(&r, &e);
    CHECK(ok && r.next - r.head == COOP_RING && !r.overflow);
    c = r;
    CHECK(!coop_ring_push(&r, &e) && r.overflow);          /* B7: full - refused, nothing dropped */
    CHECK(r.head == c.head && r.next == c.next && !memcmp(r.ev, c.ev, sizeof(r.ev)));
    CHECK(coop_ring_get(&r, 1) && coop_ring_get(&r, 1)->id == 1);   /* the oldest is still there */
    CHECK(!coop_ring_push(&r, &e));                         /* stays refused until the resync */
    coop_ring_ack(&r, 100000);                              /* acknowledgements out of range change nothing */
    coop_ring_ack(&r, 0);
    CHECK(r.head == 1);
    coop_ring_ack(&r, 10);
    CHECK(r.head == 11 && !coop_ring_get(&r, 10) && coop_ring_get(&r, 11));
    coop_ring_reset(&r);
    CHECK(coop_ring_push(&r, &e) && e.id == 1);
}

/* ---------------------------------------------------------------- join code */
static void test_code(void)
{
    char host[64], code[COOP_CODE_LEN + 1], c2[COOP_CODE_LEN + 1];
    int port, k;
    CHECK(coop_parse_join("1.2.3.4:7150#abcd-2345", host, sizeof(host), &port, code) && !strcmp(host, "1.2.3.4") && port == 7150 && !strcmp(code, "ABCD2345"));
    CHECK(coop_parse_join("example.org:7140 ABCD 2345", host, sizeof(host), &port, code) && !strcmp(host, "example.org") && !strcmp(code, "ABCD2345"));
    CHECK(coop_parse_join("1.2.3.4:7140", host, sizeof(host), &port, code) && code[0] == 0);   /* no code: the joiner is told */
    CHECK(coop_parse_join("1.2.3.4:7140#ABCD", host, sizeof(host), &port, code) && !strcmp(code, "?"));   /* malformed */
    CHECK(coop_parse_join("1.2.3.4:7140#ABCD234O", host, sizeof(host), &port, code) && !strcmp(code, "?"));   /* O is no code letter */
    CHECK(!coop_parse_join("1.2.3.4:70000#ABCD2345", host, sizeof(host), &port, code));
    CHECK(!coop_parse_join(":7140#ABCD2345", host, sizeof(host), &port, code));
    for (k = 0; k < 100; k++) {   /* the host's code (coop_glue.c host_code): OS randomness, as room codes */
        uint8_t rnd8[COOP_CODE_LEN];
        CHECK(rdv_random(rnd8, sizeof(rnd8)));
        rdv_code_from(rnd8, code);
        CHECK(rdv_code_norm(code, c2) && !strcmp(code, c2) && strlen(code) == COOP_CODE_LEN);
        if (k) CHECK(strcmp(code, host));   /* host: the previous one */
        strcpy(host, code);
    }
    CHECK(rdv_code_norm("abcd-2345", code) && !strcmp(code, "ABCD2345") && !rdv_code_norm("ABCD234O", code) && !rdv_code_norm("ABCD23456", code));
    CHECK(coop_auth(CODE, 5) != coop_auth("ABCD2346", 5) && coop_auth(CODE, 5) != coop_auth(CODE, 6) && coop_auth(CODE, 5) == coop_auth(CODE, 5));
}

/* ---------------------------------------------------------------- EV_ATR: what of a partner's record is taken */
static void test_atr(void)
{
    /* EtcAtariSet2 7: attr 0x1234, prm 5 6 7 8, type 2; WalAtariSet2 3 and FlrAtariSet2 1 likewise */
    static const uint8_t ops[3][10] = { { 37, 7, 0x34, 0x12, 5, 6, 7, 8, 2, 0 }, { 123, 3, 1, 0, 0, 0, 0, 0, 4, 0 }, { 124, 1, 0, 0, 2, 0, 0, 0, 1, 0 } };
    AtrSet set[3];
    AtrRec mine, in, r;
    int k;
    for (k = 0; k < 3; k++) CHECK(coop_atr_set(ops[k], &set[k]));
    { uint8_t evtoff[4] = { 11, 7, 1, 0 }; AtrSet x; CHECK(!coop_atr_set(evtoff, &x)); }   /* EtcAtariSet (on/off): not a field write */
    CHECK(set[0].kind == ATR_ETC && set[0].ix == 7 && set[0].attr == 0x1234 && set[0].prm[0] == 5 && set[0].prm[3] == 8 && set[0].type == 2);
    CHECK(set[1].kind == ATR_WAL && set[1].ix == 3 && set[2].kind == ATR_FLR && set[2].ix == 1);
    memset(&mine, 0, sizeof(mine));
    mine.flg = 1; mine.type = 1; mine.id = 9; mine.flr_no = 2; mine.attr = 0x40; mine.prm[0] = 3;
    /* a partner's record with an out-of-range prm0 (itwp/obwp have 32) and everything else changed:
       only flg and the placement are taken */
    in = mine;
    in.flg = 0; in.type = 2; in.id = 200; in.flr_no = 100; in.attr = 0xFFFF; in.prm[0] = 255; in.p[0] = 12.5f; in.p[5] = 3;
    r = mine;
    coop_atr_merge(&r, &in, ATR_ETC, 7, set, 3);
    CHECK(r.flg == 0 && r.p[0] == 12.5f && r.p[5] == 3 && r.type == 1 && r.id == 9 && r.flr_no == 2 && r.attr == 0x40 && r.prm[0] == 3);
    /* what the room's own EtcAtariSet2 7 writes is taken - for that record only; id, flr_no stay */
    in.type = 2; in.attr = 0x1234; in.prm[0] = 5; in.prm[1] = 6; in.prm[2] = 7; in.prm[3] = 8;
    r = mine;
    coop_atr_merge(&r, &in, ATR_ETC, 7, set, 3);
    CHECK(r.type == 2 && r.attr == 0x1234 && r.prm[0] == 5 && r.prm[3] == 8 && r.id == 9 && r.flr_no == 2);
    r = mine;
    coop_atr_merge(&r, &in, ATR_ETC, 8, set, 3);
    CHECK(r.type == 1 && r.prm[0] == 3);
    r = mine;
    coop_atr_merge(&r, &in, ATR_WAL, 7, set, 3);
    CHECK(r.type == 1 && r.attr == 0x40);
    in.prm[3] = 9;   /* one byte off */
    r = mine;
    coop_atr_merge(&r, &in, ATR_ETC, 7, set, 3);
    CHECK(r.type == 1 && r.prm[3] == 0);
}

/* ---------------------------------------------------------------- world event actions */
static void test_trig(void)
{
    /* a then-part: Set ev 10 (6), EvtOn (4), WalAtariSet (4), SetDoorCall (8), LoadWork player (4),
       CommonCtr.12 (6), Sv etc_idx (4), End */
    static const unsigned char d[] = { 5, 1, 10, 0, 0, 0,   20, 0, 1, 6,   10, 8, 1, 0,   51, 0, 6, 0, 2, 4, 1, 30,
                                       101, 0, 0, 0,   105, 12, 1, 0, 1, 0,   8, 20, 0, 0,   0, 0 };
    const int then1 = sizeof(d) - 2;
    unsigned short ok[] = { 0, 6, 10, 14, 22, 26, 32 };
    unsigned short mid[] = { 0, 7 }, back[] = { 6, 0 }, out[] = { 0, then1 }, past[] = { 0, 200 };
    CHECK(coop_trig_ops_ok(d, sizeof(d), 0, then1, ok, 7));
    CHECK(coop_trig_ops_ok(d, sizeof(d), 0, then1, ok + 2, 3));
    CHECK(!coop_trig_ops_ok(d, sizeof(d), 0, then1, mid, 2));     /* B5: inside an instruction */
    CHECK(!coop_trig_ops_ok(d, sizeof(d), 0, then1, back, 2));    /* out of order */
    CHECK(!coop_trig_ops_ok(d, sizeof(d), 0, then1, out, 2));     /* outside the block */
    CHECK(!coop_trig_ops_ok(d, sizeof(d), 0, then1, past, 2));
    CHECK(!coop_trig_ops_ok(d, sizeof(d), 6, then1, ok, 1));      /* before the then-part */
    CHECK(!coop_trig_ops_ok(d, 10, 0, then1, ok, 2));             /* a block past the script */
    CHECK(coop_op_replay(d + 0) && coop_op_replay(d + 6) && coop_op_replay(d + 10) && coop_op_replay(d + 32));
    CHECK(!coop_op_replay(d + 14));   /* SetDoorCall: the partner's own door */
    CHECK(!coop_op_replay(d + 22));   /* LoadWork of the player */
    CHECK(coop_insn_len_raw(d + 26) == 6 && coop_insn_len_raw(d + 14) == 8);
}

/* ---------------------------------------------------------------- fuzz */
static unsigned int rng = 12345;
static unsigned int rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

/* what an accepted event may contain (independent of coop_check_event's code) */
static int sane(const Event* e)
{
    int k;
    if (e->type == 0 || e->type >= EV_TYPES) return 0;
    if (e->type == EV_ENE_HIT) for (k = 0; k < 6; k++) if (e->u.eh.jnt[k] < -1 || e->u.eh.jnt[k] > 63) return 0;
    if (e->type == EV_PICKUP && (e->u.pk.undo > 1 || e->u.pk.bit < -1 || e->u.pk.bit >= 512)) return 0;
    if (e->type == EV_FLAGS) { if (e->u.fl.n > 6) return 0; for (k = 0; k < e->u.fl.n; k++) if (e->u.fl.word[k] >= 120) return 0; }
    if (e->type == EV_BLOB || e->type == EV_ESTATE || e->type == EV_OWN) if (e->u.blob.len > 64 || e->u.blob.off + e->u.blob.len > e->u.blob.total || e->u.blob.pad > 1) return 0;
    if (e->type == EV_OWN && e->u.blob.total != COOP_OWN_SIZE) return 0;
    if (e->type == EV_TRIG && (e->u.tr.n > COOP_TRIG_OPS || e->u.tr.bit >= 1024)) return 0;
    if (e->type == EV_BOX && e->u.bx.nrm + e->u.bx.nadd > 16) return 0;
    if (e->type == EV_ATR && (e->u.at.n > 2 || (e->u.at.kind == ATR_OBJ && (e->u.at.ix[0] < 4 || e->u.at.ix[0] > 31)))) return 0;
    if (e->type == EV_GRANT && (e->u.gt.item == 0 || e->u.gt.item > 155 || e->u.gt.count > 999 || e->stg > 9 || e->rom > 99 ||
                                !(fabsf(e->u.gt.p[0]) <= 320 && fabsf(e->u.gt.p[1]) <= 320 && fabsf(e->u.gt.p[2]) <= 320))) return 0;
    if (e->type == EV_TSCREEN) {
        if (e->u.ts.op < 1 || e->u.ts.op > TS_DECLINE || (e->u.ts.cell > 9 && e->u.ts.cell != 255) || (e->u.ts.pcell > 9 && e->u.ts.pcell != 255)) return 0;
        if ((e->u.ts.op == TS_WANT || e->u.ts.op == TS_DECLINE) && (e->u.ts.cell == 255) != !e->u.ts.take) return 0;
        if (e->u.ts.op == TS_DECLINE && !e->u.ts.take) return 0;
        if (e->u.ts.take && (((e->u.ts.take >> 16) & 0xFF) == 0 || ((e->u.ts.take >> 16) & 0xFF) > 155 || (e->u.ts.take & 0xFFFF) > 999)) return 0;
        for (k = 0; k < 10; k++) if (e->u.ts.inv[k] && (((e->u.ts.inv[k] >> 16) & 0xFF) > 155 || (e->u.ts.inv[k] & 0xFFFF) > 999)) return 0;
        if (e->u.ts.give && (((e->u.ts.give >> 16) & 0xFF) == 0 || (e->u.ts.give & 0xFFFF) > 999)) return 0;
    }
    if (e->type == EV_FX && ((e->idx >= 128 && e->idx < FX_RECEIVER) || e->u.fx.id >= 400 || (e->u.fx.id != FX_BURST && e->u.fx.lkono >= 64))) return 0;
    if (e->type == EV_FX && e->u.fx.id == FX_BURST && (e->u.fxr.w[0] > 32 || e->u.fxr.w[4] > 600)) return 0;
    if (e->type == EV_FX && e->u.fx.id == FX_PARTICLE && (e->u.fxp.typ >= FX_PTCL_TYPES || e->u.fxp.lnk >= 64)) return 0;
    return 1;
}

static void fuzz(int iters)
{
    static unsigned char b[COOP_DGRAM_MAX + 64], xbuf[0x900 + 64], copy[sizeof(xbuf)];
    CoopXfer x;
    int i, accepted = 0, evs = 0;
    coop_xfer_init(&x, xbuf, 0x900);
    for (i = 0; i < iters; i++) {
        int n, nev, nene, ext, k, r;
        if (i % 3 == 0) {   /* random bytes after a valid header */
            n = sizeof(Pose) + rnd() % (COOP_DGRAM_MAX - sizeof(Pose));
            for (k = 0; k < n; k++) b[k] = rnd();
            if (i % 2) { Pose p; memcpy(&p, b, sizeof(p)); p.magic = COOP_MAGIC; p.version = COOP_VERSION; p.type = PKT_POSE; p.auth = coop_auth(CODE, p.sid); memcpy(b, &p, sizeof(p)); }
        } else {            /* a valid datagram, a few bytes or fields changed */
            int ne = rnd() % 9, ns = rnd() % 4;
            n = dgram(b, ne, ns, rnd() & 1, rnd(), CODE);
            for (k = 0; k < ne; k++) { Event e = ev(1 + rnd() % (EV_TYPES - 1)); memcpy(b + sizeof(Pose) + k * sizeof(Event), &e, sizeof(e)); }
            for (k = rnd() % 6; k > 0 && n > (int)sizeof(Pose); k--) b[sizeof(Pose) + rnd() % (n - sizeof(Pose))] = rnd();
            if (rnd() % 8 == 0) n += (int)(rnd() % 5) - 2;
        }
        r = coop_check_all(b, n, CODE, rnd() & 1, &nev, &nene, &ext);
        if (r == COOP_OK) {
            accepted++;
            for (k = 0; k < nev; k++) {
                Event e;
                memcpy(&e, b + sizeof(Pose) + k * sizeof(Event), sizeof(e));
                CHECK(sane(&e));
                evs++;
                if (e.type == EV_BLOB) {   /* accepted chunks never write past the transfer */
                    memcpy(copy, xbuf, sizeof(xbuf));
                    if (coop_xfer_chunk(&x, e.u.blob.tid, e.u.blob.total, e.u.blob.off, e.u.blob.len, e.u.blob.data) < 0) CHECK(!memcmp(copy, xbuf, sizeof(xbuf)));
                    CHECK(!memcmp(copy + 0x900, xbuf + 0x900, 64));
                }
            }
        }
        {   /* random chunks straight into a transfer */
            int tid = rnd() % 4, total = rnd() % 0xA00, off = (int)(rnd() % 0xA00) - 64, len = rnd() % 80;
            memcpy(copy, xbuf, sizeof(xbuf));
            if (coop_xfer_chunk(&x, tid, total, off, len, b) < 0) CHECK(!memcmp(copy, xbuf, sizeof(xbuf)));
            CHECK(!memcmp(copy + 0x900, xbuf + 0x900, 64));
        }
    }
    printf("fuzz: %d datagrams, %d accepted (%d events), no crash\n", iters, accepted, evs);
}

/* ---------------------------------------------------------------- enemy targets (coop_pick_target) */
static int pick(int cur, float d0, float d1, int b0, int b1, int f0, int f1, int commit)
{
    CoopTgtIn in = { { d0, d1 }, { b0, b1 }, { f0, f1 }, commit };
    return coop_pick_target(cur, &in);
}
static void test_target(void)
{
    float n = 10.0f * 10.0f, f = 13.0f * 13.0f;   /* 1 m and 1.3 m, squared */
    enum { F = COOP_TGT_FREE, A = COOP_TGT_ATTACK, HOLD = COOP_TGT_HOLD };
    /* nearer player wins only by the margin: 1.0 vs 1.1 m keeps the target either way */
    CHECK(pick(0, n, 11.0f * 11.0f, 0, 0, 0, 0, F) == 0);
    CHECK(pick(1, 11.0f * 11.0f, n, 0, 0, 0, 0, F) == 1);
    CHECK(pick(0, f, n, 0, 0, 0, 0, F) == 1);
    CHECK(pick(1, n, f, 0, 0, 0, 0, F) == 0);
    /* no flipping back and forth around equal distances */
    { int t = 0, k; for (k = 0; k < 100; k++) t = pick(t, k & 1 ? n : n * 1.2f, k & 1 ? n * 1.2f : n, 0, 0, 0, 0, F); CHECK(t == 0); }
    /* holding (or driving its victim's motions): the victim stays, whatever the distances or who is busy */
    CHECK(pick(0, 30.0f * 30.0f, 1.0f, 0, 0, 0, 1, HOLD) == 0);
    CHECK(pick(0, 90.0f * 90.0f, 1.0f, 0, 0, 0, 1, HOLD) == 0);
    CHECK(pick(1, 1.0f, 30.0f * 30.0f, 0, 1, 1, 0, HOLD) == 1);
    CHECK(pick(0, n, n, 1, 0, 0, 0, HOLD) == 0);
    /* its grab/bite state: the victim stays while within 3 m, not when it is farther */
    CHECK(pick(0, 30.0f * 30.0f, 1.0f, 0, 0, 0, 1, A) == 0);
    CHECK(pick(1, 1.0f, 30.0f * 30.0f, 0, 1, 0, 0, A) == 1);
    CHECK(pick(0, n, n, 1, 0, 0, 0, A) == 0);
    CHECK(pick(0, 50.0f * 50.0f, n, 0, 0, 0, 0, A) == 1);
    CHECK(pick(0, 50.0f * 50.0f, n, 0, 1, 0, 0, A) == 0);   /* (not to a busy one) */
    /* its hit and death states commit to nobody: a staggering enemy takes the nearer player */
    CHECK(pick(0, 50.0f * 50.0f, n, 0, 0, 0, 0, F) == 1);
    /* a busy target is left for the free player within reach, not beyond it */
    CHECK(pick(0, n, f, 1, 0, 0, 0, F) == 1);
    CHECK(pick(1, f, n, 0, 1, 0, 0, F) == 0);
    CHECK(pick(0, n, 50.0f * 50.0f, 1, 0, 0, 0, F) == 0);
    /* never to a busy player, however near (also one the loop guard keeps it away from) */
    CHECK(pick(0, f * 4.0f, 1.0f, 0, 1, 0, 0, F) == 0);
    CHECK(pick(0, n, n, 1, 1, 0, 0, F) == 0);
    /* right in front: the player it would walk into is its target, before the distance rule */
    CHECK(pick(0, 12.0f * 12.0f, 11.0f * 11.0f, 0, 0, 0, 1, F) == 1);   /* 1.1 m in front vs 1.2 m behind */
    CHECK(pick(0, 10.0f * 10.0f, 14.0f * 14.0f, 0, 0, 0, 1, F) == 1);   /* also when the other is a little nearer, behind it */
    CHECK(pick(1, 10.0f * 10.0f, 14.0f * 14.0f, 0, 0, 0, 1, F) == 1);   /* and it stays (no flip back to the nearer) */
    CHECK(pick(0, 5.0f * 5.0f, 14.0f * 14.0f, 0, 0, 0, 1, F) == 0);     /* not over a target at less than half the distance */
    CHECK(pick(1, 5.0f * 5.0f, 14.0f * 14.0f, 0, 0, 0, 1, F) == 0);     /* (which it turns to) */
    CHECK(pick(0, 4.5f * 4.5f, 14.0f * 14.0f, 0, 0, 1, 0, F) == 0);     /* and then keeps */
    CHECK(pick(0, 14.0f * 14.0f, 10.0f * 10.0f, 0, 0, 1, 1, F) == 0);   /* both in front: it keeps its target */
    CHECK(pick(1, 10.0f * 10.0f, 14.0f * 14.0f, 0, 0, 1, 1, F) == 1);
    CHECK(pick(0, 14.0f * 14.0f, 5.0f * 5.0f, 0, 0, 1, 1, F) == 1);     /* (or the one at less than half the distance) */
    CHECK(pick(0, 12.0f * 12.0f, 11.0f * 11.0f, 0, 0, 0, 1, A) == 0);   /* mid-attack it keeps its victim */
    /* the anti flip-flop holds with the front rule: two players crossing in front alternately keep one target */
    { int t = 0, k, flips = 0; for (k = 0; k < 100; k++) { int u = pick(t, n, n, 0, 0, 1, 1, F); flips += u != t; t = u; } CHECK(flips == 0); }
    /* the loop guard: the third attack within the window trips it, then the enemy cools down */
    {
        CoopAtk a = { 0 };
        CHECK(!coop_atk_note(&a, 1000) && !coop_atk_note(&a, 1025));
        CHECK(coop_atk_note(&a, 1050));
        CHECK(coop_atk_cooling(&a, 1051) && coop_atk_cooling(&a, 1050 + COOP_LOOP_COOL - 1) && !coop_atk_cooling(&a, 1050 + COOP_LOOP_COOL));
        /* attacks spread wider than the window never trip it (a retail pace: a grab, then another) */
        memset(&a, 0, sizeof(a));
        { unsigned int t; int hit = 0; for (t = 100; t < 5000; t += COOP_LOOP_WIN / 2 + 1) hit |= coop_atk_note(&a, t); CHECK(!hit); }
        memset(&a, 0, sizeof(a));
        CHECK(!coop_atk_cooling(&a, 0) && !coop_atk_cooling(&a, 12345));
    }
}

int main(int argc, char** argv)
{
    test_target();
    int iters = argc > 1 ? atoi(argv[1]) : 200000;
    test_events();
    test_snaps();
    test_dgrams();
    test_xfer();
    test_ring();
    test_code();
    test_atr();
    test_trig();
    fuzz(iters);
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}

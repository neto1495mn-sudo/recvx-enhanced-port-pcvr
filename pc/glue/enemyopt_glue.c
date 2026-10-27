/* Enemy options (docs/ENEMIES.md "Enemy options"): (a) extra enemies of types the room already has,
 * (b) rearranged placements, (c) the enemy randomizer. Off by default; with everything off nothing
 * here touches the game.
 *
 * (a) runs on the host (or in single player) once per room load, at the first frame the room runs:
 * the extras are spawned like the F2 spawner's (spawn_glue.c spawn_add) and so reach a co-op guest
 * as added enemies. (b) and (c) change records at the end of the room load (room.c bhFinishRoom),
 * deterministically from the room file and the seed, on both co-op sides alike (the guest uses the
 * host's options), so the slots match and the host's snapshots drive the guest's copies.
 */
#include "../../decomp/include/ps2/veronica/prog/types.h"
#include "../../decomp/include/ps2/veronica/prog/main.h"
#include "../../decomp/include/ps2/veronica/prog/hitchkl.h"
#include "../../decomp/include/ps2/veronica/prog/system.h"
#include "../../decomp/include/ps2/veronica/prog/MdlPut.h"
#include "../../decomp/include/ps2/veronica/prog/hitchk.h"
#include "../src/host/actors.h"
#include "../src/host/enemies.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void host_log(const char* fmt, ...);
int coop_is_guest(void);
const char* spawn_add(const ActorModel* am, const SpawnNote* n, int* slot_out);
const char* spawn_at(const ActorModel* am, const SpawnNote* n, int want, int mode, int* slot_out);
const ActorModel* spawn_entry(int i);
int spawn_count(void);
void spawn_keep_texlist(const NJS_TEXLIST* l);
void bhCheckEneWorkNum(void);   /* eneset.h */

EnemyOpts g_enemy_opts;                 /* settings (F1, recvx.ini) */
static EnemyOpts host_opts;             /* a co-op guest's: the host's */
static int have_host_opts;
int enemy_room_gen;                     /* room loads so far */

/* test aids: RECVX_ENEMY_EXTRA / _PLACE / _RANDOM / _SEED override the settings */
__attribute__((constructor)) static void opts_env(void)
{
    const char* e;
    if ((e = getenv("RECVX_ENEMY_EXTRA"))) g_enemy_opts.extra = atoi(e);
    if ((e = getenv("RECVX_ENEMY_PLACE"))) g_enemy_opts.place = atoi(e);
    if ((e = getenv("RECVX_ENEMY_RANDOM"))) g_enemy_opts.rand = atoi(e);
    if ((e = getenv("RECVX_ENEMY_SEED"))) g_enemy_opts.seed = atoi(e);
}

const EnemyOpts* enemy_opts(void)
{
    static const EnemyOpts none;
    if (coop_is_guest()) return have_host_opts ? &host_opts : &none;
    return &g_enemy_opts;
}

static int kept_n;
void enemy_opts_from_host(const EnemyOpts* o)
{
    have_host_opts = o != NULL;
    if (!o) kept_n = 0;   /* (enemy_rand_kept: the host's kept records go with its session) */
    if (o) {
        host_opts = *o;
        host_log("enemies: the host's options: extra %d, placements %d, randomizer %d, seed %d\n", o->extra, o->place, o->rand, o->seed);
    }
}

/* ---------------------------------------------------------------- helpers */
static unsigned int rng;
static unsigned int rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }   /* xorshift32 */
static void rnd_seed(unsigned int a, unsigned int b)
{
    rng = a * 2654435761u ^ (b + 0x9E3779B9u);
    if (!rng) rng = 1;
    rnd(); rnd();
}
static int spot_player;                 /* find_spot avoids the player too (at run time, not at the room load) */
int room_file_now(void) { return sys->stg_no * 1000 + sys->rom_no * 10 + sys->rcase; }

/* what the room script does with each record (pc/tools/enemies.py classifies the same way) */
enum { REC_EVENT = 1, REC_SHOWN = 2, REC_HP = 4 };
static unsigned char rec_flags[128];
static const ActorModel* rand_src[128];   /* randomized records: where their new type comes from */
static int all_dead_check;              /* the script counts kills (DieEventCk): "all dead" doors */

static void scan_script(void)
{
    unsigned char* base = (unsigned char*)rom->evtp;
    unsigned int* tab = (unsigned int*)base;
    unsigned int offs[256], first, o, end;
    int n = 0, i, j;
    memset(rec_flags, 0, sizeof(rec_flags));
    all_dead_check = 0;
    if (!base) return;
    first = tab[0];
    for (i = 0; (unsigned)i * 4 < first && i < 256; i++) {
        offs[n++] = tab[i];
        if (tab[i] && tab[i] < first) first = tab[i];
    }
    for (i = 0; i < n; i++) {
        unsigned int ends[64];
        int depth = 0;
        for (end = 0x7FFFFFFF, j = 0; j < n; j++)
            if (offs[j] > offs[i] && offs[j] < end) end = offs[j];
        for (o = offs[i]; o < end && o < (unsigned)sys->rdtsz;) {
            unsigned char* p = base + o;
            int len = coop_insn_len(p), op = p[0];
            while (depth && ends[depth - 1] <= o) depth--;
            if (len <= 0) break;
            switch (op) {
            case 101: case 104: case 181: if (p[1] == 1) rec_flags[p[2] & 127] |= REC_EVENT; break;   /* LoadWork, 2, Ex */
            case 45: case 48: case 83: case 125: rec_flags[p[1] & 127] |= REC_EVENT; break;   /* motion pauses, links, positioning */
            case 81: rec_flags[p[2] & 127] |= REC_EVENT; break;
            case 84: rec_flags[p[1] & 127] |= REC_EVENT; rec_flags[p[2] & 127] |= REC_EVENT; break;
            case 142: case 143: case 154: case 165: case 204: rec_flags[p[1] & 127] |= REC_EVENT; break;   /* zombie/face/render/look ops read the type's work */
            case 144: rec_flags[p[1] & 127] |= REC_EVENT; break;
            case 121: rec_flags[p[2] & 127] |= REC_EVENT; break;   /* EneSeOn */
            case 7: if (p[2] == 10) rec_flags[p[1] & 127] |= REC_HP; break;   /* CmpW on the HP */
            case 9: if (p[1] >= 11 && p[1] <= 15) rec_flags[p[1] - 11] |= REC_HP; break;   /* SvW */
            case 153: case 208: rec_flags[p[1] & 127] |= REC_HP; break;   /* EnemyHpUp, EneLifeSetM */
            case 36: if (p[1] == 1) rec_flags[p[2] & 127] |= REC_SHOWN; break;   /* InitModelSet */
            case 33: all_dead_check = 1; break;   /* DieEventCk */
            case 1: if (depth < 64) ends[depth++] = o + p[1] + 1; break;   /* IfelCk */
            case 2: if (depth) depth--; if (depth < 64) ends[depth++] = o + p[1]; break;   /* ElseCk */
            case 3: if (depth) depth--; break;   /* Endif */
            }
            o += len;
            if (!depth && (op == 0 || (i >= 2 && op == 255))) break;
        }
    }
}

/* a free spot near (x, z) on floor `flr`: no wall between it and the spot (plus a body's width
   beyond), away from the player; 1 = found */
static int near_entry(float x, float z, int player)
{
    int i;
    for (i = 0; i < rom->pos_n; i++) {
        float dx = x - rom->posp[i].px, dz = z - rom->posp[i].pz;
        if (dx * dx + dz * dz < 40.0f * 40.0f) return 1;
    }
    if (player) {
        float dx = x - plp->px, dz = z - plp->pz;
        if (dx * dx + dz * dz < 40.0f * 40.0f) return 1;
    }
    return 0;
}

static int find_spot(float x, float y, float z, int flr, float rmin, float rmax, float* ox, float* oz)
{
    int k, start = rnd() & 7;
    for (k = 0; k < 16; k++) {
        float a = (float)((start + k * 3) & 15) * (6.2831853f / 16.0f);
        float r = rmin + (rmax - rmin) * (float)(rnd() & 1023) / 1023.0f;
        float tx = x + r * sinf(a), tz = z + r * cosf(a);
        NJS_POINT3 p1 = { x, y + 10.0f, z }, p2 = { x + (r + 8.0f) * sinf(a), y + 10.0f, z + (r + 8.0f) * cosf(a) };
        if (near_entry(tx, tz, spot_player)) continue;
        if (bhCollisionCheckLine2(&p1, &p2, 0x4400, (char)flr)) continue;
        *ox = tx; *oz = tz;
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------- (b) rearranged placements */
static int roams(int id) { return id == 1 || id == 3 || id == 4 || id == 5 || id == 7 || id == 9 || id == 26; }

static void place(const EnemyOpts* o)
{
    int k, cand[128], nc = 0, moved = 0;
    for (k = 0; k < rom->ene_n && k < 128; k++)
        if ((ene[k].flg & 0x1) && !(rec_flags[k] & (REC_EVENT | REC_SHOWN)) && roams(ene[k].id)) cand[nc++] = k;
    for (k = 0; k < nc; k++) {
        BH_PWORK* ep = &ene[cand[k]];
        ETTY_WORK* r = &rom->enep[cand[k]];
        float x = ep->px, z = ep->pz, nx, nz;
        rnd_seed((unsigned)o->seed, (unsigned)room_file_now() * 131u + (unsigned)cand[k] + 0x5000u);
        if (nc > 1 && (rnd() & 1)) {   /* start from another record's spot on the same floor */
            BH_PWORK* q = &ene[cand[rnd() % nc]];
            if (q->flr_no == ep->flr_no && fabsf(q->py - ep->py) < 1.0f) { x = q->px; z = q->pz; }
        }
        if (!find_spot(x, ep->py, z, ep->flr_no, 8.0f, 30.0f, &nx, &nz)) continue;
        ep->px = ep->pxb = r->px = nx;
        ep->pz = ep->pzb = r->pz = nz;
        ep->ay = r->ay = (short)(rnd() & 0xFFFF);
        if (ep->id == 4 && ep->type) ep->type = r->type = 0;   /* a Cerberus waiting in its wake-up area would wait forever */
        bhCalcModel(ep);
        moved++;
    }
    if (nc) host_log("enemies: room %04d: %d of %d free-roaming enemies placed elsewhere (seed %d)\n", room_file_now(), moved, nc, o->seed);
}

/* ---------------------------------------------------------------- (c) randomizer */
/* Free-roaming records become other types within the slot budget of docs/ENEMIES.md "Requirements for randomizers and difficulty options"
   (helper code, et_lp rows and the texture heap no longer limit the mix on the PC); everything else
   (story, cutscene, scripted HP, bosses, "all dead" rooms) stays; any failure keeps the original. */
/* A co-op guest randomizes as the host (the same options, seed and room file), but each side refuses a
   record whose type does not fit its own texture memory, and the guest's puppets follow the host's
   slots. So the guest follows the host: its spawns take no texture reserve (SPAWN_DICTATED), and the
   records the host had to keep come as SPAWN_NOTE_KEPT notes (coop_rand_kept) before the guest loads
   the room - with the host there first, the usual case; a guest that was there first logs a
   difference. Kept here until the guest's next load of that room. */
#define KEPT_MAX 64
static struct { short room; unsigned char rec; } kept[KEPT_MAX];

void enemy_rand_kept(int room, int rec, int id)
{
    int k;
    if (rec >= 128) return;
    if (sys && rom && (sys->tk_flg & 0x80) && room == room_file_now() && rec < rom->ene_n && (ene[rec].flg & 0x1) && ene[rec].id != id)
        host_log("enemies: the host kept record %d of room %04d as type %d; here it is type %d (this room was loaded before the host's)\n", rec, room, id, ene[rec].id);
    for (k = 0; k < kept_n && !(kept[k].room == room && kept[k].rec == rec); k++);
    if (k < kept_n) return;
    if (kept_n == KEPT_MAX) memmove(kept, kept + 1, sizeof(kept[0]) * --kept_n);
    kept[kept_n].room = (short)room;
    kept[kept_n++].rec = (unsigned char)rec;
}

static int boss_or_set_piece(int id)
{
    return id == 2 || (id >= 11 && id <= 19) || id == 23 || id == 25 || id == 29 || id == 30 || (id >= 53 && id <= 56) || id == 0 || id == 24;
}
static int rand_pool(int id, int stage) { return id == 1 || id == 4 || id == 9 || (stage >= 2 && (id == 3 || id == 5)); }
#define slot_cost spawn_slots_needed

static void randomize(const EnemyOpts* o)
{
    int k, j, cand[128], nc = 0, cost = 0, kinds = 0, made = 0;
    int pool[128], np = 0, kind[2], guest = coop_is_guest();
    unsigned char held[128] = { 0 };   /* guest: records the host kept */
    for (k = j = 0; k < kept_n; k++)   /* (this room's notes are used up by this load) */
        if (guest && kept[k].room == room_file_now()) held[kept[k].rec] = 1;
        else kept[j++] = kept[k];
    kept_n = j;
    if (all_dead_check) { host_log("enemies: room %04d not randomized (its doors count the kills)\n", room_file_now()); return; }
    for (k = 0; k < rom->ene_n && k < 128; k++) {
        BH_PWORK* ep = &ene[k];
        if (!(ep->flg & 0x1)) continue;
        if (boss_or_set_piece(ep->id)) { host_log("enemies: room %04d not randomized (boss or set piece %d)\n", room_file_now(), ep->id); return; }
        if (!rec_flags[k] && (ep->id == 1 || ep->id == 3 || ep->id == 4 || ep->id == 5 || ep->id == 9)) cand[nc++] = k;   /* (not the fliers: their records hover) */
        else if (ep->id < 31) cost += slot_cost(ep->id);
        else cost += 1;
    }
    if (!nc) return;
    for (j = 0; j < spawn_count(); j++)
        if (rand_pool(spawn_entry(j)->id, sys->stg_no)) pool[np++] = j;
    rnd_seed((unsigned)o->seed, (unsigned)room_file_now() * 977u + 0x3000u);
    for (j = 0; j < 40 && kinds < 2; j++) {   /* up to two kinds */
        const ActorModel* m = spawn_entry(pool[rnd() % np]);
        int i, dup = 0;
        for (i = 0; i < kinds; i++) dup |= spawn_entry(kind[i]) == m;
        if (dup) continue;
        { int q; for (q = 0; q < spawn_count() && spawn_entry(q) != m; q++); kind[kinds++] = q; }
    }
    for (k = 0; k < nc; k++) {
        int rec = cand[k], i;
        const ActorModel* m = spawn_entry(kind[rnd() % kinds]);
        BH_PWORK save = ene[rec];
        SpawnNote n = { 0 };
        const char* why;
        if (cost + slot_cost(m->id) > 128 - SPAWN_SLOTS_FREE) m = spawn_entry(kind[0]);
        if (cost + slot_cost(m->id) > 128 - SPAWN_SLOTS_FREE || (m->id == ene[rec].id && m->mdlver == ene[rec].mdlver)) { cost += slot_cost(ene[rec].id); continue; }
        if (held[rec]) {   /* as the host's: the original */
            cost += slot_cost(save.id);
            host_log("enemies: record %d stays %d (as the host's)\n", rec, save.id);
            continue;
        }
        n.room = m->room; n.rec = m->rec; n.id = m->id; n.mdlver = m->mdlver;
        n.px = save.px; n.py = save.py; n.pz = save.pz; n.ay = save.ay; n.flr = save.flr_no;
        ene[rec].flg = 0;
        why = spawn_at(m, &n, rec, guest ? SPAWN_LOADING | SPAWN_DICTATED : SPAWN_LOADING, NULL);
        if (why) {   /* softlock-safe fallback: the original record as it was */
            SpawnNote o = { (unsigned short)room_file_now(), (unsigned char)rec, (unsigned char)save.id, (unsigned char)save.mdlver, (unsigned char)save.type,
                            save.flr_no, SPAWN_NOTE_KEPT, save.px, save.py, save.pz, (short)save.ay, 0 };
            ene[rec] = save;
            bhCheckEneWorkNum();
            cost += slot_cost(save.id);
            host_log("enemies: record %d stays %d (%s)%s\n", rec, save.id, why, guest ? "; the host's may differ" : "");
            if (!guest) coop_rand_kept(rec, &o);   /* the guest keeps it too */
            continue;
        }
        for (i = 0; i < save.mdl_n; i++)   /* the original's textures are released with the room's (the record now holds the new type) */
            if ((save.mdl[i].flg & 0x200) && save.mdl[i].texP) spawn_keep_texlist(save.mdl[i].texP);
        rand_src[rec] = m;   /* (the room file still says the old type: extra enemies copy from here) */
        cost += slot_cost(m->id);
        made++;
        host_log("enemies: room %04d record %d: type %d model %d -> %s (type %d model %d)\n", room_file_now(), rec, save.id, save.mdlver, m->name, m->id, m->mdlver);
    }
    host_log("enemies: room %04d: %d of %d free-roaming enemies randomized (seed %d)\n", room_file_now(), made, nc, o->seed);
}

/* ---------------------------------------------------------------- the room load (room.c bhFinishRoom) */
void enemy_room_finished(void)
{
    const EnemyOpts* o;
    enemy_room_gen++;
    if (!rom || !sys) return;
    o = enemy_opts();
    if (!o->place && !o->rand && !o->extra) return;
    scan_script();
    memset(rand_src, 0, sizeof(rand_src));
    spot_player = 0;
    if (o->rand) randomize(o);
    if (o->place) place(o);
    spot_player = 1;
}

/* ---------------------------------------------------------------- (a) extra enemies */
/* types that roam freely and die like any enemy */
static int extra_type(int id) { return id == 1 || id == 3 || id == 4 || id == 5 || id == 9 || id == 22 || id == 26; }

void enemy_extras_frame(void)
{
    static int done_gen = -1;
    const EnemyOpts* o;
    int cand[128], nc = 0, k, made = 0;
    if (done_gen == enemy_room_gen || !sys || !plp || !rom) return;
    o = enemy_opts();
    if (!o->extra || coop_is_guest()) { done_gen = enemy_room_gen; return; }
    if (!(sys->tk_flg & 0x80) || !(sys->sp_flg & 0x2) || (sys->gm_flg & 0x9) || (sys->cb_flg & 0x1)) return;   /* the room runs */
    done_gen = enemy_room_gen;
    for (k = 0; k < rom->ene_n && k < 128; k++) {
        BH_PWORK* ep = &ene[k];
        if (!(ep->flg & 0x1) || (ep->flg & 0x2) || (ep->stflg & 0x1000000) || ep->mode0 == 5 || !extra_type(ep->id) || rec_flags[k]) continue;
        cand[nc++] = k;
    }
    rnd_seed((unsigned)o->seed, (unsigned)room_file_now() * 7u + 1u);
    for (k = nc - 1; k > 0; k--) { int j = rnd() % (k + 1), t = cand[k]; cand[k] = cand[j]; cand[j] = t; }   /* shuffle */
    for (k = 0; k < nc && made < o->extra; k++) {
        const ETTY_WORK* r = &rom->enep[cand[k]];
        BH_PWORK* src = &ene[cand[k]];
        ActorModel am = { "extra enemy", (unsigned short)room_file_now(), (unsigned char)cand[k], (unsigned char)r->id, r->mdlver, 0, 0 };
        if (rand_src[cand[k]]) am = *rand_src[cand[k]];   /* a randomized record: a copy of its new type */
        SpawnNote n = { 0 };
        const char* why;
        n.room = am.room; n.rec = am.rec; n.id = am.id; n.mdlver = am.mdlver;
        n.py = src->py; n.flr = src->flr_no; n.ay = (short)(rnd() & 0xFFFF);
        if (!find_spot(src->px, src->py, src->pz, src->flr_no, 15.0f, 35.0f, &n.px, &n.pz)) continue;
        why = spawn_add(&am, &n, NULL);
        host_log("enemies: extra %s (type %d model %d, from record %d) at %.0f %.0f: %s\n", am.name, am.id, am.mdlver, am.rec, n.px, n.pz, why ? why : "spawned");
        if (!why) made++;
        else if (strstr(why, "slots") || strstr(why, "memory")) break;
    }
    if (nc) host_log("enemies: room %04d: %d extra enemies (of %d eligible records, option %d)\n", room_file_now(), made, nc, o->extra);
}

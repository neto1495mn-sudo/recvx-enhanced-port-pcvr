# Enemies

How a room spawns and runs its enemies, what the PC port changed, and the features built on
it: the F2 enemy spawner and the enemy options (extra enemies, rearranged placements,
randomizer). Related: [ACTORS.md](ACTORS.md) (models, skeletons, motions, the actor
catalogue), [modding/ROOM_FORMAT.md](modding/ROOM_FORMAT.md) (room file layout, records,
scripts and opcodes), [COOP.md](COOP.md) (host-authoritative enemies),
[USER_GUIDE.md](USER_GUIDE.md) (cheats).

Paths are relative to `decomp/src/ps2/veronica/prog/` unless noted. Room counts come from
`pc/tools/enemies.py` over all 205 room files.

## Summary

* Every enemy of a room is a **record** of the room file (759 in all rooms), realized at load into
  the slot of the same index. **No script op creates an enemy**: scripts hide, show, drive and
  heal records; a "script-spawned" enemy is a record hidden at entry and shown on a flag.
  Everything else is spawned by enemies (zombie parts, helpers, babies, eggs, segments).
* A type's model, motions and textures exist **only in room files**. Room variants are separate
  files (`RM_SRRC`, C = case, picked by op 55 `RoomCaseNo`). Difficulty never changes placements.
  111 hostile records have no kill bit and come back on every visit.
* The PC lifts the PS2's texture and helper-code budgets; the binding budget is the **128 enemy
  slots** ("Budgets on the PC").

## Where a room's enemies come from

### Records, realized at room load

`system.c bhSysCallMonitor` mode 4 reads the room file; `room.c bhSetRoom` walks the enemy
records: every record with `flg & 1` goes through `room.c bhSetEneMdl` -> `eneset.c bhSetEnemy`,
which takes the **first free slot** of `ene[128]` (at load: slot `k` for record `k`).
`bhFinishRoom` assumes slot `k` = record `k` for motion sets and matrices, and every script op
addresses a record as `ene[rom->enep[k].wrk_no]`. Record order and count are therefore
load-bearing.

Init runs on the slot's first enemy tick (`mode0 == 0`). Enemies present at load get their first
tick from `bhInitEvent` (the entry script `scd0`), before the first frame is shown.

### What the scripts do to records

| Op | Effect |
|---|---|
| 13 `DieCk rec, bit` (every frame in `scd1`) | the record is dead (`flg & 2`): set `ed_flg` bit (`& 0x3FF`) |
| 34 `EneSetCk rec, bit` (in `scd0`) | `ed_flg` bit set: hide the slot (`stflg \|= 0x1000000`); bit masked with `0x1FF` |
| 33 `DieEventCk n, bit, cmp, count` | count of set bits `bit..bit+n-1` compared ("all dead" checks) |
| 36 `InitModelSet 1, rec, 0/1` | hide (also `flg \|= 0x8000`) / show the slot |
| 101/104/0xD3 `LoadWork 1, rec` | an event task works on this enemy (controllers 100/103/105 move and animate it) |
| 45/48 `InitMotionPause(Ex) rec` | the enemy plays room motions for the script (`mode0 = 5`, `bhEne_Event`) |
| 7 `CmpW rec, 10, ...` / 9 `SvW 11..15` | test / set HP (SvW only records 0..4) |
| 153 `EnemyHpUp`, `EneLifeSetM` | raise / set HP |
| 142 `ZombieUpDieCk`; 81/83/84 links, 121 `EneSeOn`, 125 positioning, render/look/face ops | other per-record effects |

The 569 hostile records (ids 0..30, 53..56, 71):

| Class | Records | Meaning |
|---|---|---|
| free | 283 | not hidden at entry, not touched by an event (186 with a kill bit) |
| event-driven | 252 | `LoadWork` targets: cutscene actors, enemies that rise/burst in on cue, bosses |
| hidden at entry, shown on a condition | 13 | e.g. RM_0060 #3 (shown once `ev_flg` 116 is set and kill bit 15 is clear) |
| hidden at entry, shown by events | 16 | e.g. RM_3020 Cerberus, RM_3160 zombies |

`pc/tools/enemies.py room ROOM` prints each reference with its enclosing conditions (e.g.
"after story event X": `Ck ev_flg X set; Ck ed_flg K clear; InitModelSet 1 rec show`). 14 rooms
branch on the character played (`CmpB ply_id`); none on `gm_mode`.

### Spawned at run time by enemies

| Parent | Children (ids) | Where |
|---|---|---|
| zombie (1) | linked parts, ids 31..38: link, head, 2 arms, 2 legs, + cap / worm / bomb by model version | `en01.c bhEne01_SetLinkEnemy`; 7 extra slots for a plain zombie |
| Hunter (5) | 1 helper (31) | `en05.c` |
| Black Widow (3) | leader, children (31), 15 babies (24) | `en03.c` |
| Giant Black Widow (23) | children (31) | `en23.c` |
| moth (6) | leader, children (31), eggs (27) on the player | `en06.c`, `player.c bhCheckMothEgg` |
| Gulp Worm (2) | segments | `en02.c` |
| Alexia 2 (13) | tentacles (18), body part (31) | `en13.c` |
| Monster Steve (17) | arms (31, 32) | `en17.c` |
| tentacle (29) | a damage dummy | `en29.c` |

`bhSetEnemy` returns NULL when the 128 slots are full, and none of these callers checks it.

## Data an enemy type needs

### Per room file

| Data | Room file table | Realized by | Lives in |
|---|---|---|---|
| Model: chunk list (MLB sub-models, `SKIN`, `MASK` for ids > 90) | `hed01`, one per record | `bhSetEneMdl` (relocates in place) | room arena |
| Motions: 512-entry set per type | `hed02`, first record of the type that has one; others share `sys->emtp[id]` | `bhFinishRoom` / `bhSetEneMtn` | room arena |
| Textures | `hed04`, one per first (type, model version, textured sub-model) | upload list built by `bhSetRoom` | texture heap; released by `bhReleaseMainTexture` at the door |
| `O_WORK` matrices, AI work (`exp0`) | - | `bhKeepObjWork`, the type's Init | room arena |

Also from the room: **enemy floor areas** (floor records of type 2 whose `prm0` is the enemy
type: Cerberus wake-up areas, Hunter stairs and jump spots, zombie areas), read by
`zonzon1.c bhEne_CheckEventAtr`; **route boxes** (`rutp`/`ruttp`, 114 rooms) for path finding
(`rutchk.c bhCheckRoute`); water areas (fish, albinoids); the room's **effect textures** (effect
texture ids 30..99 and 400+) and its **sound bank** (SE bank 3 with the sounds of the room's own
types).

Global: the effect code, effect textures 0..29, the player's part file (the moth egg uses
Claire's sub-models 6/7 and player motions 93..95), the enemy code. An effect's *texture* id
(`O_WRK.tex_id`, through `sys->ef_tn`) is not its effect id: a few global enemy effects draw with
a room effect texture ("Effect textures of added types").

### Budgets on the PC

| Resource | PS2 | PC | Notes |
|---|---|---|---|
| Enemy slots | 128 (`ene[]`) | unchanged | retail uses up to 76 records (RM_9170, ants); a zombie takes 8. Slots are byte-sized in the decomp and in the co-op packets, so raising them is a protocol change |
| Room arena | 12.25 MB incl. ~1.8 MB player data | **+64 MB** (`PC_ARENA_EXTRA`) | allocators never check `sys->endp`; the map/file/option screens copy the room's textures above `sys->memp` (2.2..6.1 MB), which must stay free |
| Texture heap | 10 MB (`Ps2_tex_buff`) | **64 MB** (`PC_TEX_HEAP_SIZE`, `pc_prelude.h`) | full = `exit(0)`; retail rooms use up to 7.6 MB |
| Texture entries | 256 (`tbuf`) | **1024** (`PC_TEX_ENTRIES`) | first-free and bump allocation, so vanilla rooms use the same entries as on the PS2 |
| Texture slots per type | `sys->et_lp[100][16][16]` | model versions 16..63 in a table of their own (`room.c pc_et_lp`) | on the PS2 zombie model versions >= 16 alias other types (21 = Gulp Worm 5, 32..42 = Black Widow 0..10); no retail room combines them |
| Spawned types' data | room arena | **32 MB pool** (`spawn_glue.c`), emptied by `bhSetRoom` | model + motion chunk lists, 6 KB..1.1 MB per type |
| Motion sets | 512 per type, `sys->emtp[128]` | unchanged | |
| Enemy sound bank | one SE bank 3 per room | **31 extra banks**, one per source room, released with the room | "Sounds of added types" |
| Helper code | `bhJumpEnemy[31..38]`, global, shared | **per creator type** (`eneset.c pc_call_enemy`) in rooms with added enemies | users: zombie 31..38, Monster Steve 31/32, Gulp Worm, Black Widow, Hunter, moth, Alexia 2, Giant Black Widow 31. A part runs its creator's function; retail rooms keep the shared table |

With these budgets 25..30 foreign types fit in one room before the slots run out
(`RECVX_SPAWN_SURVEY`). Added for mixed rooms (none of it occurs in retail rooms):

* Slot counts come from the types' Inits, plus 6 kept free (a Gulp Worm takes 19).
* An added albinoid adult's light (`en22.c` links it by slot) uses the slot beyond the room's
  records; a grab plus another type's hit at once clamps the damage level (`player.c
  bhCPM0_damage`).
* **One enemy takes a player at a time** (`eneset.c pc_call_enemy`): the enemy whose code set the
  player's busy flag (`flg & 4`) owns the player until it clears; other enemies' changes to that
  player work meanwhile are undone (parts count as their parent; the co-op partner too). A zombie
  refused this way lets go (`coop_glue.c pc_enemy_unhold`).
* Watchdog (`coop_glue.c pc_enemy_watch`): an alive enemy unchanged for 150 steps is logged; a
  zombie stuck in a grab state is released.

Mixing types the PS2 never mixed can still show odd behaviour (a Hunter may leap out of a room).

### What breaks when a type the room did not load is added

| Missing | Effect | Handled by the spawner |
|---|---|---|
| model / motions | nothing to draw, `bhSetMotion` on an empty set | loaded from a source room file |
| textures | the instance owns an empty `et_lp` slot: wrong or no texture, never freed | uploaded from the source room's `hed04` entry, released with the room |
| helper slots | a zombie's head runs the Hunter's helper or vice versa | each type's parts run its own code |
| children's slots | `bhSetEnemy` NULL -> crash in the parent's Init | refused unless the type, its children and 6 more slots fit |
| floor areas | waiting variants never wake (Cerberus 1/2); no stairs/jumps for the Hunter | spawns variant 0 |
| effect textures | effect texture 0 (a page of kanji) instead of the room effect texture | uploaded from the source room; none: not drawn ("Effect textures of added types") |
| sounds | the type plays its ids against the room's bank 3: another type's sounds | the source room's bank 3 as an extra bank; none: silent ("Sounds of added types") |
| CLUT banks | an asset's fixed palette bank could overwrite the room's | not handled; not observed |

## Runtime

* `ene[128]` (`BH_PWORK`, 0x580 bytes); `sys->ewk_n` = highest used slot + 1; free =
  `!(flg & 1)`. `eneset.c bhControlEnemy` runs `bhJumpEnemy[id](ep)` then `bhEne_InitDamage` for
  every used slot not hidden/frozen (`stflg & 0x41000000`) or linked to a hidden parent; co-op
  hooks wrap the call. `bhDrawEnemy` draws the same set.
* `bhJumpEnemy[id]` -> `bhEneNN` (`enNN.c`) -> `bhEneNN_Mode0[mode0]`: 0 `Init` (work, HP from
  the difficulty table, collision), 1 `Move` (the brain picks `mode1`/`mode2`), 2 `Nage`
  (grabs/throws, the player animated from the enemy's motion set), 3 `Damage`, 4 `Die` (sets
  `flg & 2`), 5 `bhEne_Event` (script acting). Types with their own code: 0, 1..27, 29, 30, 53,
  54, 55, 71; 56 is a scripted object; other ids up to 99 run `subpl.c bhSubpl` (NPCs, no AI). The
  AI targets `plp` (co-op swaps it per enemy).
* **Death and kill flags**: the body stays in its slot until the room is left. `scd1` runs `DieCk`
  every frame; `ed_flg` is in the save, so kills persist. `scd0` runs `EneSetCk` on the next visit
  and hides dead records; records without a `DieCk` respawn. Kill bits above 511 would not hide
  (retail uses up to 357). "All dead" checks (`DieEventCk`): rooms 3210, 4012, 5550..5710.
* **Difficulty** (`sys->gm_mode`: 0/1 normal, 2 easy, 3 Battle Game): easy doubles the player's
  HP (320), takes enemy HP from the second row of the type's table and changes ammo pickups
  (`item.c getbulletmax`). Counts, placement and speed do not depend on it.

## Co-op

Details in [COOP.md](COOP.md); what matters for enemies:

* The host runs the AI; the guest's enemies are puppets updated from per-slot snapshots (no type
  or model), so both sides must hold the same enemy in the same slot. A packet carries at most 48
  snapshots (`COOP_SNAP_FRAME`); with more active slots each packet sends the next 48.
* **Added enemies** (F2 spawner, extra enemies) are announced with one reliable `EV_SPAWN`
  {slot; source room file and record; type, model version, variant; position, angle, floor}. The
  guest realizes it with the spawner's code into the same slot (`spawn_glue.c spawn_note_apply`
  -> `spawn_at`, skipping lower free slots if needed); its Init creates the same children, and
  from then on it is an ordinary puppet. Notes arriving during a door, event or menu wait in
  `spawn_pending` (at most 900 frames). The host resends them when the guest (re)enters the room
  (`spawn_replay`); a door clears them on both sides. Randomizer records the host had to keep go
  along as notes marked `SPAWN_NOTE_KEPT`.
* **The host's enemy options** go to the guest as `EV_EOPTS` (session start, before the game
  state, on change) and replace the guest's. Placements and the randomizer depend only on the room
  file and the seed, so both sides make the same records in the same slots. The guest never spawns
  (F2 refuses there).
* **Targets** (host, `coop_proto.c coop_pick_target`): each enemy goes for one player at a time:
  kept through its grab or bite, never a busy one, the one right in front, else the nearer by a
  margin (another floor counts as twice as far); a busy target is left for a free player within
  4 m. A loop guard makes an enemy that attacks the same player three times within 90 frames
  leave it alone for 60 (COOP.md). The partner's stand-in carries what the AI reads of a player (position, angles, modes, hp,
  `stflg`, hit capsule `watr`, floor, busy bits); attacks on it reach the guest as `EV_PLY_HIT` or
  `EV_GRAB`. A zombie grab on the guest makes the slot the guest's until handed back (~10 s
  timeout).
* **Kills while the guest runs the room** (`EV_GONE`): at the take-over the guest names its dead
  enemies; the host hides its matching copies for that visit.

## Requirements for randomizers and difficulty options

1. **Record count and order** stay (scripts, `wrk_no`, model/motion/texture tables, co-op slots).
2. **Kill bits**: a replaced record keeps its bit; extra enemies get none. New bits < 512.
3. **Leave alone**: event-driven and event-shown records, NPCs (ids >= 31), ids 0, 25, 28, 53, 56.
4. **Bosses and set pieces** (2, 11..17, 19 in its story rooms, 23, 25, 29, 30, 53..55): their
   rooms' scripts wait for their death or events; replacing them can softlock the room.
5. Types that cannot die normally (29, 53, 25, 0) must not replace a record that a `DieEventCk`
   or a door condition counts.
6. **Slots**: children within 128. Water types (8, 21, 22) need water, fliers (6, 7) head room.
7. **Variant** in range (zombie 0..15, Tyrant 0..2, others mostly 0..3); waiting variants need the
   room's floor areas.
8. **Grab motions** come with the type's motion set; Wesker (`ply_id` 3) overruns some 3-row
   tables.
9. **Co-op**: decide everything from the room file, the flags and a shared seed, at the same
   point of the load on both sides.

Softlock risks: "all dead" rooms, events that `LoadWork` a record expecting its motion numbers,
items given after a scripted kill, enemies on the entry position or in a door's trigger box.

## Enemy spawner (F2)

F2 -> "Spawn enemy" (also the EXTRAS page): "Enemy" picks one of 58 entries (the "You appear as"
catalogue entries with ids up to 30, plus Gulp Worm, Alexia 2 and 3, Alexia's baby, tentacle,
both knights, ceiling trap, Nosferatu's core, Alfred, the ants, the baby spider, the exploding
prop); "Spawn in front of me" puts it 3 m in front of the player, facing it. Code:
`pc/glue/spawn_glue.c`.

Steps, as the room loader does it for a record:

1. Checks: in a room, no door/event, not the co-op guest; slots for the type and its children
   (`spawn_slots_needed`: zombie 10, Gulp Worm 19, Black Widow 25, moth 16, Alexia 2 12, others
   1..3) plus 6 kept free; arena, spawn pool, texture entries (16 spare) and heap (1 MB spare).
2. `actor_find` (`pc/glue/actor_glue.c`, shared with the appearance loader) reads the source room
   file from `RDX_LNK.AFS` and finds the record's model chunk list, the type's motion list and the
   texture entries of its textured sub-models. The source is the smallest room holding the model
   and motion set (`pc/tools/enemies.py sources`).
3. The model list is copied into the spawn pool and realized with `bhSetEneMdl` on a copy of the
   source record (variant 0 except ants C, tentacle, core); it takes the first free slot and
   shares the textures of a (type, model version) the room already has (`sys->et_lp`).
4. New texture owners get their textures uploaded from the source room (`bhSetMemPvpTexture`);
   a hook in `ps2_texture.c bhReleaseMainTexture` releases them with the room.
5. The motion set: the room's own for the type, else the source's, copied and realized
   (`bhSetEneMtn`). Then `bhKeepObjWork` + `bhCalcModel`; Init runs on the next tick.
6. Sounds and effect textures (below).

A spawned enemy has no record: no kill bit, scripts never see it, gone after a room change.

| Type | Behaviour outside its own rooms |
|---|---|
| zombies, Hunter, Bandersnatch, Cerberus, Black Widow, Giant Black Widow, Tyrant, Nosferatu, Monster Steve, anatomist, Alexia's first form | move, attack, take damage, die (Tyrant: frozen death pose; Cerberus spawned as variant 0) |
| bat, moth, Spotter | fly, take damage and die; rarely attack |
| albinoid adult | walks on land |
| Gulp Worm | burrows; wanders out of the room without its sand arena |
| fish, ants, tentacle, ceiling trap, Nosferatu's core, exploding prop | spawn and run, but belong to water, walls, their boss or an event |
| **refused**: Baby Black Widow (24), Alexia's baby (30) | their code reads the parent that spawns them (`lkwkp`) |
| **refused**: Alexia second/third form (13, 14) | the second form reads its partner pointer (NULL) |
| **refused**: Alfred (16) | his boss logic corrupts memory outside his room |

### Sounds of added types

Enemy code requests a sound id (`RequestEnemySe`, `bhEne_CallSE`; effects through
`bhEne_CallEffectSE` as enemy 127): bits 0..7 list (program), 8..11 SE bank, 12..15 how many may
play at once, 16..19 priority. Every enemy sound is in **bank 3**, played on one of 6 enemy SE
handles from SE port 3. A room's sound pack `RM_SRRC.SPQ` (key `stage*1000 + room*10 + case` in
`MULTSPQ1.IDX` for stages 0..5, `MULTSPQ2.IDX` for 6..9) loads bank 2 (room sounds) and, in rooms
with enemies, bank 3 with the room's own types' sounds. List numbers are per type and collide
between types, so an added type would play another type's sounds.

On the PC, when `spawn_at` realizes a type the room file has no record of (F2 spawner,
randomizer, co-op guest), `spawn_glue.c sound_load` reads bank 3 of its source room
(`snd_iop.c snd_xbank_load`; else another catalogue room of the type) into an extra synth bank
(one per source room, at most 31) and records type -> bank. Each request is attributed when made
(`sdfunc.c RequestEnemySeBasic` -> `eneset.c pc_enemy_sound_type`): the enemy whose code runs, a
part or child as its creator, an effect as the type that made it (`effect.c bhSetEffectTb`). Such
a sound carries the bank in bits 3..7 of the request's port byte (`ps2_sg_sd.c sdSysServer`; 0 on
the PS2) and `snd_synth.c` plays it from that bank; a type without a bank plays nothing. Everything
else keeps the room's banks, so vanilla rooms send the same driver commands. `bhSetRoom`
(`spawn_room_setup`) stops and releases the extra banks. Variants of one type share the first
one's bank. Player voices in another character's voice use the same 31 banks
([ACTORS.md](ACTORS.md) "Voices").

### Effect textures of added types

Enemy effects (ids 250..299) are global code, but an effect picks its texture by `tex_id`
through `sys->ef_tn[tex_id]`, and some take a room effect texture (ids 30..99, 400+), which
`room.c bhSetRoom` uploads only for the room's own effect records. Without it the effect drew
texture 0 (a page of kanji):

| Type | Effects | Room effect texture | Rooms with it |
|---|---|---|---|
| Alexia, first form (12) | 262/263 her fire, 265 `bhEff_E12_FrameLiquid` (burning blood on the floor) | 80 | RM_5820, 9301, 9321, 5810, 9302, 9370 |
| Gulp Worm (2) | 253 `bhEff_E02_SandDust` | 82 (variant 0), 83 (others) | 82: RM_3091, 7220/7221; 83: RM_5830, 5810, 7230/7231, 9302 |
| Alexia, third form (14; refused) | 269 `bhEff_E14_Fire` | 80 | as above |
| Alexia's baby (30; refused) | 267 `bhEff_E13_Fluid` | 81 | RM_5810, 9301, 9302 |

`spawn_glue.c fx_load` (after `sound_load` in `spawn_at`): for a type in the table whose texture
this room has not loaded (`sys->ef_tn[fx] == 0`), the entry is read from the source room
(`actor_glue.c actor_fx_tex`) and uploaded as `bhSetRoom` does (appended to `sys->ef_tlist`,
counted in `ef_extn`), so `bhReleaseMainTexture` releases it with the room's. Fallback
(`effect.c`): an effect whose room texture is not loaded gets the "not drawn" flag
(`flg 0x1000000`) instead of drawing texture 0, logged once per effect id.

## Enemy options

All off by default; off, the code returns before touching anything. In co-op the host's options
count for both. Code: `pc/glue/enemyopt_glue.c`, one decomp hook at the end of
`room.c bhFinishRoom` (`enemy_room_finished`).

| Setting (page) | ini key | Values | Test aid |
|---|---|---|---|
| "Extra enemies" (GAMEPLAY) | `enemy_extra` | 0 = Off, 1..3 = "Up to N per room" | `RECVX_ENEMY_EXTRA` |
| "Rearranged enemies" (GAMEPLAY) | `enemy_place` | 0/1 | `RECVX_ENEMY_PLACE` |
| "Enemy randomizer" (GAMEPLAY) | `enemy_random` | 0/1 | `RECVX_ENEMY_RANDOM` |
| "Enemy seed" (GAMEPLAY) | `enemy_seed` | seed for placements and randomizer | `RECVX_ENEMY_SEED` |

Every option first reads the room script once per load (`scan_script`) and marks records that an
event drives (`LoadWork`, motion pauses, links, positioning, zombie/face/look/render ops), that the
script shows or hides, whose HP the script tests or sets, and rooms with an "all dead" count.
Marked records are never changed. Choices come from a xorshift generator seeded with (seed, room
file, record), never from the game's `rand()`.

### Extra enemies

* "Up to N per room": at the first frame a room runs, the host (or single player) picks up to N
  of the room's unmarked, visible, living records of the roaming types zombie, Black Widow,
  Cerberus, Hunter, Bandersnatch, albinoid adult, anatomist, and spawns a copy of each from the
  current room file (`spawn_add`, so it reaches the guest as an added enemy): same type and model
  version (no upload), variant 0, placed 15..35 units from the source where a line test against
  the walls (`hitchkl.c bhCollisionCheckLine2`) is clear, at least 40 units from every entry
  position and from the player.
* Deterministic per (seed, room) and the room's living records. Extras have no kill bit: they come
  back with the room, and "all dead" counts ignore them.
* Budgets are the spawner's; a room that runs out stops adding.

### Rearranged placements

* At room load each unmarked record of a roaming type (as above plus bats) starts 8..30 units from
  its own spot or (half the time) another such record's spot on the same floor, in a direction
  with a clear line, 40+ units from every entry position, at a random angle. Kill bits, slots and
  types stay; a waiting Cerberus (variant 1/2) becomes variant 0.
* Not moved: story/cutscene/scripted enemies, bosses and set pieces, NPCs.

### Randomizer

* At room load, unmarked records of zombie, Black Widow, Cerberus, Hunter and Bandersnatch become
  other types from a pool: zombies (all 24 looks), Cerberus, Bandersnatch (both looks) from stage
  0, plus Black Widow and Hunters (both looks) from stage 2. Per room up to two new kinds.
* Slots are counted with what each Init spawns, up to 122 (6 stay free).
* Skipped rooms: rooms whose script counts kills (`DieEventCk`), rooms with a boss or set piece
  (ids 0, 2, 11..19, 23..25, 29, 30, 53..56). Fliers are not replaced.
* The record keeps its slot and kill bit: `spawn_at` realizes the new type into the record's own
  slot during the load.
* **Fallback**: any refusal (data not found, memory, textures) puts the original record back
  exactly as it was, logged as `record N stays`.
* Offline alternative: `pc/tools/rdxbuild.py` builds rooms with several `enemy_assets` donors
  ([modding/BUILDING_ROOMS.md](modding/BUILDING_ROOMS.md)).

### Limits

* Extra enemies come only from the host: in a room only the guest is in, there are none until the
  host comes in.
* Two looks of one type share the first one's sound bank.
* A refusal by `spawn_at` keeps the original record; both co-op sides decide the same unless one
  side's memory is fuller.

## Tools

`pc/tools/enemies.py` (Python 3, stdlib, imports `rdx.py`; reads the disc image at run time):

| Command | Output |
|---|---|
| `room ROOM` | the room's enemy records, floor areas and route boxes, and every script reference to each record with the conditions of its enclosing blocks |
| `census [--rooms]` | counts per type and room, kill flags, hidden/shown/event-driven records, helper conflicts, case variants, `gm_mode`/`ply_id` tests, floor areas, routes |
| `sources` | per (type, model version) the smallest room carrying its model and motions |
| `selftest` | the script walker on RM_0060 |

Test aids:

| Env var | Effect |
|---|---|
| `RECVX_SPAWN="gameframe:id[.mdlver][@dist],..."` | spawns the first spawner entry of that type/model version at that game frame; traces its state every 30 frames, HP changes and hits on the player |
| `RECVX_SPAWN_SURVEY="stg:rom,..."` | tries every spawner entry in each room and logs the result; `RECVX_SPAWN_SURVEY_ALL=1` keeps the spawns |
| `RECVX_ENEMY_EXTRA`, `_PLACE`, `_RANDOM`, `_SEED` | override the enemy options |
| `RECVX_ENEMY_TRACE=dist` | traces enemy steps (co-op targets) |
| `RECVX_TRACE_SND=1` | logs sound requests and extra banks |

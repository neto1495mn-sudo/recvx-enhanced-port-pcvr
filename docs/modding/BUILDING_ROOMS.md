# Building rooms

How to edit retail rooms and build new ones with the tools in `pc/tools/`, the contract of the
compiler `rdxbuild.py`, the two example areas, validation and headless testing. Binary layouts
are in [ROOM_FORMAT.md](ROOM_FORMAT.md); the mods folder and the legal rules in
[README.md](README.md). Everything built here contains data from your disc: keep it in your
ignored `mods/` and `scratch/` and do not share it.

## Workflow

```sh
export RECVX_ISO=/path/to/SLUS-20184.iso
export RECVX_MODS="$PWD/mods"                     # the game reads the same folder
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/linux-i686.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C build

# 1. Disc-free tests and a structural round trip of all 205 retail rooms
python3 -m unittest discover -s pc/tools -p 'test_*.py'
python3 pc/tools/rdxbuild.py roundtrip --all       # add --lz to recompress every room (slow)

# 2a. Edit a retail room in place (record counts stay the same)
python3 pc/tools/rdx.py dump 1110 -o scratch/1110.json
python3 pc/tools/rdx.py build 1110 scratch/1110.json -o mods/RM_1110.RDX

# 2b. Or build a room from source (example: the Service Corridor)
python3 pc/tools/rdx_demo.py scratch/corridor
python3 pc/tools/rdxbuild.py build scratch/corridor/room.json -o mods/RM_1150.RDX
printf '60\n' > mods/RM_1150.SPQ.key                         # use RM_0060's sound bank
python3 pc/tools/rdxbuild.py link 1110 0 --to 1:15:0 -o mods/RM_1110.RDX   # retarget a door

# 3. Inspect and test
python3 pc/tools/rdx.py script mods/RM_1150.RDX
python3 pc/tools/rdx.py obj mods/RM_1150.RDX -o scratch/corridor-check.obj
pc/tools/room_test.sh scratch/run 1:15:0 120
```

Stop the game while rebuilding; each tool replaces its output files atomically.

### Editing a retail room

`rdx.py dump` -> edit the JSON -> `rdx.py build ROOM edit.json -o mods/RM_xxxx.RDX` changes any
fixed-size record field. `"patches"` in the JSON (raw bytes at expanded-file offsets, as printed
by `rdx.py script`) change script operands. `rdx.py tex` / `teximport ROOM LIST INDEX x.png`
replace a texture of the same size with the nearest colours of its palette. Rules:

* Record counts stay the same (use `rdxbuild.py` to change them).
* Check the room's scripts for ops that rewrite the record you edit (37/123/124, 10/11, 36,
  179/180): most rooms set their door and item triggers every frame.
* Move an item together with its type 4 pickup trigger.

## Design guidelines

A room is a real-time 3D scene with separately authored cameras, collision, interaction zones,
actors and scripts. Room loading is the unit of asset ownership, the camera cut the unit of
composition; doors and scripts form the world graph. Start with a route, a decision, a reward
and the player's view of them, then build walls and visuals that support them.

Retail rooms per room (census over all 205 files):

| Measurement | Median | P90 | Maximum |
|---|---:|---:|---:|
| Camera cuts / switch areas | 5 / 8 | 9 / 18 | 14 / 31 |
| Collision walls / triggers | 49 / 10 | 89 / 18 | 168 / 27 |
| Camera-area span x / z, units | 89 / 96 | 180 / 160 | 427 / 325 |
| Room-model strip triangles / textures | 4,638 / 43 | 7,629 / 72 | 13,639 / 126 |
| Expanded file, bytes | 6.2 M | 8.3 M | 8.9 M |

A good first room: 2-4 cuts, 10-30 simple colliders, 1-2 enemies, 1-3 items, under 5,000
triangles and 16-32 textures. Most retail textures are 256x256 (8-bit: 66,560 bytes with CLUT);
prefer repeating floor/wall materials over unique textures per object.

* **Cameras.** Areas are half-open boxes or triangles; the current camera holds until all four
  ±1-unit probes leave its area, and in overlaps file order decides. Put the entrance landing
  and the first steps in one readable shot; switch after a landmark, not on an item zone. The
  common FOV is about 55 degrees, camera height above the floor median 25 units. Split ceilings
  and foreground walls into their own mesh groups so a shot can hide them; better, keep cameras
  inside the room under the ceiling and pitched down so no shot sees past the shell.
* **Scale and motion.** Claire: radius 3.5, height 16.5 (others 17.5). Walking 0.389
  units/frame (~11.7 units/s), running 1.053 (~31.6 units/s). With collision on, the player stops
  3.5 units from low boxes (up to ~10 high) and 5.5 from tall boxes and thin outer walls. Clear
  widths: 14-18 units for a tight passage, 24-40 for a combat corridor.
* **Collision is separate from visuals.** Every solid prop needs its own wall box; a shelf above
  head height should not block the floor. There is no walkable mesh: heights come from floor
  levels, stair/step triggers or (PC rooms) slopes.
* **Light and fog.** Start with the four ambient slots only; fog distances are negative (`0/-1000`).
* **Doors.** Retail door triggers are about 8 x 9 units. Keep the visible slab, the blocking wall
  and the action box separate; give every target an entry facing into the room, clear of
  collision, and maintain return edges explicitly. Door type 22 reuses a retail animation.
* **Items and enemies.** Item record, pickup trigger and script flag must agree; use one saved
  flag per pickup and per enemy, **below 512**, not used by retail scripts (see
  [ROOM_FORMAT.md "Event scripts"](ROOM_FORMAT.md#event-scripts)), and set enemy `wrk_no` = record index.
* **Locks.** Keep the trigger off or an examine trigger until a saved flag is set; `scd0` must
  restore the state on revisit.

## Source format (`rdxbuild.py`)

```text
rdxbuild.py [--iso ISO] decompile ROOM -o DIR       retail room -> room.json + preserved.bin (+ OBJ/PNG preview)
rdxbuild.py [--iso ISO] build room.json -o OUT.RDX  compile, validate, compress
rdxbuild.py [--iso ISO] link ROOM TRIGGER --to S:R:E -o OUT.RDX   retarget a door (record + scripts)
rdxbuild.py [--iso ISO] census -o OUT.json          statistics over all 205 rooms
rdxbuild.py [--iso ISO] roundtrip [--all] [--lz] [ROOM]   re-layout check
```

The portable input is **room.json + triangulated OBJ + opaque PNG** (stdlib only, no Blender
needed). `format` must be `recvx-room-1`. Record arrays (`room`, `cuts`, `walls`, `floors`,
`positions`, `lights`, `triggers`, `items`, `objects`, `enemies`, `routes`, `route_next`) use
the `rdx.py dump` field names; `rdxbuild.zero()` fills unused fields. Unspecified arrays are
empty in an authored room and inherited in a preserved one. `rdx_demo.py` is a complete example.
All paths are relative to the JSON.

| Field | Meaning |
|---|---|
| `budget` | `"retail"` (default) or `"pc"`, see [Budgets](#budgets) |
| `mesh` | OBJ of the room model; named `o`/`g` groups become nodes, materials `tex0`, `tex1`, ... |
| `textures` | ordered PNGs for the room model |
| `retail_textures` | instead of `textures`: `{"room":"5810","list":0,"index":20}` references, copied with all palettes |
| `gindex` | first global texture id of the authored textures (default `0x4D000000`) |
| `object_meshes` / `item_meshes` | one `{mesh, textures, gindex}` per object record after the 4 reserved / per item |
| `object_assets` | retail object models, e.g. `{"door":22}` = the retail door model and textures |
| `item_assets` | `{"room":"0060","record":2}` (floor model of a retail item), `{"inventory_item":21}` (inventory model) or `{}` (find a floor donor, else the inventory model) |
| `enemy_assets` | one `{"room":"0060","record":0}` per enemy record: model, motions and textures from that donor |
| `scripts` | hex blocks: entry (`scd0`), frame (`scd1`), then events; offsets are rebuilt |
| `preserved` | sidecar of a decompiled retail room (lossless unknown data) |

* **Enemies**: donor records may come from several rooms; the resolver replays the game's
  texture ownership and emits one motion set per type. `id`/`mdlver` must match the donor.
  Allowed combat types: zombies, Cerberus, Hunters, bats, Bandersnatches and anatomist zombies,
  variant 0; helper-code conflicts, model versions >= 16 and more than 112 child slots are
  rejected. A new animated creature needs a skeleton/motion/AI pipeline that does not exist.
* **Objects/items**: distinct ids within a class (textures are shared per id). Static props can
  instead be baked into the room mesh with a wall box, avoiding an object slot.
* **Re-layout**: the packer writes a fresh header, room block and record arrays, aligns sections
  to 64 bytes and relocates the model/motion/texture tables, so counts and sizes may change.
  `decompile` + `build` of a retail room is lossless; adding `mesh`/`textures` replaces the room
  model (and clears its camera hide masks). Messages and event cameras cannot be edited (their
  bytes survive); absolute `patches` are rejected (apply them with `rdx.py build` first).

### Coordinates

OBJ and JSON use game coordinates (y up, right-handed, ~10 units per metre). A Blender export
with "Y up, -Z forward" shows in game as in the viewport; an artist working in metres multiplies
by ten. UV v is inverted on import. Camera `ay`: the view direction is 180 degrees minus `ay`
(`32768` looks along +z), positive `ax` pitches down. Check an asymmetric test object first.

### Geometry and texture restrictions

* One hideable node per non-empty OBJ group, baked world transforms, face normals, one 3-vertex
  strip per triangle (identical vertices shared within a node). Triangles per group: 666
  (retail budget) / 2,729 (PC). One material + strip chunk per texture and node.
* **UVs must lie in 0..1**: the game clamps room textures, a UV beyond 1 smears the edge texel.
  Tile a material by splitting faces.
* No smooth normals, vertex colours, hierarchy, animation or skinning from OBJ.
* PNG: RGB/RGBA 8-bit, non-interlaced, **opaque**, power-of-two sizes 8..512. Quantized to an
  RGB332 palette, written as 8-bit TIM2 with FormatId 1 and a private CLUT; black is stored as
  (0,0,1) so it does not turn transparent. Transparency and 4-bit output need `retail_textures`.
* Gindex collisions within a room are rejected; there is no cross-mod allocator, so keep
  original textures in a chosen high-id range.

### Budgets

`room.json` `"budget"` selects one. **`retail`** keeps a room within what the PS2 build's memory
holds. **`pc`** uses the limits the PC build raised ([ROOM_FORMAT.md "Limits and
budgets"](ROOM_FORMAT.md#limits-and-budgets)); such a room needs this build (an older build
overruns the arena silently). `rdxbuild.py` holds them in `BUDGETS`; `RECORD_LIMITS` apply to both.

| Check | `retail` | `pc` | Engine limit behind it |
|---|---:|---:|---|
| Expanded file | 9 MiB | 44 MiB | ~10 MB / ~74.6 MB free arena on arrival |
| Packed + expanded | (not checked) | 64 MiB | both copies are in the arena while loading |
| TIM2s in the room | 192 | 768 | 256 / 1024 texture entries |
| TIM2 payload | 6 MiB | 40 MiB | 10 / 64 MB texture heap |
| Triangles per mesh group | 666 | 2,729 | vertex work buffer 5120 / 8192, <= 8189 vertices per chunk |
| Mesh groups | 512 | 65,535 | u16 node count; hide masks cover nodes 0..511 |
| Cameras / areas per camera | 255 / 255 | same | byte operands |
| Walls | 32,767 | same | scripts and co-op reach 0..255 |
| Triggers, floors | 256 | same | byte compares in scripts |
| Entry positions / routes | 128 / 255 | same | |
| Lights / event lights / effects | 256 / 128 / 256 | same | |
| Enemies / objects / items | 128 / 32 / 32 | same | runtime slots |
| Player stands on slopes | no | yes (`ROM_WORK.flg` 0x100) | |

The retail budget's limits are deliberately below the PS2's real limits. Persistent player
data, effects and enemies also use the arena and texture heap; measure headroom in game with
`RECVX_SPAWN_SURVEY=STAGE:ROOM` (texture entries, heap and arena free). `rdx_bigroom.py OUT
[--grid N] [--size S] [--textures T]` builds a disc-free stress room (default 106,602 triangles,
629 walls, 300 textures); 254k triangles have loaded and collided correctly in the PC budget.

### Slopes

Only in `"budget": "pc"` rooms: the player (and zombies) stand on **type 6 slope walls**. On the
PS2 only some enemies use them. Records (in `walls`):

| Field | Slope |
|---|---|
| `flg` | 1 |
| `type` | 6 |
| `id` | direction the surface rises: 0 `+x`, 1 `-z`, 2 `+z`, 3 `-x` |
| `px, pz, w, d` | box `px <= x < px + w`, `pz <= z < pz + d` |
| `py`, `h` | height at the low edge and the rise; `h > 0` (0 means the room's wall height); flat patch: `h` 0.001 |
| `flr_no`, `attr`, `prm0..3` | `flr_no` a floor in `grand[]`; `attr`, `prm` 0 |

Height at x/z: `py + h * s` with `s` = `(x-px)/w` (id 0), `1-(z-pz)/d` (1), `(z-pz)/d` (2),
`1-(x-px)/w` (3). Helpers in `rdxbuild.py`: `slope_height(rec, x, z)`, `slope_wall(x, z, w, d,
low, high, rise)` (`rise` `'+x' '-x' '+z' '-z'`), `heightfield_slopes(heights, x0, z0, cell)`
(one slope per grid cell along its steeper axis). `rdx_slopes.py OUT` builds a test room (ramp,
plateau, hill), e.g. as `RM_1280` (teleport `1:28:0`, or `1:28:1` on the hill top).

1. **Which slope**: among the slopes whose box holds x/z and whose band `py - ah .. py + h + |ah|`
   holds y (`ah` ≈ 17), the surface nearest to the current y. Boxes may overlap (a bridge over a
   ramp). The player steps onto a slope from any side from as low as ~17 below; fence sides
   that must not be climbed with ordinary walls.
2. **Outside every slope** the player is on its floor level. A ramp must end at a level height
   or another slope. For terrain, cover all walkable ground with slopes (flats included) on
   **one floor** (`grand[2]` = lowest ground, `grand[3..]` 0), so cut areas, triggers and floor
   records stay on floor 0.
3. **Seams**: a slope is planar, so a twisted patch leaves steps at the seams (cross gradient x
   cell size). The player crosses small steps without snagging; smaller cells reduce them. Test
   movement both ways across seams, including downhill onto lower patches.
4. **Entrances and items** inside a slope: author `py` on the surface.
5. **Collision is separate**: slopes do not push. Buildings and fences are ordinary walls whose
   `py..py+h` band covers the terrain height there.
6. **Budget**: slopes are wall records (put them after the walls scripts change); each costs a
   loop iteration per actor per frame, a few thousand are fine.
7. **Enemies** moving with `bhCheckWall` use the PS2 projection on the same records (they drift
   slightly); zombies get the player's rule. Not yet tested with enemies in a sloped room.
8. Shadows read slope surfaces; the camera follows the player's y.

### World wiring, sound and map screen

* `rdxbuild.py link ROOM TRIGGER --to S:R:E` changes the type-0 trigger record, every
  `EtcAtariSet2` for that trigger and `SetDoorCall`s with the old target. Conditions and
  `RoomCaseNo` stay; inspect the disassembly. It refuses scripts that do not disassemble cleanly.
* Room case is script state (`RoomCaseNo`) and is not reset by doors; teleports keep the current
  case. Test new rooms from a case-0 context and both story states of patched retail rooms.
* Sound: `mods/RM_xxxx.SPQ.key` with a retail key (e.g. `60`) loads that room's whole bank,
  including its enemy sounds. No new sound effects.
* Map screen: a new room has no map data; opening the map there is untested and may be unsafe.

## Example: Service Corridor

`rdx_demo.py` generates a 40x100-unit technical blockout: two cameras (areas z 0..52 and
48..101), entry (20,0,10) facing +z, return door to RM_1110 entry 0 (animation 22), a cabinet and
an original crate prop with collision boxes, one handgun-ammo pickup (it_flg 506) and one zombie
from RM_0060 record 0 (ed_flg 500). Build it with the commands in [Workflow](#workflow). Its door
patch retargets RM_1110's piano-room door, so do not install it together with the annex.

## Example area: Quarantine Annex

Six original rooms on stage 1, rooms 16-21, case 0, built from `pc/tools/area/annex.json` by
`build_area.py`. Geometry and materials are procedural (stdlib PNGs: concrete, steel, tile,
wood, rust, hazard stripes, stencil signs); enemies come from your disc.

```text
Retail RM_1110 piano room <-> [116 Reception] <-> Retail RM_1020/1021 hall
                                  |     |
                            [117 Wash] [121 Dispatch]
                                  |     |
                            [118 Records] <-> [119 Isolation]
                                                    |
                                              KEY [120 Supply]
```

```sh
python3 pc/tools/build_area.py               # needs RECVX_ISO; --source-only: assets only, no disc
```

Outputs: `mods/RM_1160.RDX`..`RM_1210.RDX`, patched `RM_1110.RDX`, `RM_1020.RDX`,
`RM_1021.RDX`, six `.SPQ.key` files (RM_1210: 3020, others 60), `mods/quarantine-validation.json`
and the generated source in `mods/quarantine-source/`. Uninstall: remove those nine RDX and six
key files.

| Room | Teleport | Contents | Pickups (id / it_flg) | Enemies (ed_flg) |
|---|---|---|---|---|
| 1160 Reception | `1:1:16:0` | counter, benches; safe hub | 12 / 460, 21 / 461 | none |
| 1170 Decontamination | `1:1:17:0` | basins, lockers | 12 / 462, 21 / 463 | 2 zombies (460, 461) |
| 1180 Records Office | `1:1:18:0` | desks, shelves; token key | 72 / 464, 21 / 465 | 2 zombies (462, 463) |
| 1190 Isolation Gallery | `1:1:19:0` | cages, beds; locked Supply door | 12 / 466, 21 / 467 | zombie + Cerberus (464, 465) |
| 1200 Emergency Supply | `1:1:20:0` | shelves, pallet; safe reward | 12 / 468, 22 / 469 | none |
| 1210 Dispatch Bay | `1:1:21:0` | crates, trolleys | 12 / 470, 21 / 471 | 2 Cerberus (466, 467) |

Item ids: 12 handgun ammo, 21 green herb, 22 red herb, 72 Key with Tag. Zombies: RM_0060 record 0;
Cerberus: RM_3020 record 2, variant 0.

Doors (all animation 22; in the new rooms trigger N = arrival entry N; S/N/W/E = -z/+z/-x/+x wall):

| From / trigger | To S:R / entry | Condition | Landing x, z |
|---|---|---|---|
| RM_1110 / 0 | 1:16 / 0 | retail piano-exit logic | 32, 11 |
| RM_1020, RM_1021 / 9 | 1:16 / 1 | retail unlock (ev_flg 147) | 11, 40 |
| 1160 / 0 S, 1 W, 2 N, 3 E | 1:11/0, 1:2/3, 1:17/0, 1:21/1 | open; W restores the hall case and door attr | 66,7.5; -6.5,7.4 (y 40); 28,11; 11,50 |
| 1170 / 0 S, 1 N | 1:16/2, 1:18/0 | open | 32,69; 32,11 |
| 1180 / 0 S, 1 E | 1:17/1, 1:19/0 | open | 28,89; 11,50 |
| 1190 / 0 W, 1 E, 2 N | 1:18/1, 1:20/0, 1:21/0 | E needs it_flg 464 | 53,40; 11,40; 36,11 |
| 1200 / 0 W | 1:19/1 | always open | 61,50 |
| 1210 / 0 S, 1 W | 1:19/2, 1:16/3 | open | 36,89; 53,40 |

* **Lock**: only this pickup's it_flg 464 opens Supply (the frame and entry scripts disable
  Isolation trigger 1 and re-enable it when 464 is set); a retail Key with Tag does not. The
  TOKEN sign is the only cue (no message). Supply's exit is never locked.
* **Retail integration**: RM_1110 trigger 0 and four script literals point to Reception, its
  `RoomCaseNo 1` becomes 0; the hall's trigger 9 script targets (2 in RM_1020, 4 in RM_1021)
  point to Reception entry 1; story locks stay. Reception's west door repeats RM_1110's own hall
  exit (case and door attr by ev_flg 151/181/142), so the detour returns to the right hall case.

## Example area: village from retail assets

`build_village.py` composes a compact village (an interpretation of a rural village square) from
retail Code: Veronica assets only: four rooms plus an extra door in the graveyard (RM_0020). No
other game's files are used and no retail event changes.

```sh
python3 pc/tools/assets.py inventory --previews --geometry   # optional catalogue in scratch/assets/
python3 pc/tools/assets.py search wood
python3 pc/tools/assets.py item 21
python3 pc/tools/build_village.py
python3 pc/tools/village_validate.py
```

Outputs: `RM_1220.RDX`..`RM_1250.RDX`, patched `RM_0020.RDX`, four `.SPQ.key` files (`60`),
`village-validation.json` in `mods/`; editable generated room JSON/OBJ in `scratch/village/`.
Uninstall: remove the five RDX and four key files.

* **Assets**: `assets.py` catalogues every room's TIM2s (all palettes), model nodes, objects,
  items, collision, the door packs in `MRY.AFS` and inventory models (item ids 1-144);
  `kitbash.py` places retail nodes, deduplicates textures and gives solid props AABB collision.
* **Rooms**: square 160x180 inside a 50-unit fieldstone wall (houses, barn, pyre with three
  looping retail flame effects, north gate), house ground floor (loft blocked), barn, church
  approach with graves and a composed church front. Eight zombies (RM_0060 record 0), pickups
  ammo/green herb, it_flg 480-486, ed_flg 480-487.
* **Cameras**: closed shells; every camera inside below the shell top, pitched down at least
  half the vertical view (`pers` 13000), at least 35 units from the billboard flames. Cuts come
  from a coverage search and are checked by `village_validate.py`.
* **Graveyard patch**: original records stay byte-identical prefixes, scripts and motions
  unchanged; the new door model is inserted before the item assets (+84 KB expanded, 3 texture
  entries).

| File | Teleport | Contents |
|---|---|---|
| RM_1220 | `1:1:22:0` (entries 0-3) | village square: houses, pyre, 3 zombies, ammo + herb |
| RM_1230 | `1:1:23:0` | house ground floor: 1 zombie, ammo + herb |
| RM_1240 | `1:1:24:0` | barn: 2 zombies, ammo + herb |
| RM_1250 | `1:1:25:0` | church approach: 2 zombies, herb |
| RM_0020 (patched) | `1:0:2:2` (new entry) | graveyard with the extra east-wall door |

| Source / trigger | Door (x, z) | To S/R/E | Landing (x, z) |
|---|---|---|---|
| Graveyard / 5 (new) | 178,88 (examine from 171,93 facing east) | 1/22/0 | square 80,10 |
| Square / 0 south | 80,0 | 0/2/2 | graveyard 168,93 |
| Square / 1 west house | 32,66 | 1/23/0 | house 32,10 |
| House / 0 | 32,0 | 1/22/1 | square 32,76 |
| Square / 2 east barn | 128,108 | 1/24/0 | barn 36,10 |
| Barn / 0 | 36,0 | 1/22/2 | square 128,98 |
| Square / 3 north gate | 80,180 | 1/25/0 | path 32,10 |
| Path / 0 | 32,0 | 1/22/3 | square 80,170 |

From a new game, reach the graveyard and use the new door on its east wall behind the
gravestones, before or after the rising-zombie event; the existing graveyard doors are unchanged.

## Validation

| Check | Command |
|---|---|
| Compiler, area, asset and OBJ tests (disc-free) | `python3 -m unittest discover -s pc/tools -p 'test_*.py'` |
| Re-layout of all retail rooms | `python3 pc/tools/rdxbuild.py roundtrip --all [--lz]` |
| Room statistics | `python3 pc/tools/rdxbuild.py census -o scratch/census.json`, `rdx.py census` |
| Annex | built into `build_area.py` (`area_validate.py`), report in `mods/quarantine-validation.json` |
| Village | `python3 pc/tools/village_validate.py [--mods DIR] [--source DIR]` |

`rdxbuild.validate_authored` rejects textures over budget or with colliding Gindex, ids outside
the runtime tables, enemy `wrk_no` != record index, scripts that do not disassemble, persistence
flags >= 512, records on floors outside `grand[]`, malformed slopes and rooms without a camera
or an entrance. The area validators add closed shells, navigation on a 1-unit grid with the
measured clearances, reachable interaction probes, reciprocal doors, camera coverage of feet and
head, flags checked against all retail scripts, and memory estimates. Static checks do not
replace a game run.

## Headless room testing

`pc/tools/room_test.sh OUT STAGE:ROOM:POS [SECONDS]` starts a new game headless (offscreen video,
dummy audio, movies skipped), teleports to the target and writes `OUT/frames/frame_*.bmp` (every
30th presented frame), `OUT/log.txt` (look for `mods:`) and `OUT/save/` (its own save directory).
The new game is started by frame ~300 and the teleport lands around frame 330 (later on a loaded
machine). The target may be a teleport list such as `'1:17:0,1500:1:16:0,150:1:17:0'` (each
later entry: frames after the previous one, then stage:room:entry).

| Variable | Meaning |
|---|---|
| `RECVX_ISO` | disc image (default `ISO/SLUS-20184 ROM.iso`) |
| `RECVX_MODS` | mods folder (default `./mods`) |
| `RECVX_BIN` | binary to run (default `build/pc/recvx`), e.g. another build to compare |
| `MORE_INPUT` | extra `RECVX_INPUT` entries, `frame:buttons` (`560:r1,600:r1+cross,605:-`) |
| `SIZE` | window size (default `320x240`) |
| `RECVX_WARP` | `stage:room:frame:x:z:ay[,...]`: place the player at room-local game frame |
| `RECVX_NOCLIP` | `frame:flags`; `1:0` keeps collision and logs the position every 15 game frames |
| `RECVX_EQUIP`, `RECVX_INF_AMMO`, `RECVX_INF_HEALTH` | equip `frame:item:count` (`20:5:60` = handgun, 60 rounds), unlimited ammo, survive attacks |
| `RECVX_TRACE_SND` | log sound bank uploads |
| `RECVX_SPAWN_SURVEY` | `stage:room`: log free texture entries, heap and arena, then exit |
| `RECVX_DUMP_EVERY` | frame dump interval |

The full list of test aids is in [../ARCHITECTURE.md](../ARCHITECTURE.md). Input times are
presented frames, warp times room-local game frames; interaction probes sit 2 units in front of
the player. Example: kill the corridor zombie, then come back and check it stays dead:

```sh
shots=$(python3 -c "print(','.join('%d:r1+cross,%d:r1'%(f,f+5) for f in range(600,1300,24)))")
RECVX_INF_HEALTH=1 RECVX_NOCLIP=1:0 RECVX_EQUIP=20:5:90 RECVX_WARP=1:15:10:20:30:32768 \
MORE_INPUT="560:r1,$shots,1300:-" pc/tools/room_test.sh scratch/enemy '1:15:0,900:1:11:0,150:1:15:0' 600
```

Acceptance per room: loads by teleport and by its doors (both directions, correct file and
landing), every camera cut both ways with the player in view, collision against every wall and
prop, pickups taken and absent on revisit, enemies attack, die and stay dead on revisit, the
`mods: room sound key` line, and a minute or more in the room without errors. Compare a rebuilt
retail room with the disc's under identical input (`RECVX_BIN`, frame dumps).

## Known limits

* Not supported: new sound effects or banks, map screen entries (the map may be unsafe in new
  rooms), new inventory text (pickups show the retail item's name), message and event-camera
  editing, a symbolic script assembler, transparent or 4-bit authored textures, smooth normals,
  new animated creatures, a Blender exporter.
* Untested: save/reload of new pickup/kill flags, audible sound in new rooms (bank uploads are
  traced only), alternative characters, co-op in new rooms, enemies on slopes.
* Room IDs of the examples overlap (`RM_1150` corridor, `RM_1280`/`RM_1290` test rooms vs. the
  RE4 rooms): install one set at a time.

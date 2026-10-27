# Modding

The PC port reads replacement and additional game files from a **mods folder**. Rooms can be
edited, cloned or built from scratch with the Python tools in `pc/tools/`. This directory
documents the formats and the tools:

| Document | Contents |
|---|---|
| [ROOM_FORMAT.md](ROOM_FORMAT.md) | room file (`RM_xxxx.RDX`) format, load path, records, event scripts, limits |
| [BUILDING_ROOMS.md](BUILDING_ROOMS.md) | building rooms with `rdxbuild.py`, the example areas, validation, headless testing |
| [RE4_VILLAGE.md](RE4_VILLAGE.md) | converting an RE4 village export into rooms |

Related: [../ACTORS.md](../ACTORS.md) (models, skeletons, motions, enemy records),
[../ENEMIES.md](../ENEMIES.md) (enemy scripts, kill flags, budgets).

## Legal

* The repository contains **no game data**. Every tool reads your own copy of the disc (and,
  for the RE4 tools, your own RE4 export) at run time.
* Mods are built **locally** from your disc and exports. Generated room files, extracted
  textures, models, previews, catalogues and sidecars contain game-derived data: **do not
  share, upload or commit them**. This applies to rooms built from purely original geometry as
  well when they import retail enemies, items, doors or textures, which all the example areas do.
* What can be shared: tool source code, JSON recipes and original procedural assets that
  contain no extracted bytes (for example `pc/tools/area/annex.json` and the generators).
* The RE4 tooling needs **your own RE4 export**; its outputs (rooms, reports, previews) are
  derived from both games and are likewise not for distribution.
* Keep build output in the ignored `mods/` and `scratch/` directories of your clone.

## The mods folder

`pc/src/audio/adx.c` implements it; small `#ifdef TARGET_PC` hooks in the decomp ask it first.

* Location: `$RECVX_MODS`, default `mods` relative to the **working directory** the game is
  started from (not the save directory). Names are case-sensitive on Linux; the game asks for
  upper-case names.
* The log shows `mods: <path> replaces the disc file (<size> bytes)` for every file it uses.
* Files are read as from the disc (whole 2048-byte sectors, zero padded).

| Mods file | Replaces | Notes |
|---|---|---|
| `RM_SRRC.RDX` | room S (stage 0..9), RR (room 00..99), C (case 0..9) | replaces the disc's room, or **adds** a room the disc does not have; packed (LZ) like the disc files. Hooks: `ps2_sg_gd.c gdFsGetDirInfo` (size), `adxwrap.c OpenAfsIsoFile` (open) |
| `RM_SRRC.SPQ.key` | room sound bank key | text file with one decimal retail key, e.g. `60` = RM_0060's bank. Log: `mods: room sound key 1150 uses 0060`. Not recursive; an invalid file is ignored |
| `<ARCHIVE>/<index>` | entry *index* of an AFS archive opened by partition, e.g. `MRY.AFS/76` | sound banks, door animations, map files, player/weapon files, streamed ADX music and voices (ADX, at most 2 channels). Only ADX streams are tested in game |
| `<NAME>` | a plain disc file opened through `ADXF_Open` | |

Not covered: files the game reads with `ReadFileEx` (`MULTSPQ?.IDX`, `SYSMES.ALD`,
`COMMON.MLT`).

A room key is `stage*1000 + room*10 + case` written as four digits (`RM_1150.SPQ.key` for
RM_1150). Without an alias a new room has no sound bank of its own and the previous room's
banks stay loaded.

To uninstall a mod, remove its files from the mods folder. Rooms are never saved, so edited
rooms apply to existing saves the next time the room loads.

## What can be modded

| Change | How | Status |
|---|---|---|
| Records of an existing room (positions, items, enemies, walls, triggers, cameras, lights, fog, heights) with the same counts | `rdx.py dump` -> edit JSON -> `rdx.py build` | works |
| Script operands (door targets, flags) | `"patches"` in the `rdx.py build` JSON, offsets from `rdx.py script`; `rdxbuild.py link` for doors | works |
| A texture, same size and palette | `rdx.py tex` / `rdx.py teximport` | works |
| Record counts, sections, new geometry and textures | `rdxbuild.py decompile` / `build` (OBJ + PNG + JSON) | works |
| A cloned room in a new slot | copy to an unused `RM_xxxx.RDX`, edit, point a door at it | works |
| New original rooms connected to the game | `rdxbuild.py`, `build_area.py`, `build_village.py` | works, see [BUILDING_ROOMS.md](BUILDING_ROOMS.md) |
| New sound effects, map screen entries, inventory text, new animated creatures | | not supported |

A clone keeps everything in the file, including the scripts' flag numbers (give it its own
`it_flg`/`ed_flg` bits); it loses the room sound bank (use a `.SPQ.key` alias), its map entry and
the per-room tables in the executable.

## Tools

Run from the repository root. The disc image is taken from `--iso`, `$RECVX_ISO` or
`ISO/SLUS-20184 ROM.iso`.

### Room tools

| Tool | Purpose |
|---|---|
| `pc/tools/rdx.py` | room files: `list`, `dump`, `build`, `extract`, `pack`, `script` (disassembler), `tex`, `teximport`, `obj`, `census`, `grants` (items scripts hand over), `selftest` |
| `pc/tools/rdxbuild.py` | authoring compiler: `decompile`, `build`, `link`, `census`, `roundtrip` |
| `pc/tools/rdx_demo.py OUT` | generates the original Service Corridor source (no disc data) |
| `pc/tools/rdx_slopes.py OUT` | generates a slope test room (no disc data) |
| `pc/tools/rdx_bigroom.py OUT` | generates a large PC-budget stress room (no disc data) |
| `pc/tools/assets.py` | retail asset catalogue: `inventory [--previews] [--geometry]`, `search`, `item` |
| `pc/tools/kitbash.py` | module: bakes retail MLB nodes into authored room geometry |
| `pc/tools/build_area.py` | builds the Quarantine Annex (6 original rooms); `--source-only` without the disc |
| `pc/tools/build_village.py` | builds the village from retail assets (4 rooms + graveyard door) |
| `pc/tools/area_validate.py` | module: offline checks of the annex (used by `build_area.py`) |
| `pc/tools/village_validate.py` | offline checks of the village (`--iso`, `--mods`, `--source`) |
| `pc/tools/enemies.py` | enemy census: `room`, `census`, `sources`, `selftest` |
| `pc/tools/analyze_obj_surfaces.py` | measures an external OBJ (orientation, heights, materials) |
| `pc/tools/room_test.sh` | headless new game + teleport with frame dumps and log |
| `pc/tools/test_rdxbuild.py`, `test_area.py`, `test_assets.py`, `test_obj_surfaces.py` | disc-free unit tests |
| `pc/tools/area/annex.json` | annex design: rooms, flags, links |
| `pc/tools/area/village_assets.json` | hand-written semantic tags for `assets.py search` |
| `pc/tools/area/re4_materials.json` | RE4 material -> retail texture mapping |
| `pc/tools/re4/` | RE4 village conversion ([RE4_VILLAGE.md](RE4_VILLAGE.md)) |

### Developer tools (not modding)

| Tool | Purpose |
|---|---|
| `pc/tools/asm2c.py` | translates the decomp's inline EE/VU0 asm into C |
| `pc/tools/vsm2inc.py` | converts VU microcode listings into tables for `pc/src/ps2/vu.c` |
| `pc/tools/decomp_warnings.py` | regenerates the decomp warning lists in `docs/dev/` |
| `pc/tools/profile.py` | summarises a `RECVX_PROFILE` sample file |
| `pc/tools/compare_frames.py` | per-frame diff and montage of two frame dump directories |
| `pc/tools/compare_native.sh` | runs the emulated and native renderer headless and compares |
| `pc/tools/layout_check.sh` | determinism check of two binaries or layouts |

### Python requirements

* Python 3.9 or newer. `rdx.py`, `rdxbuild.py`, `assets.py`, `kitbash.py`, the generators,
  `build_area.py`, `build_village.py`, the validators and the tests use the **standard library
  only**.
* `pc/tools/re4/`: NumPy; the previews, contact sheets and several surveys also need Pillow;
  `slope_engine.py` needs a C compiler (`cc`).
* `compare_frames.py` / `compare_native.sh`: OpenCV (`cv2`) and NumPy.

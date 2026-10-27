# RE4 village conversion

`pc/tools/re4/` converts the geometry of Resident Evil 4's opening village into Code: Veronica
rooms for the PC port: a square prototype, five connected rooms and one whole-map room on the PC
budget. Textures are **not** taken from RE4: every material is mapped to a native Code: Veronica
texture from your disc. Room format and compiler: [ROOM_FORMAT.md](ROOM_FORMAT.md),
[BUILDING_ROOMS.md](BUILDING_ROOMS.md).

**Not for distribution.** The tools need your own RE4 export and your own Code: Veronica disc.
Everything they produce (room files, recipes with source coordinates, surveys, previews,
reports) is derived from both games: keep it in your clone's ignored `scratch/` and `mods/`,
and do not share, upload or commit it. Only the tool code and the material mapping are tracked.

## Inputs

| Input | Form |
|---|---|
| RE4 village geometry | **your own export** as a Wavefront OBJ (+ MTL), source units, Y up, triangles or polygons. Materials are named `ROOM_MATERIAL_NNN` (the keys of the material mapping); groups are the source's model parts. Only the OBJ geometry is used; source textures and other export files are not needed |
| Code: Veronica disc | `RECVX_ISO` or the recipe's `iso` (texture, door, item and zombie donors) |
| Asset catalogue | optional, `python3 pc/tools/assets.py inventory --previews` (for donor search) |
| Material mapping | `pc/tools/area/re4_materials.json` (tracked) |
| Group overrides | optional, `scratch/re4/group-overrides.json` or `--group-overrides` (source group names stay untracked) |
| Transform | `scratch/re4/transform.json` for the square tools: `scale`, `source_origin` |
| Recipes | JSON files under `scratch/` (required location): `obj`, `iso`, `mapping`, `overrides`, `scale`, `output`, `exclude_materials`, per-room `origin`/regions, links, pickups, enemies |

The square scripts read the OBJ path from `RE4_VILLAGE_OBJ`; the room and whole-map tools from
the recipe's `obj`. Generated output is confined to `scratch/` (the tools refuse other paths)
and installed rooms go to `mods/`. Requirements: Python 3, NumPy, Pillow (previews, surveys),
a C compiler for `slope_engine.py`.

## Materials

`re4_materials.json` maps each source material to a `class`, a `retail_texture` (`room`,
`list`, `texture`, zero-based; optional `uv_rect` for an atlas region), `review_required` and
`review_reasons`. A `classes` table gives the default donor per class:

| Class | Donor room:list:texture | Notes |
|---|---|---|
| ground | 5810:0:20 | seamless ground tile |
| wood | 0120:0:26 | planks |
| stone | 7150:0:29 | fieldstone |
| plaster | 0030:0:72 | weathered grey render |
| roof | 0030:0:18 | dark slate |
| foliage | 1100:0:4 | broadleaf cutout, native alpha |
| ceramic | 0030:0:6, `uv_rect` | tableware atlas region; provisional |
| metal | 5810:0:1 | opaque dark metal |
| opacity | 1000:0:22 | iron cutout, binary alpha |
| water | 1040:0:11 | opaque surrogate; not used |

`materials.py` resolves a use with the precedence group/material override, group `*`, then
material; a class override selects that class's donor; an explicit `null` donor excludes the
use. Unknown materials fall back to a logged provisional roof (sloped and high) or wood. The
build copies native TIM2 payloads with all palettes and alpha (never via preview PNGs) and
refuses an opaque-class donor whose texture has transparent texels. The sky shell and the
map-wide alpha overlay layers are excluded; horizontal alpha decals are dropped rather than
made opaque. All mappings keep their review flags.

Donor search helpers: `find_foliage.py CATALOGUE`, `select_donors.py`, `contact_donors.py`,
`contact.py`, `review_materials.py OBJ --materials ...` (isolated material renders),
`analyze_obj_surfaces.py OBJ --grid-size N [--mtl MTL]` (orientation, heights, role hints).

## Axes, scale and partition

* Y up, no axis reflection; uniform scale about **0.86** after translating by the source
  origin. Calibration: a door-sized source panel against retail door type 22 (20 units high,
  9 wide; `door_reference.py`), with the player 17.5 units tall. This keeps Code: Veronica's
  door/player ratio; RE4's own ratio would mean a scale of about 0.97.
* Do not infer scale from the whole map's bounding box.
* Rooms are partitions of the map with a bounded walk region each, conservative collision from
  the source mesh (`components.py` lists connected solid components), and searched fixed
  cameras. Visual stairs do not create walkable heights; upstairs areas stay inaccessible.
* Mesh per room: visibility culling for the fixed cuts plus vertex clustering (the finest cell
  under the triangle budget). Changing cameras means regenerating the visible selection.

## Pipeline

### 1. Square prototype (RM_1260)

```sh
export RECVX_ISO=... RE4_VILLAGE_OBJ=...      # your disc image and your RE4 export
python3 pc/tools/re4/components.py            # solid components -> scratch/re4/components.json
python3 pc/tools/re4/build_square.py [--group-overrides FILE]
python3 pc/tools/re4/check_square.py          # walk samples, cameras, budgets
python3 pc/tools/re4/ray_check.py             # head visibility against the mesh
python3 pc/tools/re4/render_square.py         # software camera previews + overview
python3 pc/tools/re4/publish_square.py        # validate, install mods/RM_1260.RDX (+ SPQ key 60)
pc/tools/re4/play_square.sh NAME SECONDS [teleport]   # room_test.sh with a fixed vanilla 4:3 recvx.ini
python3 pc/tools/re4/contact_square.py scratch/re4/play/NAME FRAME...   # in-game frame vs preview per cut
```

A rectangular walk area in the centre of the square, seven footprint blockers, six cuts
(3 x 2 tiles, cameras at height 30 looking at the facades), under 12,000 triangles. The
publisher archives a previous `mods/RM_1260.RDX` and checks that the ID is not retail.

### 2. Navigation evidence (optional)

Surveys that measure the source without installing anything:

| Tool | Output |
|---|---|
| `navigation.py OBJ --mapping M --transform T --bounds X0 Z0 X1 Z1 --entry X Z [--cell] [--radius]` | reachable flat ground, solid cells, open edges |
| `outline.py SURVEY [--reduce] [--tolerance N] --output F` | compact candidate collision boxes |
| `validate_outline.py SURVEY OUTLINE --output F` | clearance and connectivity audit of an outline |
| `boundary_heights.py OBJ SURVEY OUTLINE --mapping --transform --output` | terrain beyond open boundaries |
| `fit_transitions.py EVIDENCE --output F` | how well one incline fits each opening |
| `inspect_region.py OBJ --bounds ... --output F` | source cross-sections for reviewing openings |

Missing geometry is never turned into a wall; unsupported edges stay explicit transition
requirements.

### 3. Connected rooms (RM_1260-RM_1300)

```sh
python3 pc/tools/re4/build_rooms.py RECIPE.json          # bounded rooms, collision, cameras
python3 pc/tools/re4/connect_rooms.py MANIFEST.json --publish   # links, pickups, zombies, install
```

`connect_rooms.py` stages all builds, checks clear return entries, reachable trigger zones and
placement clearance, imports retail type-22 door models, ammo/herb floor models and RM_0060
zombie assets, gives each room sound key 60, and allocates pickup and kill flags 490-494 against
the retail flag census (`retail_flags` in the manifest, from `build_area.census`).
`graveyard_link.py` appends a second graveyard door next to the village door, keeping every
existing record, script and motion byte. A passage uses animation 255 and no door model.
`build_regions.py` / `publish_regions.py` and `preview_rooms.py` are an alternative region
builder and its previews.

| Room | Role | Teleport | Return entries |
|---|---|---|---|
| RM_1260 | square | `1:1:26:0` | 1 from paths, 2 from graveyard |
| RM_1270 | paths | `1:1:27:0` | 1 square, 2 barn, 3 house, 4 approach |
| RM_1280 | western building (barn role) | `1:1:28:0` | 1 from paths |
| RM_1290 | northern house, ground floor | `1:1:29:0` | 1 from paths |
| RM_1300 | church approach lane | `1:1:30:0` | 1 from paths |
| RM_0020 | graveyard (patched) | `1:0:2:0` | 3 from the RE4 square; entry 2 still serves the village of [BUILDING_ROOMS.md](BUILDING_ROOMS.md) |

Each room has one retail pickup and one zombie that stay gone after pickup/kill. Doors into the
barn and house are type 22 at the source building openings; square <-> paths and square <->
graveyard are passages.

### 4. Whole-map PC room (RM_1310)

One `"budget": "pc"` room with the uncropped village: teleport `1:1:31:0`. The graveyard's RE4
door is retargeted to RM_1310 entry 1 (three operands; return entry 3); RM_1260-RM_1300 stay as
they are.

```sh
python3 pc/tools/re4/build_whole.py RECIPE.json [--publish-inspection]  # mesh, nodes, transform
python3 pc/tools/re4/survey_whole.py RECIPE.json [--cell 2]             # height-aware survey
python3 pc/tools/re4/terrain_patches.py RECIPE.json                     # axis ramps on source corners
python3 pc/tools/re4/terrain_pc.py RECIPE.json                          # PC slope records, movement audit
python3 pc/tools/re4/collision_whole.py RECIPE.json                     # static walls + support-edge walls
python3 pc/tools/re4/cameras_whole.py OUTPUT_DIR --dense-rays           # fixed cuts over the survey
python3 pc/tools/re4/finish_whole.py RECIPE.json --publish              # contents, checks, install
python3 pc/tools/re4/validate_whole.py RECIPE.json                      # audit of the compiled room
python3 pc/tools/re4/render_whole.py OUTPUT_DIR [--candidate-cameras] [--cuts N...]
```

Further evidence tools: `audit_terrain_seams.py`, `joint_terrain.py`, `refine_terrain.py`,
`continuous_terrain.py` (coverage certificate), `movement_whole.py` and `slope_engine.py`
(extracts `pc_slope_ground` from `hitchk.c`, compiles it with `cc` and replays movement with C
float arithmetic over the compiled slope records).

Contents: about 130k triangles in 217 nodes (shell parts first, vegetation last; hideable nodes
below 512), native retail textures, 93 cuts, six pickups and five zombies. Terrain is about
11,500 type-6 slopes (flat patches included) on one floor (`grand[2]` below the lowest ground);
collision is about 3,900 static boxes plus invisible support-boundary walls that keep actors on
authored terrain. Packed about 6 MB, expanded about 18 MB. Distant vegetation is hidden per
camera so translucent primitives stay within the ordering table, with 4,096 reserved for actors
and effects.

`finish_whole.py` publishes only after its gates pass: quarter-unit coverage of the walk area
by slopes without gaps, seam steps below 0.25 units, source height error, closure of every
boundary wall, bidirectional movement through the compiled slope function, camera coverage and
framing, placement clearance, native asset budgets, compression round trip and unchanged
fallback rooms. For a later revision, archive the previous publication and refresh
`final-before.json` (the installed baseline) first; the graveyard retargeter expects the
historical RM_1260 destination.

## Validation

```sh
python3 -m unittest discover -s pc/tools/re4 -p 'test_*.py'    # synthetic tests, no source data
```

Offline checks cover entry clearance, connected walking space, camera coverage and feet/head
frusta, sampled head visibility against the mesh, source ground support, geometry and texture
budgets, TIM2 FormatId 1, byte-identical native textures and packed-file round trips. Each tool
writes its report next to its output and records input hashes, so stale evidence is detectable.

In game, check per room: every cut in both directions, the walk perimeter (collision on), feet
and head clearance, doors and passages both ways with clear landings, pickups and the zombie
(and that both stay gone on revisit), status screen, leave/re-enter, a few minutes in the room,
and headroom with `RECVX_SPAWN_SURVEY`. Use a fresh Claire game, case 0, enemy options off.

## Limits

* The square prototype passed in game; the connected rooms and RM_1310 are validated offline
  only, in-game acceptance is pending.
* Materials are substitutes from Code: Veronica's texture set: one roof donor, provisional
  ceramic/metal, per-face cutout UVs on foliage cards, no water.
* Walking is confined to the surveyed region; upper floors, ladders, interiors beyond the two
  ground floors and the church interior are not built. Some exterior crops show sky at the edges.
* No RE4 scripting, enemies, items or sounds; zombies and pickups are Code: Veronica's.
* Room IDs RM_1280/RM_1290 collide with the `rdx_slopes.py`/`rdx_bigroom.py` test rooms.

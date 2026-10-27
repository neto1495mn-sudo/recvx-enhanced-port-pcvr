# Third-person mode

An optional over-the-shoulder camera with mouse/keyboard movement and free aim, in the style
of RE4/RE5. Off by default. With it off the game behaves exactly as without it: every decomp
hook is `#ifdef TARGET_PC` + `if (tps_on)`; the logic lives in `pc/glue/tps_glue.c` (game
thread), `pc/src/host/tps.h`, `pc/src/input/tps_input.c` and the F1 overlay (settings, mouse).

## Usage

Toggle with **F5**, the `--third-person` command-line flag, or `tps = 1` in
`<save dir>/recvx.ini`. It can be switched at any time; it only takes over in normal play.
While it is on the mouse is captured (F1/F2/F3 free it).

| Key | Action |
|---|---|
| W / S | walk forward / back (back + Shift = quick turn, as in the original); with "Game auto-aim" also aim up / down while the weapon is raised, as the original's up / down |
| A / D | turn |
| Shift | run |
| mouse | X turns the character, Y looks up/down |
| right mouse (hold) | ready weapon; the mouse now aims (camera closer, narrower FOV, crosshair) |
| left mouse | fire while aiming; otherwise action like E |
| E | action / examine / confirm |
| Space (hold) | quick knife: knife drawn and raised, left mouse slashes ([USER_GUIDE.md](USER_GUIDE.md)) |
| Q | cancel |
| Tab | status screen (inventory) |
| M | map |

Arrows, Enter, Backspace, Z/X, 1/2 and gamepads keep working. Any key bound to a third-person
row stops pressing its DualShock 2 button while third person is on (by default W/A/S/Q lose
R1/Square/Triangle/L1). All third-person keys, a quick-turn key (unbound by default) and the
toggle key can be rebound in F1 -> Controls or OPTION -> ENHANCED SETTINGS -> CONTROLS (ini
`key_tps_*`: `tps_forward`, `tps_back`, `tps_left`, `tps_right`, `tps_run`, `tps_action`,
`tps_cancel`, `tps_inventory`, `tps_map`, `tps_aim`, `tps_fire`, `tps_quickturn`,
`tps_toggle`; the toggle key's first slot is also `tps_key`).

### Mouse/keyboard camera (A/B)

Setting "Mouse/keyboard camera" (THIRD PERSON page, ini `tps_free_camera`):

* **B (behind character)**, the default: the camera stays behind the character and the mouse
  turns it (the table above).
* **A (free)**: the mouse orbits the camera around the character (pitch within +-60 degrees,
  walls as in B) without turning it. W/A/S/D walk towards that direction on screen
  (diagonals too) and the character turns to it smoothly (0.3 of the angle per game frame);
  Shift runs. Aiming turns the character to the camera direction (free aim: at once, through
  the crosshair as in B; with "Game auto-aim": towards the camera unless the game locks on);
  after aiming the camera stays where it looked. The quick-turn key does the game's quick
  turn (S + Shift runs towards the camera). Arrow keys and the gamepad keep tank controls;
  while they steer, the camera follows behind as in B.

Doors, events and cutscenes put the camera behind the character again in both. A/B are this
option's own letters, not the game's KEY CONFIG types. The gamepad has no third-person camera
(no right-stick camera).

### Settings

All on the THIRD PERSON page (F1 or OPTION -> ENHANCED SETTINGS). Lengths are in tenths of a game
unit (the character is about 17 units tall).

| Setting | ini key | Default |
|---|---|---|
| Over-the-shoulder camera | `tps` | 0 |
| Field of view (horizontal over the 4:3 picture; widescreen adds to the sides) | `tps_fov` | 75 |
| Field of view aiming | `tps_aim_fov` | 65 |
| Camera distance | `tps_distance` | 140 |
| Camera distance aiming | `tps_aim_distance` | 100 |
| Camera height | `tps_height` | 165 |
| Shoulder offset | `tps_shoulder` | 35 |
| Left shoulder | `tps_left_shoulder` | 0 |
| Mouse sensitivity X / Y (percent; 100 = 0.12 degrees per mouse count) | `tps_sens_x`, `tps_sens_y` | 100 |
| Aiming sensitivity (percent of the above) | `tps_aim_sens` | 70 |
| Invert Y | `tps_invert_y` | 0 |
| Crosshair | `tps_crosshair` | 1 |
| Game auto-aim (0 = free aim) | `tps_auto_aim` | 0 |
| Mouse/keyboard camera (0 = B, 1 = A) | `tps_free_camera` | 0 |
| Light and fog (1 = smooth, 0 = per cut) | `tps_atmos` | 1 |
| Toggle key (SDL scancode) | `tps_key` | 62 (F5) |

### Test aids

| Env var | Effect |
|---|---|
| `RECVX_TPS_MOUSE="frame:dx:dy,..."` | moves the mouse dx/dy counts per game frame from that presented frame on |
| `RECVX_TPS_TRACE=1` | logs the free aim per frame, each shot's direction and wall hit against the crosshair target ("SHOT"), the drawn weapon's barrel with "You appear as", the camera ("tps: cam A\|B"), the aim height given to the game; `=2` also keeps the camera on the enemy the game locked on to (the middle of its hit capsule, or `RECVX_TPS_AIM_AT=f` along it: 0 = `c1`, for a zombie the head, 1 = `c2`, its legs) |
| `RECVX_TPS_ORBIT=degrees` | turns only the view about the player (side views; aim unchanged, camera ignores walls) |
| `RECVX_ATMOS_TRACE=1` | logs each game frame's cut, the game's fog, lights and filter and what "smooth" shows |
| `RECVX_EFF_TRACE=1` | logs effects spawned and queued |

## How the original camera works

* **Cuts** (`cut.c`): each room has `rom->cut_n` camera cuts (two versions `cam[camver]`).
  `bhCheckCut` (game.c `bhMainSequence`, after drawing) picks the cut whose floor area contains
  the player; `bhSetCut` loads its camera (`cam.px/py/pz`, `ax/ay/az`, `ln`, `pers`), fog, filter,
  the per-cut **hidden room parts** (`hidobj` -> `NJS_CNK_OBJECT.evalflags |= 8`) and hidden
  lights (`hidlgt`). `bhControlActiveCamera` follows the player inside tracking cuts. Rooms with
  `ts_flg & 0x200` also change the clip range per cut.
* Event cameras (`st_flg & 1`), first person and the sniper scope (`gm_flg & 0x40`,
  `st_flg & 0x800000`) and door transitions (`cb_flg & 1`) are separate paths. First person is the
  precedent for a free camera: it un-hides all room parts and sets `njClipZ(-1, -20000)`.
* **View** (`camera.c bhControlCamera`, before drawing): `njSetPerspective(cam.pers)` and
  `cam.mtx = T(0,0,-ln) Rz(az) Rx(ax) Ry(ay) T(-(p + of))`, plus the view direction `cam.v*` and
  eye `cam.wp*`. Everything 3D uses `cam.mtx`: drawing, billboards, model culling, 3D sound.
* **Projection** (`ps2_NaView.c`): `_nj_screen_.dist = 320 / tan(ang/2)` (horizontal FOV over the
  4:3 screen); culling cone, VU1 projection and clip volume follow (widened by `pc_hor_scale()` in
  widescreen). A different camera needs no change there.

## How the player moves, turns and aims

* `pad.c bhSetPad` (decomp) maps the DualShock 2 bits through the key type table into virtual bits
  `sys->pad_on/pad_ps`: `0x1` forward, `0x2` back, `0x4`/`0x8` turn, `0x10` ready weapon,
  `0x20`/`0x40` aim up/down, `0x100` fire, `0x200` examine, `0x400` run, `0x1000` cancel,
  `0x2000` map, `0x4000` status.
* `player.c bhControlPlayer` runs tank controls; turning adds a fixed rate to `plp->ay`
  (65536 = 360 degrees; forward = `(-sin ay, 0, -cos ay)`).
* Raising a weapon calls `bhSearchNearEnemy` (auto-aim: `plp->ayp`, `flg |= 0x100000`, `ay` turns
  to it over `yrct` frames). Aim pitch is three-way (45 degrees; `at_flg` 2 straight, 4 up,
  8 down), and enemies read it when they take damage ("Aim height" below). Firing casts a ray from
  `(px, py + 11, pz) + forward * (3 + r)` along `Rx(wax) Ry(ay)` against enemy capsules
  (`weapon.c bhCheckGunAtari`); the wall test (`bhCheckL2Wall`) uses the same direction from the
  muzzle on hand node 9 (about 3 units higher). Setting `ay` and `wax` towards the crosshair target
  therefore makes the game's own ray hit it.

## What hides room parts

* Third person draws every room part while its camera is active (`ps2_NinjaCnk.c
  pc_show_hidden`). Room records with `flg 0x40000000` (drawn only in the cuts in `hide[0..3]`)
  are drawn too (`objitm.c`). Model culling uses the active view.
* Rooms are modelled for their camera angles: from elsewhere some walls are missing, single-sided
  or untextured. The camera is kept inside the collision walls (`bhCheckL2Wall`): the shoulder
  point is pulled in sideways if it is in a wall, the eye slides forward along the view ray in
  front of the first wall behind the shoulder.

## Design

* **The cut system keeps running** (`bhCheckCut` unchanged), so doors, triggers and per-cut
  lights/fog keep working. A hook in `game.c bhMainSequence` before `bhControlCamera()` writes our
  camera into `cam.px/py/pz, ax, ay, az, ln, pers`, so `cam.mtx`, `cam.v*`, `cam.wp*` and 3D sound
  agree. Not applied with an event/fixed cut, first person/scope, door transitions or events
  (`cb_flg & 5`, `gm_flg & 9`); when it stops, `gm_flg |= 0x800` re-initialises the current cut
  and the room's clip range is restored.
* **Frozen cut camera**: messages, examine texts, pick-up prompts and menu fades clear
  `sys->sp_flg 0x40`; our camera stays active and holds still until it is back.
* **The cut camera underneath**: `bhCheckCut` leaves the cut's camera in `cam`, and the next
  frame's game logic reads it (gore sprites towards `-cam.v*`, `bhEff305/306` towards `cam.wp*`,
  3D sound). `tps_after_cut` (after `bhCheckCut`) puts our view back and clears `gm_flg 0x20`
  (the cut change that wipes lens blood).
* Enhanced-mode interpolation: `pc_scene_id` ignores cut changes while our camera is active.
* Hooks: `bhSetPad` (third-person keys and mouse buttons as virtual bits); `bhControlPlayer` after
  the mode handler (mouse yaw into `plp->ay`; while aiming `ay`/`wax` towards the crosshair target,
  auto-aim turn cancelled); its `bhCalcModel(plp)` replaced by `tps_calc_model` (free-aim pose).
* Camera A (`g_tps.free_cam`): `tps_pad` turns W/A/S/D into "forward" in move mode; `tps_player`
  turns `ay` towards camera yaw minus the keys' angle (not in the game's own turns or special
  moves, `mode2 > 13`); `tps_camera` adds the mouse X to the camera yaw. Pose, aim and co-op read
  `ay`/`wax`, never the camera.
* Camera collision never feeds back into the aim: the crosshair ray starts at the wall-free
  shoulder point, and the camera position depends only on the player's position.
* Mouse: SDL relative mode while third person is on and no overlay window is open; deltas are
  consumed once per game frame.

## Light and fog

With the fixed cameras each cut's atmosphere changes exactly when the picture cuts. With the
third-person camera the cut still follows the player underneath, so light and fog would
switch while walking.

**What a cut sets** (`cut.c bhSetCut`, `CAM_WRK` of `rom->cutp[ncut].cam[camver]`; counts over
all 205 rooms, camera version 0, from `pc/tools/rdx.py`):

| Setting | Where it acts | Rooms whose cuts differ |
|---|---|---|
| fog on/off (`flg & 8`) | `sync.c bhControlEOR` | 31 |
| fog colour (`fog_col`) | `njSetFogColor` | 110 |
| fog range (`fog_nr`, `fog_fr`, linear in view z) | `njGenerateFogTable3` | 172 |
| camera-relative fog (`flg & 0x40` near, `0x80` far): range shifted by eye minus cut camera base, every frame | `bhControlActiveCamera` | 82 |
| light set: lights 4.. hidden per cut (`hidlgt`); the first directional and first three point lights that are on fill the VU1 slots | `light.c bhControlLight` | 181 |
| point lights only if on screen (`flg & 0x10`) | `bhControlLight` | 9 |
| screen filter (`flg & 0x8000`, `fil_no`, `fil_rt`) | `effsub1.c bhEffFil` | 62 |
| full-screen distortion (`flg & 0x400`, `bhEff110`) | | 2 |
| `ts_flg & 0x200` rooms: view clip, background colour per cut | `event.c`, `ps2_event.c` | (third person keeps its own clip) |

Room ambient and lights 0..3 (muzzle flash, lighter, ...) do not change per cut.

**Smooth** (default; ini `tps_atmos` 1; 0 = per cut). `tps_atmos` (`game.c` after
`bhControlLight`) and `tps_filter` (`bhEffFil`) take the game's per-cut values as the target:

* A change in a frame where the cut changed (`cam.ncut`/`camver`) fades; any other change
  (scripts, flicker, muzzle flash, event fog) applies at once. Lights 0..3 are never faded.
* Lights: a weight per room light, 0 -> 1 in 24 game frames (0.8 s), applied to the values the
  game computed this frame. A point light fading in waits for a free slot (at most three in use);
  meanwhile the outgoing ones fade twice as fast (a short dip). The on-screen test of
  `flg & 0x10` is not applied.
* Fog: density fades for on/off; range and colour ease (8% per game frame); the colour follows the
  denser fog faster. The range is measured from our camera (the camera-relative shift otherwise
  made fog come and go as the camera turned).
* Screen filter: alpha fades over 0.8 s. Not faded: the distortion of 2 rooms, the per-cut
  background colour of `ts_flg & 0x200` rooms.
* When our camera stops, lights are recomputed and the fog is re-applied (`gm_flg 0x10`).

## Effects (blood, hits)

* Creation does not depend on the camera (`bhEne01_SetBlood`, `bhEne_SetBlood*`,
  `bhSetGunSplash`). Drawing uses `cam.mtx`; the camera-dependent parts are the cut camera values
  seen by game logic (`tps_after_cut`) and the lens blood (`bhEff301`, follows our camera).
* Culling: `njDrawTexture3DEx` drops a quad with a vertex behind the camera or all vertices
  off screen (`_Clip_Screen`); its window is widened by `pc_hor_scale()` like the clip volume,
  so effects in the widescreen margins are drawn. Particles and the ordering table cull nothing.

## Free-aim pose

* The weapon motions (file `wpnr_no + ply_id*30 + 20`, motions 100+: 100 raise, 101 fire, 104
  hold, 116 reload, up/down at +5/+10) lack up/down variants for some weapons. Free aim keeps the
  straight pose (`tps_pad` clears the up/down bits) and `tps_calc_model` runs `bhCalcModel`,
  turns the world matrices of nodes 2 (spine) and 3 (chest) by half the pitch each about the
  model's x axis, marks both `O_WORK flg 1` and runs `bhCalcModel` again, so arms, head, hair and
  weapons follow. Node angles are untouched (no feedback into the motion blend). The bend eases in
  and out (0.35 per frame).
* The pitch is measured from the weapon hand (node 9) of the unbent pose to the target. Muzzle
  flash, knife hits and attachments use the bent matrices; the shot ray is the game's (`wax`).
* **Shots on walls** (`aim_at_crosshair`): enemies are aimed from the enemy-test origin (11 units
  up); when the crosshair target is a wall, the aim starts at the muzzle, so wall hits land on the
  crosshair.
* **"You appear as" another model** ([ACTORS.md](ACTORS.md) "You appear as"): the weapon is drawn
  in the model's hand, so the bend's pitch is taken from there (`actor_aim_offset`: drawn minus
  real hand, in the frame of the real chest). The shot stays the real skeleton's
  (`actor_hands_back`). With the retargeting ([ACTORS.md](ACTORS.md) "Retargeting") the pitch is
  then solved on the weapon as it will be drawn (`actor_aim_hand`: the drawn hand's position and
  rotation in the frame of the real chest, which the bend turns): up to four steps of bend, measure
  the barrel's pitch against the line from its muzzle to the crosshair target, correct. The bend
  swings a hand far from the chest (a tall model's) off the line the unbent pose measured; the drawn
  barrel now stays within about 0.35 degrees of the target (before: up to 1.5 degrees on a far wall,
  11 on a near ceiling). The camera's height, shoulder offset and distance scale with the model's
  head height (`actor_cam_scale`). Nothing changes without a model.
  `actor_aim_hand` is the hand as drawn in the previous tick, so the solve runs only while the aim
  pose stands still: the weapon raised (`mode2` 65) for two ticks without a motion blend, never
  for the knife (it has no barrel). Its correction to the hand's pitch eases in (0.35 per frame)
  and is kept through raising, shots, recoil and slashes. Solved every tick on a moving hand, it
  bent the upper body back and forth from one tick to the next (the twitch when raising a weapon,
  firing or slashing).
* Sniper rifle and linear launcher switch to the game's scope (first person); no bend.

### Aim height

The enemies' damage code reads the player's aim height (`plp->at_flg & 0xE`), not only where
the shot went: zombies and the anatomist lying on the ground (`en01.c`/`en26.c`, `exp0 + 0x44 &
0x40`), a knocked-down Hunter (`en05.c`, motions 37/38) and Cerberus and Bandersnatch in one of
their states (`en04.c`, `en09.c`) take no damage from a straight shot (the bow gun and the
grenade, rocket and linear launchers excepted); a shotgun or grenade launcher blows a zombie's
head only aimed up at close range. In the original you aim down at a crawling zombie. Free aim
kept the straight pose and with it the straight aim height, so its hits on lying or crawling
zombies were thrown away.

* `aim_at_crosshair` sets the aim height from the free aim after the mode handler: up when the
  shot rises more than 22.5 degrees (half the original's 45), down when it drops more than that or
  goes at anything lower than 6 units above the player's floor (a crawling zombie, legs, the
  floor), else straight. The enemies read it on their next step (`bhControlEnemy` runs before
  `bhControlPlayer`).
* `tps_player_mode` (a hook before the mode handler in `bhControlPlayer`, and when third person
  stops) puts it back to straight, so the weapon modes keep the straight motions and the shot
  (`bhCheckGunAtari` copies `at_flg`) keeps the straight test; an enemy that cleared `at_flg`
  meanwhile (grabs) is left alone.
* With "Game auto-aim" the game's own aim height plays: W / S press the up/down bits as the
  d-pad does (`0x21` / `0x42`).

## Limits

* **Room geometry**: some corners have no wall, ceiling or back side; looking there shows the
  clear colour. With the back to a wall the camera slides forward up to the shoulder, so the
  character can leave the picture (view and aim stay put).
* Aim poses: free aim bends the straight pose up to 60 degrees of pitch. No laser sight.
* The crosshair goes down to 60 degrees: from the shoulder (16.5 units up) it meets the floor
  about 9.5 units ahead, so a crawling zombie nearer than that cannot be put under it (the shot
  goes at the floor behind it; the game's wide hit test may still reach it).
* Co-op: enemies judge a hit with the aim height of the player they are after; the partner's
  stand-in carries none, so every hit counts on an enemy after the partner.
* Near walls: a wall closer than 40 units on the crosshair ray counts as 40 units away for the
  character's yaw/pitch (else the shoulder offset would turn it by tens of degrees); the shot
  then hits a little beside the crosshair. Enemies, and walls farther away, are hit exactly.
* The crosshair marks the screen centre. Target selection uses enemy hit capsules and the
  room's collision walls, not every visible prop.
* Mouse turning rotates the character directly (no turning animation while standing). Not
  while climbing stairs/ladders, pushing or in other special moves.
* Events, cutscenes, door transitions, first person/scope and menus use the original camera;
  so does an event without its own camera (`mode0 == 7`, rare).
* Messages, examine texts, pick-up prompts and the fades to the inventory, map, files and
  options keep the third-person view; the camera holds still while the game is frozen.

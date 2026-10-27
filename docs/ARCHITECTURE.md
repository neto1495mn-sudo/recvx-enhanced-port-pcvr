# Architecture

How the PC port is put together. For building see [BUILDING.md](BUILDING.md); for the
feature-specific internals see [COOP.md](COOP.md), [SERVER.md](SERVER.md),
[ACTORS.md](ACTORS.md), [ENEMIES.md](ENEMIES.md), [THIRDPERSON.md](THIRDPERSON.md) and
[modding/](modding/README.md).

## Overview

The game code (gameplay, AI, collision, menus) **and** the original PS2 platform layer are
compiled from the [recvx-decomp](https://github.com/AshfordFamily/recvx-decomp) decompilation.
The decomp's `ps2_*.c` files implement Sega's Ninja/Shinobi API on PS2 hardware: they build DMA
chains and VIF/GIF packets and run VU microcode. Instead of rewriting that layer (and risking
behavioural drift), the port keeps it and emulates what lies underneath, at the hardware and
library boundaries:

```
game logic (decomp)  ->  Ninja/Shinobi on PS2 (decomp ps2_*.c)
                           |-- inline EE/VU0 asm  -> translated C (asm2c) on a VU0 model
                           |-- vcallms            -> VU0 microcode (static C)
                           |-- sceDmaSend/CHCR    -> pc/src/ps2/dma.c (chain walker)
                                 |-- VIF1 -> vif.c -> VU1 microcode (static C) -> XGKICK
                                 |-- GIF  -> gs/gs.c (PATH1/2/3) -> gs_gl.c (OpenGL)
                           |-- libcdvd/libpad/libmc/SIF RPC -> host implementations
```

On top of that, native paths replace the hottest parts of the emulated pipeline (see
[Renderer](#renderer)), and the PC features (co-op, third person, model swaps, enemy options,
settings menus) hook into the game through small glue functions.

### Source layout

| Path | Contents |
|---|---|
| `decomp/` | the decomp as a submodule (fork, branch `pc-port`). PC changes are inside `#ifdef TARGET_PC` blocks: asm replacements, PS2-address idioms, hooks into the glue, and fixes of undefined behaviour, with the PS2 original kept under `#else` |
| `pc/include/` | `pc_prelude.h` (force-included into every decomp file: host headers, `long` as 64-bit, declarations of all PC hooks) and replacement SDK headers (`sdk/`) |
| `pc/glue/` | PC code that needs the decomp's types: co-op, lobby, trade screen, third person, model swaps, cheats, enemy options and spawner, native renderer hooks, ENHANCED SETTINGS menu, mouse in menus, quick knife, native saves |
| `pc/src/ps2/` | EE/IOP layer: kernel, DMA, VIF1, VU0/VU1 (static microcode and interpreter), libcdvd, libpad, libmc, SIF, the address window |
| `pc/src/gs/` | GS emulation (GIF parser, register file, local memory), the OpenGL rasterizer, the frame stream, native textures, in-between frames |
| `pc/src/audio/` | CRI ADX (ADXF/ADXT), SDL audio output, the TSNDDRV sound driver (high-level emulation) and its synthesizer |
| `pc/src/movie/` | PSS demuxer, MPEG-2 decoder, movie player |
| `pc/src/input/` | DualShock 2 emulation (`pad.c`), bindings, third-person input, mouse in menus |
| `pc/src/host/` | entry point (`host.c`), render loop, settings table and `recvx.ini`, native save files (`savestore.c`), F1/F2/F3 overlay (Dear ImGui), logging, profilers, model catalogue |
| `pc/src/net/` | co-op UDP, packet formats, port mapping, rendezvous client and protocol |
| `pc/server/` | `recvx-server` and the `recvx-client` test tool |
| `pc/tests/` | ctest programs |
| `pc/tools/` | code generators (`asm2c.py`, `vsm2inc.py`), room and map tools, test and comparison scripts |

The build (`pc/CMakeLists.txt`) has three parts: `recvx_game` (the decomp's `prog/*.c` minus a
few excluded files, plus `pc/glue`), `recvx_platform` (`pc/src`) and the `recvx` executable.
The decomp is compiled with flags that keep the PS2's arithmetic and layout assumptions:
`-fsigned-char`, `-fwrapv`, `-fno-strict-aliasing`, `-msse2 -mfpmath=sse -ffp-contract=off`
(IEEE single precision, no FMA, PS2 operation order), `-ftrivial-auto-var-init=zero`, and on
Windows `-mno-ms-bitfields`.

## Data model and memory

- **32-bit build.** Disc data contains 32-bit pointers that are relocated in place, and the
  platform code stores host pointers in 32-bit DMA tag fields. A 64-bit build would need those
  structures to keep 32-bit pointer slots; it is not planned before the 32-bit port is complete.
- **EE `long` is 64-bit**: `pc_prelude.h` includes the host headers, then
  `#define long long long`.
- **Address window**: the executable is linked at `0x30000000` (Linux `-Ttext-segment`, Windows
  `--image-base` with ASLR off), so the EE's uncached/accelerated aliases (`addr | 0x20000000`,
  `addr | 0x30000000`) are identities and DMA tag addresses fit in 31 bits. The 16 KB scratchpad
  is mapped at its real address, `0x70000000`, because the platform code builds packets at
  literal scratchpad addresses.
- **The window is 256 MB**: all static game memory (room arena, texture heap, spawn pool, ...)
  is the image's `.bss` and must end below `0x40000000`; `ps2/memory.c` checks this at start.
  Host allocations (SDL, OpenGL, caches) can live anywhere. The PC raises several PS2 budgets
  within this window (see [ENEMIES.md](ENEMIES.md) and
  [modding/ROOM_FORMAT.md](modding/ROOM_FORMAT.md#limits-and-budgets)).
- String literals are writable (the game writes into them; the EE has no MMU protection).
- File paths are UTF-8 everywhere (`pc_fopen` in `host/fileio.c` uses `_wfopen` on Windows; the
  Windows build rebuilds `argv` from the UTF-16 command line). Settings are saved by writing a
  temporary file and renaming it over the old one.

## Threads and shared state

| Thread | Owns |
|---|---|
| main (`main()`, `render_loop`) | SDL window and events, OpenGL, GS emulation and presentation, the F1/F2/F3 overlay, binding capture, lobby text input and clipboard, the setting appliers (window, vsync, volumes) and **every `recvx.ini` write** |
| game (`game_thread`) | the decomp and `pc/glue` (ENHANCED SETTINGS menu, lobby, co-op incl. its UDP socket polled once per frame, cheats applied at the frame hand-off), disc reads, memory card, creating and destroying audio streams |
| SDL audio | ADX decoding in the stream callbacks (own file handle per voice), the sound driver's 240 Hz tick, its queued commands and the synth mix |
| portmap | UPnP/NAT-PMP while hosting (`net/portmap.c`) |
| co-op arrival times (Windows) | waits on the co-op socket and notes when data arrived (`net_rx_us`) |
| log (Linux, `--log`) | copies stdout/stderr into the log file |

The game runs single-threaded as on the PS2: interrupts are delivered synchronously at platform
entry points (vblank waits, sync calls), and timing follows NTSC rates.

How state crosses threads:

- **Settings** (`g_config`, `g_tps`, `g_coop` options, cheat toggles, bindings) are aligned
  ints written by both menus; a reader sees the old or the new value. A change is handed over
  with `settings_set`/`settings_save_later` (release store of a dirty flag); `settings_poll` on
  the main thread takes it with acquire, runs the applier and writes `recvx.ini` 500 ms after
  the last change and at exit. Nobody else calls `config_save`.
- **Commands** from the overlay to the game are request words (`g_cheat.request`,
  `g_coop.request`, `g_coop.trade_req`), taken by the game thread at its frame.
- **Audio**: one lock in `audio.c` covers a stream's publication, gain and removal. The sound
  driver (`snd_iop.c`, `snd_synth.c`) has its own lock over the driver state, the banks and the
  command queue the game thread fills and the mixer empties.
- **Co-op status** strings are written by the game thread and read by the overlay without a
  lock; every string keeps a terminating 0, so a read may show a half-updated line but stays
  bounded.

## Renderer

### Frame stream

The game thread runs the decomp's main loop, the VU code and the GIF parser, and only
**records** each frame's GS command stream (register writes, image uploads, display buffer
changes; `gs/stream.c`). At the buffer swap the frame is queued for the main thread, which
executes it against the GS emulation and OpenGL and presents it in step with the display
(`host/render.c`). At most one frame is in flight. A GS readback (`StoreImage`) is a sync
point: the partial frame is executed and the game waits for the pixels.

### GS emulation

- GS local memory is exact: all pixel-storage swizzles, CLUT buffer and CLD semantics,
  transfers.
- Rasterization is OpenGL 3.3 at `scale` times the native resolution, with one render target
  per framebuffer address. Colours are stored as RGBA16F (rgb = C/255, a = A/128), so GS blend
  factors up to ~2.0 map directly to GL blending. A texture whose base is a render target samples
  that target, so render-to-texture effects (for example `PS2_swap`'s frame blend) work
  unchanged.
- Textures are decoded from GS memory and cached per TEX0/CLUT, invalidated by page write
  generations.

### Widescreen (hor+)

Render targets get GS-pixel margins on both sides. The game's clip matrix
(`_Make_ClipMatrix`) and CPU culling cone (`Ps2CalcScreenCone`) are widened by `pc_hor_scale()`
while the projection is untouched, so 3D fills the margins and 2D drawn at x 0..639 stays 4:3 in
the middle. Model culling is widened the same way (`fNaViwHalfW`, the joint window of
`bhCheckClipModel`/`bhCheckClipPoint`). Room cameras hide room parts they cannot see (`hidobj`
lists, authored for 4:3); in widescreen such a part is drawn anyway when its bounding sphere lies
entirely outside the 4:3 view, while hidden parts inside it (occluders) stay hidden
(`pc_show_hidden`). Full-width scissors, fills and render-target copies are extended into the
margins.

### Native paths

The emulated pipeline (VU1 microprograms -> GIF -> GS -> OpenGL) is replaced by native paths
where it pays off. Each is switchable at run time (F1 -> Display, `RECVX_NATIVE3D`,
`RECVX_NATIVE_TEX`, `RECVX_NATIVE_RTEX`), and `pc/tools/compare_native.sh` diffs the paths
headless.

- **Native 3D** (`pc/glue/native_glue.c`, `gs/stream.c`, `gs_gl.c`): chunk-model strips
  (`njCnkCsUvh/Uvn`) no longer go to VU1. The per-vertex results of the VU1 programs (colour,
  ST/Q, fog, back-face culling of single-sided strips) are computed on the CPU; the position is
  passed homogeneously, exact to the VU0 `CALCPOINT` formulas, and the GPU projects. Triangles
  are clipped against near/far on the render thread, as the VU1 `SCISSOR` program does.
- **Ordering-table primitives**: the CPU strip functions (`ps2_Vu1Strip.c`) and the general
  builder `Ps2AddPrim` (2D polygons, effects, particles) produce native records too. Their GS
  packets keep the render state but carry, instead of vertices, a write of the record number to
  GS register 0x7F (`NATIVE_MARKER_REG`), so each record is drawn exactly where the GS would have
  drawn it in the sorted order. Sprites, lines, points and the full-screen operations of
  `ps2_dummy.c` (fades, copies) still take the GS path.
- **Native textures** (`pc/glue/texture_glue.c`, `gs/textures.c`): the game re-uploads the
  whole TIM2 to its single GS texture slot on every switch. Each TIM2 content is recorded once as
  a definition and bound on switches. CLUT uploads are executed for real; image uploads are
  deferred and written into GS memory only before something reads or overwrites them. Memory is
  capped (definitions 96 MB, decoded textures 256 MB). Texture identities are kept in an 8-way
  set-associative table.
- **Render textures on the GPU**: the game's only GS readback is `StoreRenderTex` (full-screen
  distortion, the stage-7 small screen, `Ps2TextureMalloc` textures). The game thread records a
  capture event instead; the render thread blits the render-target area into a texture that
  becomes a native texture definition, and the TIM2 only receives a 16-byte marker. A stale marker
  falls back to a plain upload. In widescreen the full-screen distortion captures the widened view
  (`pc_rtex_wide`) and draws it back over the whole view. The other small screen (the Palace
  monitor, `PS2_Render_Tex_Sub`) is a GS copy into FBP 476 sampled as a texture; a target's drawn
  area is its scissor cut to the vertices of 2D batches, so a CLUT upload in the pages below that
  copy does not make it stale.
- Still emulated: the GS register/state model, GS memory (render-to-texture, CLUTs, non-native
  textures), render targets, VIF/DMA for the remaining packets.

### Enhanced mode: in-between frames

Game logic, physics, animation and audio stay at the original 30 Hz: they are frame-stepped
throughout the game code, so running them faster would change gameplay. Instead, the renderer
shows in-between images at the display rate:

- The engine tags every 3D mesh it draws (`njCnkDrawModelLocal[Mod]` -> `pc_mesh_begin/end` ->
  `stream_tag`) and every strip (`pc_mesh_strip`), which gives exact vertex correspondence
  between consecutive frames. Of several instances of one mesh, the one that moved least is
  matched. TEX0 is not part of the key (textures stream through rotating GS memory).
- The game's clip volume reaches into the GS guard band (`pc_clip_guard`), so geometry crossing a
  screen edge is not re-cut differently every frame.
- At each display refresh the executed frame's whole GS stream is **replayed** with matched
  vertices blended in homogeneous space (x*w, y*w, w = 1/Q; S/T/Q rescaled so textures keep their
  perspective). For a display rate n times the game's (60/120/240 Hz) t = k/n; otherwise
  t = time since arrival / frame period. Untagged output (2D, screen filters) is shown as is. A
  mesh is blended if at least half its vertices moved less than 128 px; single vertices moving
  more than 512 px are not.
- **Cuts**: a frame that changed scene or camera is shown at once, without in-between images
  (the previous frame's images end at that frame itself), so no image mixes two cameras. The
  frame's scene is taken when its first mesh is drawn (`view_glue.c pc_frame_view`), because
  `bhCheckCut` changes the cut after drawing: stage, room and, unless the third-person camera is
  on, the cut (`ncut`, `camver`), the event/fixed camera mode with its event camera, and first
  person. In addition the eye and view direction of `cam.mtx` are compared (`stream.c view_cut`):
  a move of more than 10 units or a turn of more than 30 degrees in one game frame is a cut (event
  cameras can switch a frame before their number does; the game's own smooth pans between cuts
  stay below 5 units and 6 degrees). The third-person camera is not compared: it turns fast under
  the mouse but never jumps.
- **Effects** (`effect.c` 3D lists: fire, the lighter flame, muzzle flashes, smoke, sparks, blood)
  are tagged per effect (`pc_effect_tag`): the tag is the effect's `eff[]` slot and that slot's
  life (it starts again when the slot holds another effect or was not drawn in the previous
  frame), and its polygons are numbered. Effect polygons drawn later from the ordering table get
  the tag after their vertices (`Ps2AddOT` writes it to GS register 0x7E, `STREAM_TAG_REG`). So an
  effect pairs only with itself; its position (and size on screen) is blended like a mesh's,
  its animation frame (texture coordinates) and colour are the current frame's. Effects use the
  128 px limit and do not count towards the "most meshes matched" test. 2D effects, lines and
  effects with their own draw function (`ef_fnc`) stay as they are.
- **Frame blend**: `PS2_swap` mixes each new frame 75% over the previous display image (a
  motion trail). Replays start from the display image as the frame found it, so the trail stays
  correct. With frame blend off the swap copies the frame instead (as during movies).
- **Replay targets**: a replay draws into a second set of render targets swapped in for its
  duration, so the executed frame's targets are never drawn over. Only the parts a frame reads
  before overwriting are copied in. The display target's start image is the previous final
  image (the two swap roles instead of being copied).
- **Static strips**: a matched native strip whose blended fields are bitwise identical in both
  frames is drawn unblended from a per-frame vertex buffer instead of being clipped, converted and
  uploaded again.
- Replays load no CLUT, skip unchanged batch state, set uniforms only when they change, and
  same-value register writes do not end a batch (except inside a two-pass alpha-test batch).

### Presentation and frame rate limit

With vsync on but a swap that returns at once (offscreen drivers, some compositors), the render
loop paces itself to the refresh: it measures the swap duration (averaged over 8 presents; below
1/8 of a refresh means the swap does not wait) and then presents on a fixed grid one refresh
apart. An in-between replay is skipped when the next game frame is already queued. The frame
rate limit (`fps_limit`: 30/60/90/120/144) caps presents per second below the display refresh
(`render.c present_hz`); the game itself stays at 30.

Picture calibration (`brightness`, `contrast`, `gamma`) is applied in the present shader
(`gs_gl.c draw_display`, uniform `u_pic`), so executed frames, in-between images and movies get it
and the overlay drawn afterwards does not. At the neutral values the shader skips it and the
output is bit-identical; `gs_picture` is the same formula on the CPU (F1's test pattern).

`glGetError` is read only every 64th executed frame (reading it after every flush made the
render thread wait for the driver's GL thread).

## VU emulation

VU microcode is not rewritten by hand: the decomp's `vsm/*.vsm` listings are converted to
decoded tables (`pc/tools/vsm2inc.py`). The execution model handles FMAC latency, delayed
MAC/clip flags, Q latency and branch/[E] delay slots, because the original programs are
hand-scheduled around them. The programs are compiled statically: `vu_step()` (one instruction
pair) is inlined into a switch with one case per micro address (`vu_static.inc`), so the C
compiler folds every decoded field into straight-line code. Both paths share `vu_step()`.

Inline EE/VU0 assembly in the decomp is translated to C by `pc/tools/asm2c.py`.

The VU0 time is almost all `VU0_CALCPOINT` (one vertex of a chunk model per call, 97-99% of
the microcode time in every scene) and the macro-mode ops of the translated asm (skinning,
matrices, `Ps2AddPrim3DMod`). These run as native code with the same operations in the same
order, the same clamping (`vu_clampf`: Inf/NaN -> +-FLT_MAX, denormals -> +-0), flags and
resulting VU state:

- `VU0_CALCPOINT` (`vu.c calcpoint_fast`): every light setup, directional and point lights. The
  program has no loops and its stalls only delay, so its timing is resolved statically: which
  DIV/RSQRT result each `MULq` reads (and Q at the end), which FMAC's MAC flags each `FMAND`
  reads, the CLIP flags at `FCGET`. Work whose result the program overwrites is skipped.
- Whole vertex runs (`vu0_calcpoint_n`, called by `njCnkCvVnPs2` through a `TARGET_PC` hook in
  the decomp): four vertices per vector, one per lane, with the same operations per vertex;
  only the last vertex runs through `calcpoint_fast`, which leaves the VU state of the
  per-vertex calls (vi11 and the clip flags are folded over the run). On x86 this kernel is
  also compiled for AVX and chosen at run time (`RECVX_VU_AVX=0`: SSE2).
- Macro mode: `vu_upper` (`vu0.h`) is inline; at the asm2c call sites all its arguments are
  constants, so each COP2 op folds to a few vector instructions. The strip clip test of
  `njCnkCsUvh/Uvn` (`vu_strip_clip`) is the same ops in one loop.
- The four-lane layer (`vu_simd.h`) is SSE2, or plain C with the same results
  (`RECVX_VU_PORTABLE`, any non-x86 build).

The other VU0 programs (`CLIP_VIEW_VOLUME_ALL`, `CALC_COLOR`, `CALCPOINT_INIT/END`, ...) take
about 0.1 ms a frame and stay on the static path, as do the VU1 programs (unused while the
native 3D paths are on).

Exactness: `RECVX_VU_CHECK=1` runs every microprogram call and every CALCPOINT run both ways
and compares the whole VU state (and the run's outputs); `RECVX_VU_INTERP=1` runs all
microprograms (CALCPOINT runs too) through the interpreter. `pc/tests/test_vu.c` (ctest `vu`,
`vu_portable`) compares CALCPOINT, the runs and every macro op (op, field mask, broadcast,
overlapping registers) against the interpreter on random states with overflow, denormal,
zero, -0, Inf and NaN inputs, in SSE2, AVX and plain C. With two NaN operands of ADD or MUL,
which NaN (and so which clamped sign) survives depends on the compiler's operand order, in the
interpreter too; the PS2 has no NaN, and the tests use negative NaNs.

### Cost per frame

Game-thread time per game frame (`--profile`: logic + VU0 + VU1 + GIF recording, mean of game
frames 400-1500 of a headless run; the two builds alternately on an otherwise idle Ryzen 7
9800X3D, Linux, 32-bit build; run to run within about 20%). Before: the build without the
native VU0 paths (CALCPOINT native only without point lights).

| Scene | Before | After |
|---|---|---|
| `RM_0000`, new game, cutscene skipped | 6.0 ms (VU0 4.9) | 1.1 ms |
| `RM_0000`, the opening cutscene | 2.7 ms (VU0 1.9) | 0.6 ms |
| `RM_0060`, walking and shooting zombies (as `layout_check.sh`) | 4.5 ms (VU0 3.2) | 1.0 ms |
| `RM_0060`, standing while a zombie comes | 7.6 ms (VU0 5.9) | 1.3 ms |
| `RM_0030` (outdoors, rain) | 5.8 ms (VU0 4.3) | 1.1 ms |
| Title menu | 0.06 ms (no VU0) | 0.06 ms |
| RE4 village, whole map (`RM_1310` mod, ~200k vertices a frame) | 23.3 ms | 9.6 ms |

Retail rooms have point lights, so CALCPOINT ran through the static interpreter (~0.8 us a
vertex); every macro op was a call with register copies. The village has none: its CALCPOINT
was already native, at ~75 ns a vertex, now ~16 ns. The largest game-thread costs now: CALCPOINT
runs, `pc_native_strip`, `vu_strip_clip`, the other VU0 programs (~0.1 ms) and, in the
village, collision. The render thread (GS emulation, replays: `r_exec`, `r_interp`) is separate:
1.5 ms a game frame in a retail room, 7.7 ms in the village.

## Audio and movies

- **ADX** (`audio/adx.c`, `audio/audio.c`): CRI's ADXF file system (AFS partitions and plain
  files read from the disc image) and the ADXT stream player with the original status and time
  behaviour (the game waits on `ADXT_STAT_PLAYEND` for voice lines). Each stream is an
  `SDL_AudioStream` decoded on demand on SDL's audio thread.
- **Sound effects and MIDI** (`audio/snd_iop.c`, `audio/snd_synth.c`): Capcom's TSNDDRV IOP
  driver is emulated at the SIF RPC level. The TQ request layer follows the retail
  `TSNDDRV.IRX`; HD/BD banks are played on a model of Sony's hardware synthesizer on SPU2 voices,
  with SPU2 reverb presets read from the disc. SQ MIDI sequences (ambient, door, event sounds)
  run on a sequencer with the loop conventions of `MODMIDI.IRX`, on the EE's 10 MIDI ports
  (ports 0 and 1 carry the room's background ambience; the system sounds - typewriter, item box,
  computers, doors - rotate over ports 2, 8 and 9). MIDI bank 1 is the door's (`DOOR_xxx.SPQ`,
  loaded only with the door animation), 2 the common system sequences, 3 the room's ambience.
  - **Timing**: on the PS2 the driver runs on the IOP beside the game; the EE sends one command
    batch per frame (`sdSysServer`) and the IOP's 240 Hz timer ticks the TQ slots and the MIDI
    ports in between. Here the tick runs in the SDL audio callback, every 200 output samples, and
    the RPC handler queues the sound commands in order, stamped with the time they were sent. A
    mixed buffer stands for the time since the previous one, so each command runs at its share of
    the buffer: commands keep their spacing (one game frame) and the ticks fall between them, as
    on the PS2. Bank data commands (HD/BD/SQ uploads) run at once on the game thread, after what
    was queued before them, since the EE reuses the staging buffer and polls the checksum. The
    status block (`se_info`, `midi_info`) is filled from the driver state when the EE reads it
    (`sceSifGetOtherData`). Without an audio device the commands run at once and the ticks follow
    the vblank count.
  - Why: the computers print a letter per frame, each a system MIDI sound that the next letter's
    `sdMidiPlay` stops (`SdrBgmStop` of the previous port). When the ticks ran only at the next
    RPC, a sequence's first note was keyed on and stopped in the same instant and never sounded;
    short notes of other sequences were lost the same way.
- **Movies** (`movie/`, `pc/glue/movie_glue.c`): the disc's FMVs are PSS files (MPEG-2 program
  stream: MPEG-2 video, 320 pixels wide, progressive 29.97 fps, and 16-bit PCM in private
  stream 1), decoded by our own demuxer and MPEG-2 decoder (`pss.c`, `mpeg2.c`). The game's own
  movie code (Sofdec wrapper `mwwrap.c`, `ps2_sfd_mw.c`) runs unchanged: status PREP -> PLAYING
  -> PLAYEND, skip, pause, fades.
  - A **movie thread** (`player.c`) reads the file through its own handle, keeps half a second
    of sound queued on the output device ahead of what plays, and decodes into a ring of 8 YUV
    frames. Neither the game thread nor the render thread decodes.
  - The **game thread** (`readMpeg`) picks the frame for each game frame: one per two vblanks
    (the game runs at 29.97 Hz during movies, the movies' own rate), so the cadence is even; when
    the sound's position (samples handed to the device minus those still queued) is a whole
    frame or more away, the frame number follows it. The movie ends with its last video frame,
    as on the PS2.
  - The game's sprite (`vbrank_draw`, as on the PS2) draws the frame, so size, position and
    the screen fades drawn over it are the game's. Before it, `stream_movie` puts the frame
    number into the GS stream; the **render thread** (`gs_gl.c gs_movie_bind`) uploads the
    three planes once per new frame and converts them on the GPU (BT.601 video range, MPEG-2
    chroma siting, interpolated chroma) into the texture the sprite samples. The PS2 path's
    upload of the IPU's macroblock output into GS memory (`setImageTag`) is gone.
  - Movie frames have no tagged 3D, so the in-between path never blends or replays them: each
    display refresh shows the latest executed frame.

## Input

- `input/pad.c` emulates a DualShock 2 in pressure-sensitive mode on port 0, fed by the
  keyboard, mouse buttons and an SDL gamepad, with rumble through SDL.
- **Bindings** (`input/bindings.c`) work at the layer that builds the DS2 pad report: a
  binding row is a **DS2 button**, and the game's own key type (OPTION -> KEY CONFIG) turns
  buttons into actions in `bhSetPad` as before, so nothing in the game is patched per action.
  The menus name each row by what the button does under the current key type. Keyboard/mouse and
  gamepad have one binding set each, two slots per row; extra rows are the quick turn, quick knife,
  trade request, the third-person keys and the hotkeys. The ini format is in
  [USER_GUIDE.md](USER_GUIDE.md#rebinding). Game and third-person rows may share keys (third
  person takes over the keys it uses); hotkeys share with neither.
- **Input source** (`input_source`, `input_pad`): Gamepad only sets
  `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS` at run time (`bind_apply_source`), so the pad keeps
  working without focus; an environment variable `SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1` wins
  over it. There is no pause or mute on focus loss. Pad numbers come from
  `SDL_GetGamepadPlayerIndexForID` (XInput slot, PlayStation player LEDs, else the lowest free
  number).
- **Quick knife** (`pc/glue/knife_glue.c`, hooked in `bhSetPad` after the third-person input):
  while its key is held the Combat Knife is equipped through the inventory's own `WeaponSet` and
  raised (ready bit 0x10), so ammo, hit checks, aiming and the co-op partner's view are the game's
  own. It waits until the player is back to a neutral state (not aiming, reloading, grabbed, in a
  menu, event or door), and on release restores the previous weapon (or the same item in another
  slot) once the knife is lowered. Nothing is restored if the weapon was changed meanwhile.
- **Modern controls for the fixed cameras** (`fixed_cam_controls`; `pc/glue/modern_glue.c`, the
  math in `input/modern.c`, ctest `modern`): `pad.c` keeps the frame's screen direction (left
  stick past a 25% dead zone, else the d-pad rows). `modern_pad` (`bhSetPad`, before the quick
  knife) replaces the direction bits by forward (`0x1`, plus run `0x400` when the stick is pushed
  70%) or, more than 100 degrees off, a turn bit; `modern_player` (`bhControlPlayer`, after the
  mode handler) turns `plp->ay` towards the target, 40% of the way per game frame, at most 30
  degrees. The target is the direction on the ground of the view matrix's right axis (`cam.mtx`
  column 0, so cameras with roll work) combined with the stick. While the direction is held across
  a cut (`cam.ncut`, `cam.camver`) the old cut's axis stays until the direction turns by more than
  35 degrees or is let go. Untouched: aiming (`mode1` 1), the quick-turn row (back only), the game's
  own turns (`flg 0x100000`), third person, menus, events and doors; special moves (`mode2` 12 and
  above 13: stairs, steps, pushing) only get forward, when the direction is within 90 degrees of
  the facing. Only `plp == &ply`, so in co-op only the local player; nothing is sent.
- **Mouse in menus** (`pc/glue/menu_glue.c`, hook at the end of `bhSetPad`;
  `input/menu_mouse.c` on the main thread): the mouse drives the screens' own cursor state, so
  keyboard and gamepad work unchanged. Window positions map to the game's 640x480 2D space through
  the display rectangle (pillarbox, stretch and hor+ margins included). Covered: inventory, item
  commands, combine, examine (drag rotates, wheel zooms), item box, files, map, title menu, co-op
  lobby, OPTION and ENHANCED SETTINGS, the trade screen. The OS cursor is hidden unless such a screen or
  an overlay is open; third person releases the captured mouse while a screen is open.
- **Idle dimming**: the game's screen saver (`bhSysCallScreenSaver`, 50% black after 10800
  frames without pad input) only reads the PS2 pad. `pc_input_active` (`tps_glue.c`) also counts
  third-person mouse motion and PC-only input, so it no longer dims during mouse/keyboard play.

## Settings

One table, `g_settings[]` in `pc/src/host/settings.c`, drives both the F1 overlay and the
in-game ENHANCED SETTINGS pages. Each entry has a page, label, variable (or get/set), range and
step, value names or a printf format, flags, help text, an applier, a `locked()` reason and an
optional `.group` heading. Rows are in menu order; the pages (`SP_*`, `settings_pages[]`) are
DISPLAY, GAMEPLAY, CONTROLS, THIRD PERSON, AUDIO, CO-OP, EXTRAS. `recvx.ini` keys live in `keys[]`
in `pc/src/host/config.c`.

ENHANCED SETTINGS is option level 4 of the game's OPTION screen (`pc/glue/options_glue.c`; hooks in
`adv.c`: `OptionDef[8]`, `DisplayOptionPlate` and the fades treat it like KEY CONFIG). Its entry
sits right of EXIT, lettered from the option texture's own glyphs: left/right move between EXIT
and it, up from it goes to DEFAULT, down wraps to the top as from EXIT (`DisplayOptionPlateLevel0`,
`pc_options_mouse0` for the mouse). The pages use the game's font, cursor sounds and highlight;
labels and value names must be plain ASCII the font has (no `&`, `*`, `#`, `@`).

**Adding a setting**: add the variable's ini key to `keys[]` in `config.c` and one entry to
`g_settings[]`, for example

```c
{ SP_GAMEPLAY, "Enemy difficulty", V(g_config.enemy_level), 0, 2, 1, levels,
  .help = "...", .locked = in_session },
```

ENHANCED SETTINGS shows every row of its page; F1 shows every row too, except on the CO-OP and
EXTRAS pages (their own windows are F3 and F2), where only `SET_F1` rows appear. ctest `settings`
(`pc/tests/check_settings.cmake`) fails when an ini key in `config.c` has no row (by its variable),
unless the key is listed there with where it is set instead (the lobby's per-session choices, the
third-person key binding).

| Flag | Meaning |
|---|---|
| `SET_F1` | a CO-OP or EXTRAS entry that F1 shows too (those pages otherwise live in F3/F2) |
| `SET_RESTART` | takes effect at the next start; both menus add "(next start)" |
| `SET_GAME_ONLY` | only while a game runs |
| `SET_ACTION` (+ `.apply`) | a command, not a value; the native menu runs it when it closes |
| `SET_NOW` | an action the native menu runs at once |
| `SET_CALIB` | picture calibration: while the row is chosen, both menus show the grey-step test pattern (`settings_calib`) |

## Save system

Two save systems, chosen by `save_system` (F1 / ENHANCED SETTINGS GAMEPLAY "Save system"):

- **Native files** (0, default): `<save dir>/saves/NNN_<label>.sav`, one file per save, no limit
  (`pc/src/host/savestore.c`, tested by `pc/tests/test_savestore.c`).
- **Memory card** (1): the PS2 card as before, `pc/src/ps2/mc.c` (libmc on a folder,
  `<save dir>/card0`, 15 save slots plus the system file).

**File format.** A 64-byte header followed by the game's save exactly as the card holds it
(`SAVEFILE`, 0x838 bytes: `sys->version .. save_end` and the game's byte-sum checksum), so a native
save and a card save convert into each other by adding or dropping the header. Header
(little-endian): `"RECVXSAV"`, version 1, data size 0x838, `"SLUS-20184"`, a reserved word, the
time written (64-bit seconds since 1970), play time, save count and difficulty (32-bit each),
character, stage and room (bytes), padding and the header's byte sum. A file counts as a save
only if the header, its sum, the size, the game's checksum and the header's copies of the data's
fields all agree; anything else in the folder is skipped (and logged). The **number** `NNN` is the
save's identity: three digits, more past 999 (`1000_...`), a new save takes the highest number in
the folder plus one (invalid files included, so nothing is ever overwritten by a new save). The
**label** (`Claire_Prison-front`: character and the save menu's room name, made safe for any file
system, UTF-8 kept, at most 64 bytes) is for people and never parsed. Writes go through
`pc_save_file` (temporary file, flush, rename); overwriting a save in another room writes the new
name first and then removes the old file. The system file (`CONFIGFILE`: options, clear flags, best
times; the card's `BASLUS-20184`) is `saves/system.dat`, byte for byte.

**The game's screens.** The typewriter's save screen and LOAD GAME (`ps2_SaveScreen.c`,
`ps2_LoadScreen.c`) keep their states, drawing and messages; `pc/glue/save_glue.c` changes only
where the rows come from and who does the I/O:

| Hook (decomp, `TARGET_PC`) | Native files |
|---|---|
| `ExecuteSaveScreen` / `ExecuteLoadScreen`: `ExecuteMemoryCard` | `pc_saves_native(state)` takes the setting while the screen is in its first state; then `pc_save_card` replaces `ExecuteMemoryCard`: no card polling, and a read or write the screen queued on `McState` (by `ReadMemoryCard` / `WriteMemoryCard` from `mcReadStartSaveFile` / `mcWriteStartSaveFile`, which keep the game's own save logic: save count, ink ribbon, checksum) is done on the chosen row's file, ending in the card's end states (9 read, 10 written) or its error codes |
| `ExecuteState*AwarenessCard` | `pc_save_list` fills the file window (`SELECTFILEWINDOW.pRecordTop`, `lRecordMax`) with the saves in number order, plus an empty row with the next number when saving; straight to the list (no card check, card selection or "checking memory card") |
| `ExecuteState*SelectFile` | `pc_save_move`: up/down through any number of rows, wrapping as the card's 15 do; cancel leaves the screen instead of going back to the card selection |
| `ExecuteState*Save` / `*Load` | the row index can be past the card's 15 file names: file name 0 is passed and `pc_save_card` uses the row |
| `mcDispFileNumber` | numbers past 99 are drawn narrower, ending where two digits end |
| `ExecuteSysLoadScreen` / `ExecuteSysSaveScreen` | `pc_sysload_native` / `pc_syssave_native`: `system.dat` read at boot, merged (`SysSaveHikaku`) and written by OPTION and the ranking, in one frame without the card screens |

**Sounds**: the native screens keep the card screens' own sound calls (cursor, confirm, cancel,
the typed-in row's keys and bell after a save); the card-only steps they skip (slot choice, card
check, the system file's "save?" question) have no screen and play nothing, as no button is
pressed there.

With the memory card these hooks call the original code; `pc/tools/layout_check.sh` with
`INI="save_system = 1"` and a scripted card save and load give byte-identical frames to the build
before the native system.

**Rows and cursor.** Saving: the cursor starts on the new row (last); loading: on the save written
last. An empty row looks like the card's empty slot ("NO_DATA"); loading with no saves shows one.

**Migration.** The first time native files are used (`save_native_dir()` finds no `saves` folder)
the card's saves are copied in (`savestore_import_card`): slot nn becomes save number nn + 1 when
free, with `coop-partners-nn.bin` as `saves/NNN.coop-partners.bin`, and the card's system file as
`system.dat`; the card is never changed. "Import memory card saves" does the same later (saves
whose bytes are already there are skipped).

**Other readers** go through `pc/src/host/lobby.h`, which follows the active system: save "slots"
are the card's 0..14 or the native number - 1 (so a migrated slot keeps its meaning in `coop_start`
and `coop_items`), `lobby_save_slots()` is their count. The co-op lobby's START list, the joiner's
ITEMS choice, `RECVX_TEST_SAVE`, the host's partner records (`coop-partners-nn.bin` /
`saves/NNN.coop-partners.bin`, written after each save by `coop_save_written`) use it. Game over
"continue" and the retry points live in memory and never touch a save.

## Fidelity notes

- Game logic is compiled unchanged. Where the decomp relied on MWCC behaviour standard C does
  not have, the C was rewritten under `TARGET_PC` to the evidently intended semantics:
  `*mkaP->key++` read several times in one expression (`en03.c`, `en23.c`) becomes `key[0]`,
  `key[1]`, `key[2]`; implicit returns of asm functions through `v0`/`$f0` (`njSin`, `njCos`,
  `Ps2BitCount`, ...); functions without a `return` on some path were checked against the retail
  code and fixed where the value is used ([dev/missing-returns.txt](dev/missing-returns.txt)).
- PS2 FPU/VU arithmetic is not IEEE (no denormals, Inf or NaN; truncating rounding). Results
  are clamped like the VU (`vu_clampf`) and the build uses SSE single precision without FMA
  contraction, but bit-exactness with the EE FPU is not guaranteed.
- Uninitialized locals are zero (`-ftrivial-auto-var-init=zero`), a deterministic stand-in for
  MWCC's stale registers ([dev/uninitialized-locals.txt](dev/uninitialized-locals.txt)).
- MWCC compiles a signed division by a power of two as an arithmetic shift (rounding down; C
  rounds toward zero). Where it matters this is written as the shift under `TARGET_PC`:
  `sdfunc.c SE_BANK_NO`, the bank of a sound number (negative numbers are the system MIDI
  sounds: typewriter, item box, computers).
- Functions that were empty in the retail build stay empty (for example `ADXT_Pause`).
- `ps2_sg_sd.c Volume_Control`: retail's MIDI fade up writes the current volume into the fade's
  target and returns a stale register; the PC returns the current volume and leaves the target
  alone (otherwise a MIDI fade up ended silent).
- `player.c bhCPM0_damage`: a damage level above 2 (possible when another enemy's hit takes over
  a grab) is clamped to 2; there are three levels.
- The Windows build gives the game glibc's `rand()` (`pc/src/ps2/stubs.c`): the game assumes
  `RAND_MAX` 0x7fffffff, UCRT's is 0x7fff. Both platforms get the same random sequence.
- Translated from the retail ELF instead of the decomp's asm blocks (hand-written asm in the
  original): `bhObj005` (`objitm.c`), `njMirror`, `_ClipInter`, `ScissorTriangle`, `KazuSet`.

## Determinism

The same build run twice with the same input, and two builds of the same code with a different
binary layout, must give byte-identical frames, in vanilla and in enhanced mode. Game logic is
stepped per game frame, so wall-clock timing does not enter it. What can break it is retail code
reading memory it never wrote (stale stack, bytes past an array): on the PC such bytes are often
addresses, which change with the layout, with ASLR or with what ran before on the stack.

**Check**: `pc/tools/layout_check.sh BIN_A BIN_B OUT [ISO]` runs both binaries headless with the
same scripted input (new game, teleport to room 0/6 with the handgun, walking, shooting, idling
while zombies change motions) in vanilla and enhanced mode (in-between images at
`RECVX_INTERP_T=0.5`), dumps every 10th frame up to 1800 and compares them byte for byte; exit
status 1 on any difference. For a second layout configure another build tree with
`-DRECVX_LAYOUT_PAD=20500` (unused bytes before all code, data and bss). Run to run: pass the same
binary twice. `INI="key = value"` adds lines to the runs' `recvx.ini` (e.g. `save_system = 1`).
It needs the disc image and a GPU, so it is not a ctest.

Retail bugs fixed because they made frames depend on the layout:

| Where | What retail reads | Fix |
|---|---|---|
| `Motion.c` (`bhSetMotion`, `bhGetObjMotion`) | a motion change at `bhGetFrameNum(...) * 65536` starts one past the last key, so the blend reads one key past each array, and after the last array the motion's table of key pointers (heap addresses) | an array whose past-the-end read would hit the pointer table is swapped for the duration of the read for a copy ending in its first key; every other read stays as retail |
| `ps2_NaGraphics3D.c Ps2SetPlane` | copies a pointer parameter plus 12 stale stack bytes as the plane's point; the near-plane cone of `njDrawLine3D` (particle lines, search light, laser sight) went through stack addresses read as floats | the point is the camera point (0, 0, -dist) the planes are meant to pass through |

**Real time by design** (as on the PS2, which runs these off the display and sound hardware):
movies play by vblank count, pulled to the sound's position (`RECVX_SKIP_MOVIES` counts game
frames instead, so test runs repeat), ADX streams report their position from the audio stream, the play time counts vblanks,
and an in-between image's t follows the clock unless `RECVX_INTERP_T` fixes it. A run that waits
on these can shift by frames. The sound driver ticks with the audio output, so the sound status
the EE reads (which sounds still play) follows it too: from run to run a sound trace can differ in
the channel an enemy sound gets and in how many volume/pan updates are sent; the frames do not.

Not covered: other stale or out-of-range reads that the scenarios do not reach. `rand()` is only
called by the game thread.

## Profiling and logging

- `--log[=file]` copies all output (the game's own diagnostics and the port's messages) to a log
  file with a timestamp per line; a crash appends a backtrace (Windows: exception address and
  frame chain).
- `--profile[=file]` writes one CSV line per game frame: total frame time and exclusive time in
  game logic, VU0, VU1, GS emulation + GL submission, interpolation, presentation and idle wait.
  F1 -> Performance shows the same live. The native `CALCPOINT` (single calls and runs) is not
  timed separately and counts as game logic; VU0 is the other microprograms.
- `--profile-samples=file` (Linux, same as `RECVX_PROFILE=file`) samples each thread at 1 kHz of
  its own CPU time; `pc/tools/profile.py build/pc/recvx file [top] [r|g]` summarises by function,
  optionally for the render (`r`) or game (`g`) thread only. `file.maps` lets it name libraries.
- Render-thread wall time in `r_exec`/`r_interp` with low CPU use is time spent waiting for the GL
  driver or GPU.
- `room: RM_xxxx texture table has N list(s) more/fewer than the room reads`: a mod room whose
  texture lists do not match what `bhSetRoom` takes in table order (every later effect would use
  its neighbour's atlas). `pc/tools/assets.py dependencies` applies the same rule.

## Environment variables

Test aids and diagnostics, read at start. Frame numbers are presented frames for input scripts
and game frames for game-state aids, as noted. Co-op test aids (`RECVX_COOP_*`,
`RECVX_LOBBY_KEYS`, `RECVX_TEST_*`, `RECVX_TRADE_TRACE`) are listed in
[COOP.md](COOP.md#test-aids).

### General

| Variable | Effect |
|---|---|
| `RECVX_ISO=path` | the disc image (as on the command line) |
| `RECVX_MODS=dir` | mods folder (default `./mods`) |
| `SDL_VIDEO_DRIVER=offscreen` | headless run |
| `RECVX_SHOW_OVERLAY=1` / `=3` | open the F1 overlay / the co-op window at start |
| `RECVX_EXIT_FRAME=n` | exit at presented frame n |
| `RECVX_WATCHDOG=sec` | Linux: print a backtrace after sec seconds and exit |
| `RECVX_FAST_DEMO=1` | start the attract demo right away |
| `RECVX_SKIP_MOVIES=1` | movies end after 30 game frames without decoding (repeatable test runs) |
| `RECVX_SKIP_CINE=n` | skip every cutscene after n game frames, as its skip button |
| `RECVX_MOVIE=n` | play `MV_n.PSS` in place of every movie (e.g. the intro at start-up with `skip_intro = 0`) |

### Scripted input

| Variable | Effect |
|---|---|
| `RECVX_INPUT="frame:buttons,..."` | scripted pad input by presented frame, e.g. `200:start,205:-` |
| `RECVX_KEYS="f:key+key,f:-,f:@x:y"` | hold keyboard keys (SDL names, `_` for a space) from presented frame f, sent as key events; `@x:y` clicks at a window pixel |
| `RECVX_VPAD="f:input+input,..."`, `RECVX_VPAD_TYPE=xbox\|ps5\|generic` | a virtual SDL gamepad holding inputs (ini spelling) from frame f; `axis=value` holds an axis at -1..1 (`leftx=0.5+lefty=-0.5`: the left stick half up-right) |
| `RECVX_MENU_MOUSE="f:x:y[:act],..."` | scripted mouse in menus (window pixels; act `l`/`r` click, `L`/`U` left down/up, `w`/`s` wheel) |
| `RECVX_TPS_MOUSE="frame:dx:dy,..."` | scripted third-person mouse motion |
| `RECVX_UNFOCUS="f:1,f:0"` | hide the window at frame f (focus loss), `0` shows it again |

### Game state

| Variable | Effect |
|---|---|
| `RECVX_TELEPORT="f:stg:rom:pos,..."` | teleport at game frame f |
| `RECVX_PLAYER=n` | the first teleport switches the played character to `ply_id` n (as the script op `bhPlayerChangeSet`) |
| `RECVX_WARP="stg:rom:f:x:z:angle,..."` | move the player to x/z f frames after entering that room |
| `RECVX_EQUIP="f:item:count[;f:item:count]"` | equip a weapon at game frame f (again later after `;`) |
| `RECVX_HP="f:hp,..."`, `RECVX_RETRY="f,..."` | set health / set a retry point (Continue resumes in that room) at game frame f |
| `RECVX_TEST_TYPEWRITER="f,..."` | open the typewriter's save screen at game frame f (as its ink ribbon question's "yes"; no ribbon needed) |
| `RECVX_TEST_COMPUTER="f:n,..."` | open computer terminal n at game frame f, as the room scripts' `EvtComSet n` (1 self-destruct lock, 2 security lock, 3 Albinoid file, 4-6 3D scanner, 7 password; rooms 9/31, 1/2, 3/12, 0/9, 1/9) |
| `RECVX_FLAGS="set:bit,..."` | set story flags at the first game frame (1 `ev_flg`, 2 `ky_flg`, 3 `ed_flg`, 7 `it_flg`, 8 `mp_flg`) |
| `RECVX_INF_AMMO=1`, `RECVX_INF_HEALTH=1` | infinite ammo / health cheats on |
| `RECVX_NOCLIP="f:mode,..."` | noclip mode 0/1/2 from game frame f, logs position every 15 frames |
| `RECVX_APPEAR="f:chr,..."` | set "You appear as" (`coop_char` value) at game frame f |
| `RECVX_EFFECT="f:effno:type:mode1"` | spawn effect effno at the player (e.g. `60:110:0:1`: full-screen distortion) |
| `RECVX_SPAWN="f:id[.mdlver][@dist],..."` | the F2 enemy spawner at game frame f, with traces ([ENEMIES.md](ENEMIES.md)) |
| `RECVX_SPAWN_SURVEY="stg:rom,..."`, `RECVX_SPAWN_SURVEY_ALL=1` | try every spawner entry in each room, log results and budgets, exit |
| `RECVX_ENEMY_EXTRA=0..3`, `RECVX_ENEMY_PLACE=0/1`, `RECVX_ENEMY_RANDOM=0/1`, `RECVX_ENEMY_SEED=n` | enemy options over the settings |
| `RECVX_ENEMY_TRACE=dist` | one line per step of every enemy within dist units of a player |
| `RECVX_KNIFE_TRACE=1`, `RECVX_KNIFE_DROP=1` | quick knife: log its state; drawing the knife empties the previous weapon's slot |
| `RECVX_POSE_TRACE=file` | one line per game frame: the player's state (modes, weapon, motion, frame, `at_flg`, `ay`, `wax`) and every node of the body as drawn ("You appear as" included) in its base matrix's frame; twitches show as nodes moving back and forth (`pc/glue/pose_trace.c`) |
| `RECVX_STRESS_SETTINGS=1` | volumes change every frame (settings/audio thread test) |

### Rendering

| Variable | Effect |
|---|---|
| `RECVX_NATIVE3D=0/1`, `RECVX_NATIVE_TEX=0/1` | native 3D / native textures off/on |
| `RECVX_NATIVE_RTEX=0` | `StoreRenderTex` through the emulated readback instead of the GPU capture |
| `RECVX_NATIVE_DEBUG=1` | log the first native strips' vertices |
| `RECVX_NO_GUARD=1` | enhanced mode without the widened clip volume (diagnosis) |
| `RECVX_DBG_NOTEX=1` | draw everything untextured |
| `RECVX_DUMP_FRAMES=dir` | every 30th frame (and its in-between predecessor) and all render targets as BMP, with frame statistics |
| `RECVX_DUMP_EVERY=n`, `RECVX_DUMP_FROM=n`, `RECVX_DUMP_OVERLAY=1` | with `RECVX_DUMP_FRAMES`: also every n-th frame; nothing before frame n; also the window image with the overlay |
| `RECVX_DUMP_TEX=dir` | dump decoded textures |
| `RECVX_INTERP_T=t` | the first refresh after each game frame shows the in-between image at t (comparable dumps) |
| `RECVX_INTERP_STATS=1` | per-frame interpolation matching statistics |
| `RECVX_REPLAY_CHECK=1` (or `=n`), `RECVX_REPLAY_CHECK_DIR=dir` | replay each interpolated frame at t = 1, which must reproduce it; log and save differing frames |
| `RECVX_REPLAY_RTSTART=0` | in-between images start from the executed frame's display image (old behaviour) |
| `RECVX_DISPLAY_HZ=n` | assume an n Hz display (e.g. 1000 with vsync off: present as fast as possible) |
| `RECVX_TRACE_PRESENT=file` | one line per present: frame, t, start time, swap duration |
| `RECVX_MOVIE_TRACE=file` | one line per game frame of a movie: time, vblanks, frame shown, game-thread time, sound position and queue, sound minus picture (ms); at the end decode times and frames shown before they were decoded |
| `RECVX_GPU_TIME=1` | every 2 s: frames and in-between images per second, render-thread CPU and GPU time |
| `RECVX_TRACE_NTEX=1` | log native texture binds, misses and render-texture captures |
| `RECVX_TRACE_DMA=1` | log GIF DMA chain tags |
| `RECVX_VU_CHECK=1`, `RECVX_VU_INTERP=1` | compare the static and native VU code with the interpreter on every call / use the interpreter |
| `RECVX_VU_AVX=0` | the CALCPOINT run kernel in SSE2 instead of AVX |
| `RECVX_DUMP_CALCPOINT=n`, `RECVX_DUMP_VU1=n` | dump VU0/VU1 state at the n-th call |
| `RECVX_EFF_TRACE=1` | log live effects each frame |
| `RECVX_TPS_TRACE=1/2`, `RECVX_TPS_AIM_AT=f`, `RECVX_TPS_ORBIT=deg`, `RECVX_ATMOS_TRACE=1` | third person: log free aim (`=2`: aim at the locked enemy, `f` along its hit capsule); turn the view about the player; log light and fog ([THIRDPERSON.md](THIRDPERSON.md)) |

### Other traces

| Variable | Effect |
|---|---|
| `RECVX_TRACE_TASKS=1` | game task (`sys->tk_flg`) changes |
| `RECVX_TRACE_SND=1` | sound driver commands, key-ons, dropped requests (bank not loaded, no such program or block), stolen voices and notes released before they sounded (key-ons print from the audio thread) |
| `RECVX_TRACE_MC=1` | memory card commands and results |
| `RECVX_PAD_TRACE=1` | DS2 buttons of the pad report when they change |
| `RECVX_MODERN_TRACE=1` | modern controls: each frame a direction is translated, the direction, cut (and whether the previous cut is held), target and facing angle, the player's place on screen and the camera |
| `RECVX_MENU_TRACE=1` | menu clicks with the menu state |
| `RECVX_ITEM_TRACE=1` | the bag (equipped slot, lighter slot, slots) and the metal detector's security boxes (`sys->itm[192]`, `[224]`) whenever a word changes, with the box screen's cursors, by presented frame; in 0/9 also the detector's floor zone (`sys->flr_idx`), its "metal carried" flag (`rm_flg` bit 2) and whether it went off (`ev_flg` bit 53) |
| `RECVX_PROFILE=file` | sampling profiler (as `--profile-samples`) |

## Tools

| Tool | Purpose |
|---|---|
| `pc/tools/asm2c.py`, `vsm2inc.py` | EE/VU0 inline asm to C; VU microcode listings to decoded tables |
| `pc/tools/layout_check.sh` | determinism check (above) |
| `pc/tools/compare_native.sh`, `compare_frames.py` | compare native and emulated rendering, frame dumps |
| `pc/tools/profile.py` | summarise sampling profiles |
| `pc/tools/decomp_warnings.py` | regenerate the lists in [dev/](dev/missing-returns.txt) |
| `pc/tools/enemies.py` | enemy census over the room files ([ENEMIES.md](ENEMIES.md)) |
| room and map tools | [modding/README.md](modding/README.md) |

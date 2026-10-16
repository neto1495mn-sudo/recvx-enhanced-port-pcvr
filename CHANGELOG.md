# Changelog

## 0.4.0 (2026-10-16)

### Gameplay

- **Battle Game from the start**: BATTLE GAME is in the title menu without a finished
  playthrough (GAMEPLAY setting "Battle Game unlocked", on by default). It only changes what the
  menu offers: your saves and system data are not touched, and turned off the menu is as in the
  original again. Finishing a Battle Game saves its records but does not count as finishing
  the story. Its characters unlock as before: Claire from the start, Chris after finishing the
  story once, the others under the original's conditions.

### Renderer

- **Menu frames: no more dark lines**: the status screen, files, map and item screens showed thin
  dark lines between the pieces of their frames and backgrounds, most visible at higher internal
  resolutions (for example around the text box under the menu). The last pixels of each piece
  reached past its picture into the neighbouring one in the texture; each piece now samples only
  its own picture, as on the PS2. Fixed thanks to fmil95 (decompilation team), who found
  and tracked down the problem.
- **Steve in the gas room** (Palace): the monitor close-up when freeing Steve showed the game in a
  tiny window; it now shows the full view, and the gas effect fills the whole screen in
  widescreen. The memory card screen's save counts had the same kind of bug.

### Fixes

- **Alexia's tentacles** use their intended attack timing again: a decompilation bug read the wrong shape table. Fix by fmil95 (decompilation team).
- **Metal detector with "No ink ribbons needed"** (prison): with the setting on (or the co-op
  host's INK RIBBONS *Not needed*), the detector went off even with every metal item put in the
  trays. The setting made every check for an ink ribbon succeed, and the detector checks for ink
  ribbons too, since they count as metal there. The setting now only applies to the typewriters;
  the detector goes off for ink ribbons you actually carry, as in the original. Test aid:
  `RECVX_ITEM_TRACE=1` also logs the detector's state in that room.

## 0.3.1 (2026-10-16)

### Renderer

- **Camera cuts at high frame rates**: the in-between images no longer blend across a camera
  cut. The game changes the cut after drawing a frame, so the renderer took the cut one
  frame late and swept the camera from the old view to the new one, through walls and past room
  parts hidden for one of the two cameras. The cut is now taken from the frame as drawn, event
  cameras and camera jumps (a cutscene cut, a door animation's camera) count as cuts too, and
  the new view is shown at once. As a side effect the frame just before a cut is now smoothed too.
- **Effects follow what they are attached to**: the lighter's flame, muzzle flashes, smoke,
  sparks, fire and blood are placed in the in-between images like the 3D models, so the flame
  stays on Claire's lighter instead of jumping at the game's own rate. Their animation still steps
  at the game's rate. `RECVX_INTERP_STATS` also counts matched effect vertices and logs camera
  jumps.

### Picture calibration

- **Brightness, Contrast and Gamma** in DISPLAY / PICTURE (F1 and ENHANCED SETTINGS), with
  "Reset picture calibration". They adjust the game's picture, menus and movies included, but not
  the F1 overlay, and change it live. At the neutral values the picture is exactly as before.
- While one of them is chosen, both menus show a small test pattern of grey squares on a black and
  a white strip, to set the dark and light ends.

### You appear as

- **Models keep their own proportions when animated**: each model is moved by its own skeleton,
  with Claire's or Chris's joint rotations carried over relative to each one's rest pose, instead of
  fixed joint positions. Long or extra-jointed limbs now bend at the right joints: the Bandersnatch
  holds the weapon in its claw instead of mid-arm, Monster Steve moves his arms from the shoulders
  instead of the collarbones and holds the weapon in his hand instead of at the elbow.
- **Feet on the floor**: a taller or shorter model crouches and bobs in proportion, and a foot of
  another shape no longer lifts the body.
- **Aiming up with tall models** in third person: the camera's height, shoulder offset and distance
  follow the model's height, so it is no longer inside a Tyrant or Monster Steve while aiming; the
  aim pose is solved on the weapon as it is drawn, so its barrel points at the crosshair for every
  model and pitch. The shots are
  unchanged: they always went where the crosshair is.
- A weapon or gloved hand the model has no hand for (Nosferatu's right, the Bandersnatch's left)
  is no longer drawn floating in the air.
- The character played without another model is unchanged. Test aids: `RECVX_RETARGET=0` keeps
  the previous animation of models, `RECVX_RIG_DUMP=1` logs their skeletons and feet
  ([ACTORS.md](docs/ACTORS.md#retargeting)).

### Audio

- **Computer terminals click for every letter**: the Palace's security lock, the 3D scanner, the
  Albinoid file, the password and self-destruct terminals. Each letter the screen prints starts a
  short sound that the next letter stops one frame later. The emulated sound driver only moved on
  when the game sent its next sounds, so a letter's sound was started and stopped at the same
  moment and never heard: only the last letter of a line clicked. The driver now runs on the
  sound output's own clock, as the PS2's sound processor does beside the game: its timer ticks
  between the game's frames, and the game's sound commands are played in order at the time they
  were sent. Every letter clicks, evenly spaced.
- Other sounds lost the same way play too: a system sound cut by another one in the next frame,
  and very short notes of the rooms' background sound sequences. Sound effects and sequences also
  start without waiting for the game's next frame.
- Test aids: `RECVX_TEST_COMPUTER="frame:n"` opens computer terminal n; `RECVX_TRACE_SND` also
  logs notes released before they sounded.

### Fixes

- Third person: the knife no longer slashes upwards at an enemy standing right in front; it slashes level at anything not below the player (aiming down at crawlers still works).
- **No more twitching when aiming or using the knife as another model** in third person: with
  "You appear as" another model, or Claire's or Chris's model on the other character with
  gender-matched animations, the upper body jerked back and forth when a weapon was raised,
  fired or the knife swung. The aim on the weapon as drawn is now only adjusted while the aim pose
  holds still, smoothly, and not for the knife. The character played without another model was
  never affected. Test aid: `RECVX_POSE_TRACE=file` logs the player's body as drawn, per frame.
- **Hands in the lobby's model preview**: the characters in the MULTIPLAYER lobby had no hands.
  They now wear the hand pieces the game gives an unarmed player, on the same wrist joints: each
  player character its own; the story cast's models, which have none, Chris's (men) or Claire's
  (women). Zombies and creatures keep their own hands. The look in game is unchanged.
- **Movies play smoothly and in sync with their sound**: the intro, the opening, the in-game
  movies and the ending. Reading and decoding a movie now happen on a thread of their own
  instead of in the game's frames, the sound is kept queued ahead so it no longer runs dry
  (gaps, and silence over the last frames of a movie), the picture follows the sound's clock
  instead of running on its own, and the short freeze at the start of every movie is gone. Each
  frame is uploaded once and converted to RGB on the GPU, with smoother colour edges; it is no
  longer passed through the emulated GS memory. Skipping, pausing and the fades over a movie
  work as before. Test aids: `RECVX_MOVIE=n` plays movie n in place of every movie,
  `RECVX_MOVIE_TRACE=file` logs the frame timing and the sound's offset.

## 0.3.0 (2026-10-16)

### Controls

- **Modern controls for the fixed cameras** (option, off by default): CONTROLS -> "Fixed camera
  controls: Classic (tank) / Modern (camera-relative)" in F1 and ENHANCED SETTINGS, ini
  `fixed_cam_controls`. As the alternate controls of the RE HD remasters, the left stick or the
  d-pad (arrow keys) walks where it points on screen, turning quickly and smoothly;
  the stick pushed far or the run button runs. Through a camera cut the held direction keeps
  the previous camera's meaning until it is clearly changed or let go, so the character does
  not walk back and forth at cut boundaries. Aiming, the quick-turn row,
  stairs, doors and items work as before; third person keeps its own controls; in co-op only
  your own player ([USER_GUIDE.md](docs/USER_GUIDE.md#modern-controls-fixed-cameras)).
- Test aids: `RECVX_VPAD` holds axes at a value (`leftx=0.5`), `RECVX_MODERN_TRACE=1` logs the
  modern controls; ctest `modern` covers the direction math and the cut hold.

### Performance

- **VU0 runs as native code**: much less CPU time per frame on the game thread. The vertex
  program `VU0_CALCPOINT` ran through the VU interpreter whenever a point light was on (most
  rooms); it is now native for every light setup and computes whole vertex runs four vertices at
  a time, using AVX when the CPU supports it and SSE2 otherwise (`RECVX_VU_AVX=0` forces SSE2).
  The macro-mode VU0 operations of the translated EE assembly (skinning, matrices) are inlined.
  Results are bit-exact with the interpreter: frames are byte-identical, `RECVX_VU_INTERP=1`
  still forces the interpreter, `RECVX_VU_CHECK=1` compares every call in game, and ctest
  `vu`/`vu_portable` compare the native code with the interpreter on random and edge-case inputs.

### Renderer

- Font: no more seams around the letters of messages, item names and "Now Loading..." at higher
  internal resolutions. The glyph quads' texture coordinates reached a quarter texel into the
  neighbouring glyph of the font texture; they now sit half a texel inside the glyph's cell on
  every edge. Fix by fmil95 (decompilation team).

### Audio

- **Missing system sounds**: the typewriter's keys and bell while a save is typed in, the item
  box lid, the computer terminals' beeps and typed text (and other sounds the game plays as
  system MIDI, such as some effects and enemy events) were silent in every save mode. The game
  takes a sound's bank from its number divided by 256; the PS2 compiler rounds that down (an
  arithmetic shift), C rounds toward zero, so these numbers (bit 31 set) asked for bank 3, which
  holds the room's ambience, instead of bank 2.
- The emulated sound driver has the game's 10 MIDI ports: the system sounds rotate over ports
  2, 8 and 9, and 8 and 9 were played on ports 0 and 1, where they stopped the room's background
  ambience (with door animations on, the door and stairs sounds did).
- `RECVX_TRACE_SND` also logs requests the driver drops (bank not loaded, no such program or
  block) and voices stolen when all 48 are busy.

### Fixes

- **Metal detector security boxes** (prison, 0/9): taking an item out of the box onto the lighter
  ("STANDARD") slot no longer makes it vanish. As on PS2, the item goes into the bag (or "There's
  no space to put the item in.") and the lighter, if it was there, goes into the box; storing
  the lighter with a full bag no longer overwrites the last item, and the lighter can be taken
  out onto any slot again. The item box exchange (`ItemBoxChange`) was an inexact decompilation; it now does
  what the PS2 code does in every case, which also keeps the equipped weapon right when two-slot
  weapons move in or out of any item box and fills a partial ammo stack when ammo is taken out
  onto an empty slot. Test aid: `RECVX_ITEM_TRACE=1` logs the bag and both security boxes.

## 0.2.0 (2026-10-14)

### Third person

- Free aim hits zombies lying or crawling on the ground (and a knocked-down Hunter) with every
  weapon. The game only lets those take damage from a shot aimed down, and free aim always told
  it "straight": it now passes the aim height of where the crosshair shot goes (down below knee
  height or 22.5 degrees, up above 22.5 degrees) to the enemies, while the weapon keeps its
  straight motions.
- "Game auto-aim": W / S aim up / down while the weapon is raised, as the original's up / down
  (before, only the arrow keys and the gamepad did).
- Test aid: `RECVX_TPS_TRACE=2` keeps aiming at the locked enemy, `RECVX_TPS_AIM_AT=f` at a point
  along its hit capsule; the trace logs the aim height.

### Settings

- The OPTION menu's PC SETTINGS is now **ENHANCED SETTINGS**, right of EXIT as before, reached
  with left/right from EXIT (like STEREO / MONO) instead of down; up from it goes to DEFAULT, down
  wraps to the top. Mouse hover and click areas match.
- Pages in a new order, the same in F1: DISPLAY (window, picture, renderer: the former ENHANCED
  page is part of it), GAMEPLAY (also saves, ink ribbons and all enemy options, Extra enemies
  included), CONTROLS (also Mouse in menus), THIRD PERSON, AUDIO, CO-OP (also Show ping), EXTRAS.
  Headings group the rows of a page in both menus.
- ENHANCED SETTINGS shows 9 rows and three help lines (the Input source help was cut off); the
  mouse wheel moves the selection as in the game's menus instead of changing the hovered value.
- ctest `settings`: every `recvx.ini` key needs a row in the settings table (so both menus show
  it) or an entry in `pc/tests/check_settings.cmake` saying where it is set.

### Saves

- Native save files, the new default (**Save system: Native files**, ini `save_system = 0`): every
  save is its own file in `save/saves` (`001_Claire_Prison-front.sav`, ...), numbered without
  limit. The typewriter's save screen and LOAD GAME look as before but list the files, scroll
  through any number of them and offer a new save at the end; the memory card screens (slot
  selection, "checking memory card", creating save data) are gone. The system data (options,
  extras, best times) is `save/saves/system.dat`.
- A save file is the memory card's save with a 64-byte header (game, time, play time, character,
  room); invalid files are skipped. Writes are atomic; a save is only replaced when you overwrite
  it.
- The first start with native files copies the memory card's saves, system data and co-op partner
  files into `save/saves` (the card is kept); F1 / ENHANCED SETTINGS "Import memory card saves" does it
  again later.
- **Memory card (original)** (`save_system = 1`) keeps the emulated card exactly as before
  (frame-identical).
- The co-op lobby's START list, the joiner's ITEMS choice and the host's saved partner inventories
  (`save/saves/NNN.coop-partners.bin`) follow the active save system.
- Test aid `RECVX_TEST_TYPEWRITER`; `pc/tools/layout_check.sh` takes extra ini lines (`INI`).
- **No ink ribbons needed** (F1 / ENHANCED SETTINGS GAMEPLAY, ini `no_ink_ribbon`, default off; in co-op
  the host's lobby rule INK RIBBONS): typewriters save without an ink ribbon and use none up.

### Co-op

Co-op protocol version 19 (both players need this build).

- **Shared loot is one pool** (lobby LOOT *Shared (one pickup)*): each item lies there once and
  whoever takes it has it; the spot is gone for both and the partner gets nothing, so the players
  split ammo, herbs and other supplies. Before, both got a copy of everything either took. Key items
  and the lighter still go to both (a used-up key still goes for both), as do items from events that
  are for both (key items, the lighter; weapons and ink ribbons with the rule below); supplies an
  event hands over stay with the triggering player. If both take the same item at about the same
  moment, exactly one keeps it (the host when both happened within one round trip); the other's copy
  goes back ("the partner took ... first"). Decomp hook `coop_item_late` in `bhItmCk`. Individual
  loot is unchanged.
- Game rule **Important items for both** (shared loot only; lobby IMPORTANT ITEMS *For both* /
  *Shared*, F3, ENHANCED SETTINGS CO-OP; ini `coop_important`, default on): weapons and ink ribbons
  lie there for each player; one player's pickup leaves the partner's copy where it lies. Off, they
  are pool items like the rest. Each player's own pickups go with its items (saves, the host's saved
  partner inventories).
- Game rule **Synchronized cutscene skip** (lobby CUTSCENE SKIP, F3, ENHANCED SETTINGS CO-OP; ini
  `coop_skip_sync`, default on): a cutscene both players watch is skipped once both pressed skip;
  "Waiting for <partner> to skip..." / "<partner> wants to skip - press <button> to skip" at the
  top of the screen. Both scenes end together and leave both players where the scene puts them.
- The lobby's GAME RULES also set INK RIBBONS for the session.
- Protocol 19: game rules `OPT_IMPORTANT`, `OPT_NO_RIBBON`, `OPT_SKIP_SYNC`; pose flags `PF_CINE`,
  `PF_SKIP`; event `EV_SKIP` (host only); `EV_PICKUP` carries an item only for items both get, and
  `undo` (host only) for a spot both took; unknown pose flag and rule bits are refused. ctest `coop`
  covers them.
- Test aids `RECVX_COOP_GOTO` (our player alone to a spot) and `RECVX_COOP_PAD` (game pad presses at
  a host frame).
- Fixed: a crawling zombie's leg grab could repeat without end (the grabbed player in the attacked
  animation with the damage sound over and over). The grab ends with the player stomping the
  crawler dead; the grab failsafe let the player go mid-stomp and put the dead zombie back to
  standing, so it crawled on with no health and grabbed again. Now a dead zombie stays dead, the
  failsafe waits for the stomp, and a loop guard makes an enemy that attacks the same player three
  times within 90 frames leave it alone for 60 (logged).
- Enemy targets: the player right in front of an enemy (within 1.5 m, in its front quarter) is its
  target; only a grab or bite keeps a victim (one within 3 m), no longer an enemy's own hit or death
  states; a player on another floor counts as twice as far; a zombie's parts go for its victim.
- The shooter's aim height counts for hits in co-op: lying zombies and other enemies that ignore a
  shot aimed straight judged the shot by the aim of the player they chase (the partner's stand-in
  has none). The partner's hits carry their aim height (`EV_ENE_HIT`, protocol 19).
- `RECVX_ENEMY_TRACE` shows type, busy state and attacks on the partner; test aid
  `RECVX_COOP_CRAWL` (the host's zombies crawl).

## 0.1.0 (2026-10-13)

First release. Native Linux and Windows (32-bit, i686) port of Resident Evil Code: Veronica X
(PS2, SLUS-20184), built on the recvx-decomp decompilation. No game data is included; you need your
own disc image.
Co-op protocol version 18, rendezvous protocol version 3.

### Platform and port

- Game code, including the original PS2 Ninja/Shinobi layer, compiled from the decomp; DMA, VIF,
  GIF, GS, CD/DVD, pad, memory card and SIF RPC emulated underneath.
- VU0/VU1 microcode compiled statically to C (interpreter kept as a reference, `RECVX_VU_CHECK`
  compares both); hand-written fast paths for vertex lighting and strip clipping.
- Audio: ADX streams, TSNDDRV sound effects on modelled SPU2 voices with reverb, MIDI sequencer.
- FMV playback with its own MPEG-2 decoder.
- Windows build cross-compiled with mingw-w64 GCC (static, large-address-aware); runs the same
  saves and co-op sessions as Linux.
- First start without a disc image opens a file selector and remembers the choice (`iso` in
  `recvx.ini`).
- Atomic save writes, UTF-8 paths on Windows, glibc compatibility shim so the Linux binary runs on
  glibc 2.34 and newer.

### Renderer

- OpenGL renderer with runtime internal resolution, fullscreen (F11) and an FPS counter.
- Widescreen and ultrawide (hor+, up to 4:1): 3D fills the margins, room parts hidden for 4:3
  cameras are shown when they lie outside the 4:3 view, 2D and menus stay 4:3 centred; fades,
  pause darkening and the screen saver cover the whole view.
- Native 3D path for chunk models and ordering-table primitives, perspective-correct colour and
  fog; native TIM2 textures with real CLUT uploads and memory caps; render-to-texture effects
  captured on the GPU instead of read back.
- Enhanced mode: in-between frames at display rate (game logic stays at 30 Hz), blended from the
  executed frame's native geometry and resources; cheaper replays into a second render-target set.
- Frame rate limit (off/30/60/90/120/144), present pacing on the display refresh, optional
  "Frame blend (motion trail)", optional off-screen model culling.
- Larger PC room limits (bigger room arena, vertex and primitive buffers) for custom maps.

### Co-op

- Online two-player co-op (F3 or MULTIPLAYER on the title menu): host-authoritative enemies, flag
  and script-event sync, cutscene items and levers shared, independent rooms with optional follow.
- Lobby in the game's style, in steps (connection, host/join, game rules), with 3D previews,
  player names, name tags, ping display and a "Remove stuck player" action for the host.
- Shared or individual loot, own or shared item box, per-player inventories kept by the host,
  event item gifts for a partner who missed the event.
- Trade screen with both inventories, item requests and atomic swaps; optional enemy freeze while
  trading.
- Partner shown as any chosen model, with its weapon sounds, voice, lighter and effects; enemy
  effects (acid, vomit, grenades) shown and harmful on both sides.
- Enemies choose the nearer free player; zombie grabs work on both players.
- Direct play via `ip:port#CODE` with UPnP/NAT-PMP port mapping, or via a rendezvous server with
  hole punching and relay fallback; end-to-end encryption (X25519 + XChaCha20-Poly1305) in every
  server session.
- Rejoin with a per-install player ID; lost sessions time out; all datagrams validated before use.
- Local co-op on one PC: per-window input source (keyboard/mouse or a chosen gamepad).

### Server

- `recvx-server` rendezvous and relay server (C) with server-assigned room codes, Argon2id room
  passwords, authenticated relay frames, per-address and global rate limits and relay budgets.
- Config file with SIGHUP reload, Prometheus-style status metrics, anonymised logging, systemd
  unit, Dockerfile and a `recvx-client` CLI.

### Third person

- Over-the-shoulder camera (F5, `--third-person`) with WASD/mouse controls and free aim; spine
  and chest bend to the aim pitch.
- Two mouse/keyboard camera styles: behind the character, or a free orbit camera with
  camera-relative movement.
- Light and fog changes between camera cuts fade smoothly; effects, sound panning and culling use
  the third-person view.

### Actors (model swaps, voices)

- "You appear as": play as Claire, Chris, Steve, Wesker, costumes, or one of dozens of enemy and cast
  models retargeted onto the player skeleton; the choice applies to cutscene doubles too.
- Gender-matched animations: male models play the male equivalent of each motion.
- Replacement models hold weapons and the lighter in their own hands; shots leave from the muzzle.
- Optional talking faces for swapped models in cutscenes.
- Player voices follow the chosen appearance (Claire, Chris, Steve or Wesker voice banks).

### Enemies

- F2 enemy spawner for any type, loaded from its source room on demand with its sounds and
  effect textures.
- Enemy options: extra enemies, rearranged placements, randomizer with seed; synced to the co-op
  partner.
- Higher spawner limits (texture heap, spawn pool, helper slots).

### Maps and mods

- Mods folder (`RECVX_MODS`, default `./mods`) for replacement and new rooms.
- RDX room tooling: `rdx.py` (read and patch rooms), `rdxbuild.py` (build rooms from OBJ/PNG
  geometry, props, items, scripts, doors and sound banks), a "pc" budget for large rooms.
- Slope walking in rooms flagged for it, for terrain-based custom maps.
- Asset catalogue and kitbash tools to build new rooms from the player's own disc: the Quarantine
  Annex area and a village area as examples.
- RE4 village conversion tooling for a player's own RE4 export (materials mapped to CVX textures).
- Rooms built from game data must not be shared.

### Input and settings

- F1 settings overlay and a native PC SETTINGS page in the game's OPTION menu, both driven by one
  settings table and saved to `recvx.ini`.
- Rebinding for keyboard/mouse and gamepad (two slots per action).
- Mouse in the inventory and menus.
- Quick knife (hold Space / right trigger) and a trade request key.
- Skip intro and skip door animations (on by default on first start).
- F2 cheat menu: heal, items, infinite ammo, teleport, noclip with fly mode, enemy spawn.

### Tooling and tests

- ctest suites for the co-op and rendezvous protocols (including fuzzing), networking, VU,
  bindings, appearances, file I/O and movies.
- Determinism checks: `layout_check.sh` compares frames across binary layouts; replay-vs-executed
  frame checker; native vs emulated renderer comparison.
- Profiler (`--profile`), GPU timing, extensive `RECVX_*` debug and test switches.
- `decomp_warnings.py` regenerates the lists of missing returns and uninitialised locals.

### Fixes

- Decomp code made deterministic: a retail out-of-bounds motion key read and a stack-garbage near
  plane point replaced by the intended values.
- Ordering-table depth sorting fixed (transparency sorting was missing).
- MIDI volume fade-up no longer ends silent (follows the retail code's intent).
- C functions without a return checked against the retail ELF; several hand-written asm routines
  translated from the retail code.
- Idle screen dimming (the game's screen saver) is lifted by PC input, as a pad press does on PS2.
- Crash fixes: game over with a swapped model, lobby open/close, teleport during a grab, texture
  queue recursion, port mapping use-after-free.
- Graveyard effects drawn with the right textures after a door patch (room texture-list check).

Developed with AI coding agents (Claude; OpenAI Codex for parts of the map tooling), see README.

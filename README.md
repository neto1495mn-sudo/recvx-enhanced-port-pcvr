> [!IMPORTANT]
> **AI-assisted project.** Most of the code, tools and documentation in this repository were written by AI coding agents (Anthropic's Claude models, plus OpenAI Codex for parts of the map tooling), directed and reviewed by hand by the maintainer, who also tested the results in-game. Treat it like any other early-stage community project: expect bugs, read the code before relying on it, and report problems via issues.
> The underlying decompilation ([AshfordFamily/recvx-decomp](https://github.com/AshfordFamily/recvx-decomp)) is a separate, human-led project. Our changes to it live in a fork and are disclosed as AI-assisted, as that project asks.
> No game data is included. You need your own legally obtained copy of *Resident Evil Code: Veronica X* (PS2, NTSC-U, SLUS-20184).

# recvx-enhanced-recomp

## VR version (this branch)

[![Resident Evil Code: Veronica X | PC port in VR](https://img.youtube.com/vi/_2-NZcCqmSs/maxresdefault.jpg)](https://www.youtube.com/watch?v=_2-NZcCqmSs)

*Click the image to watch the VR video on YouTube.*

This `vr` branch adds a full PC VR mode (OpenXR; tested on a Quest 2 through Virtual Desktop)
to the port. Start the game with `--vr`; the VR settings are in `save/recvx.ini`.

This VR mode is built on [recvx-enhanced-recomp](https://github.com/aikatore/recvx-enhanced-recomp)
by **aikatore** (version 0.4.0, plus the game fixes of 0.5.2) and the [recvx-decomp](https://github.com/AshfordFamily/recvx-decomp)
decompilation by AshfordFamily. All credit for the PC port goes to them.

### How to play in VR

1. Download the zip from the [Releases](https://github.com/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases)
   page and unpack it anywhere.
2. Start your OpenXR runtime (tested only on a Quest 2 through Virtual Desktop / VDXR).
3. Run `PLAY_VR.bat`. On the first start, pick your own `.iso` of the game (PS2, NTSC-U, SLUS-20184).

**Controls (Touch controllers)**

| Button | What it does |
|---|---|
| Left stick | Walk (the way you look) |
| Left trigger, or click the left stick | Run |
| Right stick left/right | Snap turn (locked on stairs) |
| Right grip (hold) | Raise the gun and aim with your hand |
| Right trigger | Fire (with twin guns: the right gun) |
| Left trigger, while aiming twin guns | Fire the left gun |
| Left grip | Knife (cut by swinging it); at the belt: take an ammo box |
| A | Action: open doors, pick up items, examine, confirm |
| B | Cancel / back; with a gun in hand: drop the magazine |
| Y | Inventory (status screen) |
| X | Map |
| Hold left trigger + press Y | Select |
| Left menu button (three lines) | Start (pause, options) |
| Long press of the headset's system button | Recentre the view |

Walking around your room moves Claire too. A small "A" shows over what the action button
would use (a door, an item) when you are right in front of it.

**Reloading (by hand)**

The gun does not reload by itself: when it is empty it only clicks.

1. Press **B**: the magazine drops out of the gun in your hand.
2. Look down: an ammo box hangs at your belt, in front of your belly, with the rounds you have
   left. It is not shown when you have no ammo for that gun.
3. Bring your **left hand** to the box and squeeze the **left grip**: the box is now in your hand.
4. Keep the grip held and bring your left hand to the gun: it reloads (sound and vibration).

With an empty gun you can skip step 1.

**HD textures (optional)**

The game can use HD texture packs made for PCSX2 for this game (SLUS-20184). Put the pack's
files in the `textures` folder next to the game (any subfolder, e.g.
`textures/SLUS-20184/replacements`, as in PCSX2) and turn on `tex_hd = 1` in `save/recvx.ini`.
`tex_hd_mb` sets the video memory for them (MB). A list of file names in `textures/ignore.txt`
keeps those textures as the game's own. No texture pack comes with this mod.

**View and movement**
- Stereo 3D in first person, the camera in your head, at the headset's refresh rate (90 Hz and up).
- Characters and enemies smoothed to the headset's rate while the game logic stays at 30 FPS
  (`vr_interpolate`); the stick walking smoothed too (`vr_interp_body`).
- Room-scale: walking in your room moves Claire, and the game's walls stop her.
- Snap turn with the right stick (`vr_turn`, degrees), locked on stairs.
- Instant change of direction, no turning arc (`vr_move_direct`), and an optional steady camera
  that ignores the body's small animation moves (`vr_cam_steady`).
- Cutscenes, doors and menus on a cinema screen at eye height that follows you sideways; room
  changes fade out and in.

**Hands and weapons**
- Both hands follow the Touch controllers at the headset's rate; each gun aims where its hand points.
- Twin guns: each trigger fires its own gun.
- Laser sight: a faint red beam from the muzzle to a dot where the shot hits, hidden behind the
  gun, the hand and walls (`vr_sight`: 1 ring, 2 laser, 3 both).
- Walk while aiming (`vr_aim_walk`); letting go of the aim while walking walks at once.
- Recoil and controller vibration on every shot (`vr_recoil`, `vr_haptics`).

**Reloading by hand** (`vr_manual_reload`)
- No automatic reload: an empty gun only clicks.
- B drops the magazine; the left grip takes an ammo box from the holster (shown at your belt with
  the rounds left, gone when there are none) and you put it in the gun with your left hand.

**Knife**
- Left grip draws the knife; you cut by swinging, only with the edge leading, with a real swing
  (`vr_slash_speed`, cm/s).

**Combat feel**
- Damage by body part (head, body, arms, legs: `vr_dmg_*`).
- The view tilts when Claire is hit (`vr_hit_tilt`), and a heartbeat vignette in Danger (`vr_danger_fx`).

[![build](https://github.com/aikatore/recvx-enhanced-recomp/actions/workflows/build.yml/badge.svg)](https://github.com/aikatore/recvx-enhanced-recomp/actions/workflows/build.yml)

A native PC port of *Resident Evil Code: Veronica X* (PS2) for Linux and Windows, built from the
community decompilation [recvx-decomp](https://github.com/AshfordFamily/recvx-decomp). The
original game code and its PS2 platform layer are compiled for the PC; the PS2 hardware
underneath (GS, VU, DMA, sound driver, disc, memory card, pad) is emulated at its boundaries, and
native paths replace the expensive parts. The game reads your own disc image in place.

## Features

- **Enhanced renderer**: native 3D and textures on the GPU, widescreen (hor+, up to ultrawide),
  internal resolution up to 8x, in-between frames up to the display's refresh rate while the game
  logic stays at its original 30 FPS, a frame rate limit, optional PS2 motion trail.
- **Online co-op for two players**: direct, automatic or server-relayed connections, a lobby in
  the title menu, join codes and passwords, trading with item requests, shared or individual loot,
  saved partner inventories, rejoin after a crash. Includes a small C rendezvous/relay server.
- **Third-person mode** (optional): over-the-shoulder camera with mouse/keyboard controls, free
  aim and two camera styles.
- **Modern controls** (optional) for the fixed cameras: the stick walks where it points on screen,
  as in the RE HD remasters, and keeps its direction through camera cuts.
- **Model swaps**: play as other characters and creatures, with matching voices and cutscene
  doubles.
- **Enemy options**: extra enemies, rearranged placements, a seeded randomizer, and an enemy
  spawner.
- **Save files**: every save its own file, as many as you like, in the game's own save screens
  without the memory card screens (the emulated memory card stays available).
- **Enhanced settings** in the game's own OPTION menu and an F1 overlay; full rebinding of keyboard,
  mouse and gamepad; per-window input source for local co-op on one PC.
- **Cheats and debug tools** (F2), co-op window (F3).
- **Mods**: replacement room files and archive entries from a `mods/` folder, plus tools to build
  new rooms.
- **Tooling** for determinism checks, rendering comparisons and profiling.

## Requirements

- Your own disc image of *Resident Evil Code: Veronica X*, PS2 NTSC-U, **SLUS-20184** (`.iso`).
- **Linux**: an x86 PC; the game is a 32-bit (i686) program and needs glibc 2.34 or newer plus
  the 32-bit (multilib) OpenGL, X11/Wayland and audio libraries.
- **Windows**: Windows 10 or later (32-bit executable, runs on 64-bit Windows).
- A GPU with OpenGL 3.3.

## Download

Prebuilt Linux and Windows builds and the co-op server are on the
[Releases](https://github.com/aikatore/recvx-enhanced-recomp/releases) page (no game data
included). Unpack and run `recvx.exe` (Windows) or `./recvx` (Linux); see
[docs/USER_GUIDE.md](docs/USER_GUIDE.md).

## Building from source

```sh
git clone --recursive https://github.com/aikatore/recvx-enhanced-recomp.git
cd recvx-enhanced-recomp
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/linux-i686.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/pc/recvx
```

`--recursive` fetches the decompilation and its header submodules; the decomp's large compiler
submodule is skipped by design (the PC build does not use it). On the first start a file
selector asks for the disc image; the choice is remembered. The Windows build is cross-compiled
with mingw-w64: see [docs/BUILDING.md](docs/BUILDING.md).

## Documentation

| Document | For |
|---|---|
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | players: first start, settings, controls, cheats, saves, troubleshooting |
| [docs/COOP.md](docs/COOP.md) | players and developers: online and local co-op, protocol reference |
| [docs/SERVER.md](docs/SERVER.md) | running the co-op rendezvous/relay server |
| [docs/BUILDING.md](docs/BUILDING.md) | building on Linux and for Windows, CMake options, tests |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | developers: how the port works, renderer, determinism, test aids |
| [docs/THIRDPERSON.md](docs/THIRDPERSON.md) | third-person mode internals |
| [docs/ACTORS.md](docs/ACTORS.md) | models, skeletons, animation, model swaps |
| [docs/ENEMIES.md](docs/ENEMIES.md) | enemy data, spawner, enemy options |
| [docs/modding/](docs/modding/README.md) | room format and building custom rooms |
| [CHANGELOG.md](CHANGELOG.md) | feature history |
| [CONTRIBUTING.md](CONTRIBUTING.md) | how to contribute |

## Credits

- **[recvx-decomp](https://github.com/AshfordFamily/recvx-decomp)** by AshfordFamily and its
  contributors: the decompilation this port is built on (MIT).
- **[SkeletonPicture](https://github.com/SkeletonPicture)** for hosting the Katana SDK and CRI
  Middleware header repositories the decomp uses as submodules.
- Libraries: [SDL 3](https://github.com/libsdl-org/SDL) (zlib),
  [Dear ImGui](https://github.com/ocornut/imgui) (MIT),
  [miniupnpc and libnatpmp](https://github.com/miniupnp) (BSD-3-Clause),
  [Monocypher](https://github.com/LoupVaillant/Monocypher) (BSD-2-Clause or CC0). Versions and
  details: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

The code in this repository is released under the [MIT License](LICENSE). The decompilation in
`decomp/` is MIT-licensed by its authors; third-party components keep their own licenses (see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)).

## Legal

This is an unofficial fan project, not affiliated with or endorsed by Capcom, Sony Interactive
Entertainment, Sega or CRI Middleware. *Resident Evil* and *Code: Veronica* are trademarks of
Capcom; PlayStation is a trademark of Sony Interactive Entertainment. No game assets are
distributed: you must supply your own legally obtained copy of the game. Files you generate from
your copy (for example modded rooms) contain game-derived data and must not be shared.

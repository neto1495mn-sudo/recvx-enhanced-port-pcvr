> [!IMPORTANT]
> **AI-assisted project.** Most of the code, tools and documentation in this repository were written by AI coding agents (Anthropic's Claude models, plus OpenAI Codex for parts of the map tooling), directed and reviewed by hand by the maintainer, who also tested the results in-game. Treat it like any other early-stage community project: expect bugs, read the code before relying on it, and report problems via issues.
> The underlying decompilation ([AshfordFamily/recvx-decomp](https://github.com/AshfordFamily/recvx-decomp)) is a separate, human-led project. Our changes to it live in a fork and are disclosed as AI-assisted, as that project asks.
> No game data is included. You need your own legally obtained copy of *Resident Evil Code: Veronica X* (PS2, NTSC-U, SLUS-20184).

# Resident Evil Code: Veronica X – PC Port VR

**Resident Evil Code: Veronica X in VR on PC** – a VR mod for the native PC port of *Resident
Evil Code: Veronica X* (PS2). Play RE Code Veronica in first person with your hands as the guns:
PCVR through OpenXR, tested on Meta Quest 2 with Virtual Desktop (other OpenXR headsets not
tested yet). Also known as: RE CVX VR, Resident Evil Code Veronica VR mod, Code Veronica PCVR,
Code Veronica Quest 2.

## VR version (this branch)

[![Resident Evil Code: Veronica X | PC port in VR](https://img.youtube.com/vi/_2-NZcCqmSs/maxresdefault.jpg)](https://www.youtube.com/watch?v=_2-NZcCqmSs)

*Click the image to watch the VR video on YouTube.*

This `vr` branch adds a full PC VR mode (OpenXR; tested on a Quest 2 through Virtual Desktop)
to the port. Start the game with `--vr`; the VR settings are in `save/recvx.ini`.

This VR mode is built on [recvx-enhanced-recomp](https://github.com/aikatore/recvx-enhanced-recomp)
by **aikatore** (version 0.4.0, plus the game fixes of 0.5.2) and the [recvx-decomp](https://github.com/AshfordFamily/recvx-decomp)
decompilation by AshfordFamily. All credit for the PC port goes to them.

### How to play in VR

1. Download `RE-Code-Veronica-X-PC-Port-VR-v1.0-windows.zip` from the [Releases](https://github.com/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases)
   page and unpack it anywhere.
2. Start your OpenXR runtime (tested only on a Quest 2 through Virtual Desktop / VDXR).
3. Run `PLAY_VR.bat`. On the first start, pick your own `.iso` of the game (PS2, NTSC-U, SLUS-20184).
4. Optional, **HD textures**: download the [texture pack (SLUS-20184.zip)](https://disk.yandex.ru/d/_ILWwt29zG2bDw), unpack it into the
   `textures` folder (see "HD textures" below).

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

**How the game works in VR**

- **Where you are**: you see the game in first person from Claire's (or Chris's, Steve's) eyes.
  The left stick walks the way your head looks; the right stick turns you in steps.
- **Aiming**: hold the **right grip** to raise the gun. The gun follows your hand: point and pull
  the trigger. The red laser shows where the shot will hit.
- **Twin guns** (the dual pistols): hold the right grip, then each trigger fires its own gun.
  A quick pull during the last shot is kept and fired as soon as the game allows it.
- **Knife**: squeeze the **left grip** to hold it and swing it like a real knife, edge first.
  A slow move does not cut.
- **Doors, cutscenes and menus** are shown on a big screen in front of you at eye height. Use the
  stick and A/B as on a gamepad there. The 3D comes back by itself when you can play again.
- **Stairs**: the snap turn is off while Claire goes up or down stairs; just keep the stick pushed.
- **Lost your forward direction?** Long-press the headset's system button (Oculus button) to
  recentre.
- **Sitting or standing**: both work. Walking in your play area moves Claire too, and the game's
  walls stop her.

**HD textures (optional)**

The game can show HD textures made by the community for the **PCSX2** (PS2 emulator) version of
this game. You have to get a pack yourself; none comes with this mod.

**Download an HD texture pack:** [SLUS-20184.zip on Yandex Disk](https://disk.yandex.ru/d/_ILWwt29zG2bDw) – a community PCSX2 texture
pack for this game (not made by this mod). Open the link, click **Скачать** (Download), unpack the
zip and put its folder inside `textures` (steps 2 to 5 below).

1. **Find a pack**: it must be an HD texture pack **for PCSX2** for *Resident Evil Code: Veronica X*
   **NTSC-U, SLUS-20184** (the same disc as your `.iso`). Packs for other versions (PAL, Japanese,
   Dreamcast, PS3/Xbox HD) do not match. The pack linked above is one of them.
2. **Check the files**: a PCSX2 pack is a folder of `.dds` or `.png` images with long hexadecimal
   names such as `104bcba3e277e448-ac18592ba12c29a2-00002654.dds`. Do not rename them: the name is
   how the game finds each texture.
3. **Copy them**: put the pack's folder inside the `textures` folder next to `recvx_vr.exe`. Any
   subfolder works, e.g. `textures\SLUS-20184\replacements\...` exactly as in PCSX2, or the
   pack's own folder name.
4. **Turn them on**: in `save\recvx.ini` set `tex_hd = 1` (already on in this release).
5. **Start the game**: loading the list of files takes a few seconds the first time.

Settings in `save\recvx.ini`:

| Setting | What it does |
|---|---|
| `tex_hd` | 1 = use the HD textures, 0 = the game's own |
| `tex_hd_mb` | Video memory for HD textures, in MB (default 600; less if the game stutters, more on a big GPU) |
| `tex_hd_max` | Largest texture size kept, e.g. 1024 or 2048 (bigger ones are reduced: less memory, smoother) |

To check it worked, start with `--log` and look in `recvx.log` for a line like
`tex: 6568 replacement textures found`. A texture you do not like can be kept as the game's
own: write its file name (one per line) in `textures\ignore.txt`.

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

## The PC port (recvx-enhanced-recomp by aikatore)

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

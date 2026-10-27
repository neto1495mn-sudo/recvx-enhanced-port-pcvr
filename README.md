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

1. Download `RE-Code-Veronica-X-PC-Port-VR-v1.4.1-windows.zip` from the [Releases](https://github.com/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases)
   page and unpack it anywhere.
2. Start your OpenXR runtime (tested only on a Quest 2 through Virtual Desktop / VDXR).
3. Run `recvx_vr.exe` (or `PLAY_VR.bat`). On the first start, pick your own `.iso` of the game (PS2, NTSC-U,
   SLUS-20184); then the launcher opens: press **PLAY**.
4. Optional, **HD textures**: download the [texture pack (SLUS-20184.zip)](https://disk.yandex.ru/d/_ILWwt29zG2bDw), unpack it into the
   `textures` folder (see "HD textures" below).
5. Before playing, set the comfort options in the title screen: **OPTION → ENHANCED SETTINGS → VR**
   (see "The VR page" below).

When a newer version is out, the game says so when it starts (see "Update check" below).

### What's new in 1.4.1

- **Quick knife back on the left grip**: squeeze it with a free left hand. It does not come out when the
  left hand is on the gun, holding an ammo box, at the belt to take a box, or while you aim.
- **X opens the map** again (the right stick click still does too).

### What's new in 1.4

**Guns and aiming**
- **Hold the gun with both hands**: bring your left hand to the gun and squeeze the left grip.
  Pistols and revolvers: the left hand just holds on (pull it about 10 cm away to let go). Big guns
  (assault rifle, shotgun, grenade launcher...): the left hand aims the gun with you. Grabbing the gun
  no longer twists it, and the hands blend in smoothly.
- **Aim fixed**: every gun fires along its own barrel. The shot, the laser, grenades and crossbow bolts
  leave the muzzle where the sight starts. Guns no longer aim low with one hand or bend when you aim
  down or at something close, and the grenade launcher no longer fires low or crooked.
- **No more aim help**: crossbow bolts no longer curve towards the nearest enemy and no longer leave a
  little to the left; they always drop a little, the same way every time.
- **Recoil per gun**, stronger with one hand than with two: handgun, shotgun and grenade launcher kick
  hard one-handed; the assault rifle climbs and sways when fired with one hand.
- **Shotgun**: a thin round reticle ( + ) instead of the laser. **Sniper rifle**: no laser; zoom with
  the right stick (forward / back) in the scope, as in the game.
- **Head shots**: blood from the exact spot you hit; a head shot that kills a common zombie bursts its
  head. Head damage is now 115%.

**Knife and controls**
- **Quick knife on X** (hold); **map on the right stick click**; left trigger + Y no longer does Start
  (Y is always Y; the menu button is Select).
- **Knife**: one hit per swing (a swing used to hit up to 4 times), a hit on a wall ends the swing, no
  longer goes through walls on stairs; fixed a freeze where Claire kept walking in place.

**HD textures and picture**
- **HD textures load in the background**: the files are read on several threads and sent to the video
  card on a thread of its own, so entering a room no longer stutters.
- HD textures: the equipped weapon no longer shows the wrong (golden) picture and the inventory
  letters no longer show through the items.
- Fixed resolution by default (dynamic resolution is an option in the launcher).

**Other fixes**
- Movies that cannot be decoded (some dubbed discs) are skipped instead of freezing the game.

### What's new in 1.3.1

- **Smooth turn is truly smooth**: it follows the headset's own timing.
- **Select menu** in game: Continue, Options or Return to title; **Exit** on the title screen.
- **Title and options in Portuguese** discs show MULTIPLAYER, SAIR and MAIS AJUSTES correctly.
- **Poison no longer sways the view**.
- **Update button** in the launcher is now green and easy to spot.
- Closing the game no longer freezes the window; new game icon.
- Movie sound follows the pause; co-op guest no longer crashes against Alexia's second form.

### What's new in 1.3

- **Fan translations show their text right**: the letter widths now come from your own disc, so
  accented letters (e.g. Portuguese "Não") no longer leave a gap.
- **Crossbow**: the bolt flies where the laser points (it used to leave along the body's direction).
- **Life 0 is still alive** (as in the game): at 0 the VR movement, danger/poison edges and the knife
  no longer switch off (the stick used to turn into tank controls).
- The update check understands versions like 1.3.1.

### What's new in 1.2

- **A launcher** opens when you start the game: **PLAY** goes straight into VR; **OPTIONS** has the VR,
  graphics and texture settings. It speaks **English, Portuguese or Spanish** (EN · PT · ES at the top
  right; English the first time, then the one you chose). Put a `launcher.png` next to the game for a
  picture on its left (none is included). "Show this launcher when the game starts" turns it off.
- **No console window** next to the game any more (the log still goes to `recvx.log` with `--log`).
- **Smooth turn is really smooth**: it turns at the headset's rate (90 Hz on a Quest 2), by the clock,
  about your head, and no longer stops and jumps when a game frame comes a little late.
- **The PC monitor shows your left eye** (it showed the game's own camera). The third-person crosshair
  is no longer drawn in VR.
- **The action icon**: an arrow from a little away, the **"A" button** when pressing it works now; at the
  height of the middle of a door, over the thing itself, and it no longer moves with you.
- **Magazines**: B drops **the game's own magazine** (it falls to the floor, with its sound). Bringing the
  ammo box to the gun with the magazine still in **tops the gun up** (a full gun: the box is just put
  away). A light **"tick-tick"** on the gun's controller when it is loaded. After taking the box, the knife
  only comes out once you let go of the grip and squeeze it again.
- **The ammo count at the belt** is "in the magazine / in your boxes" (12/50; magazine out: 0/62), in front
  of the box and steady while you walk or turn.
- **The handgun bullets' box** at the belt and in your hand looks like the game's own box (with the HD
  texture pack). The other ammo keeps the plain box for now.
- **The handgun kicks like a 9 mm**: a sharp snap up and back; the other weapons as before.
- **Controls**: the left **menu button (three lines) is Select**; **left trigger + Y is Start**. **Run** like
  Call of Duty: click the left stick once while walking forward.

Not tested in the headset yet: the handgun box picture, the handgun's kick, the ammo count, the
Select/Start swap and running with one click. Fan translations (e.g. Portuguese) still show broken
text in game: the translated letters' widths are not read from the disc yet (planned).

### What's new in 1.1

- **The VR page** in the game's menu (title screen and in game, also F1): snap turn, comfort
  vignette, sight, resolution.
- **Comfort vignette**: dark edges only while you walk with the stick or turn (Off, Medium, Strong).
- **Hits you can feel**: the controller on the side of the hit vibrates (front: one knock, behind:
  two); held by a zombie, the vibration follows its bites, "check, check", and only the one holding you.
- **The zombie no longer goes through your face**: while it holds or hits you, the camera moves back,
  away from its body, smoothly and never into a wall (`vr_close_push`, cm; `vr_bite_push` adds a small
  push on each bite, 0 = off).
- **Life colours**: a yellow (Caution) or orange flash at each hit, the red edge in Danger, a
  **purple** edge when poisoned (red and purple by turns, softer, when both), with a **heartbeat
  sound** (`vr_heart_sound`, 0 = off; put a `heartbeat.wav` next to the game to use your own).
- **Ammo fixes**: a magazine taken out with B only goes back to the same gun (a gun could get 200
  rounds after Steve's part); B only drops the magazine when there is ammo to reload with (Steve's
  Lugers lost their rounds); B again puts a stuck magazine back.
- **Exit to desktop** in OPTION → ENHANCED SETTINGS → GAMEPLAY.
- **Twin guns**: a quick pull during the last shot is kept and fired as soon as the game allows it.
- **Stairs**: the view no longer turns by itself; only the snap turn is locked.
- **OBS / VR overlays**: a broken OpenXR layer no longer drops the game to the desktop.
- **HD textures**: lighter stutter on entering a room (`tex_hd_per_frame`, see below).
- **Update check** at start.
- Game fixes from aikatore's port 0.5.2 (no crash in four rooms with the free camera, the item box,
  an enemy bug).

### The VR page

In the title screen (or in game): **OPTION → ENHANCED SETTINGS**, the first page, **VR**:

| Option | Choices |
|---|---|
| Snap turn | Smooth, 15, 30, 45, 60 or 90 degrees (right stick) |
| Comfort vignette | Off, Medium, Strong – dark edges only while walking with the stick or turning |
| Sight | Laser, Dot (a small ring where the shot hits), Off |
| Resolution | 50% to 200% of what the headset recommends |

The other VR options stay in `save/recvx.ini`.

### Update check

When the game starts it asks GitHub for the latest release of this project (one request, nothing
about you is sent; at most a few seconds, nothing happens without a network). If a newer version is
out, the launcher shows it in yellow (click it to open the download page; with the launcher off, a window
on the PC offers it). Set `update_check = 0` in
`save/recvx.ini` to turn it off.

**Controls (Touch controllers)**

| Button | What it does |
|---|---|
| Left stick | Walk (the way you look) |
| Click the left stick while walking forward | Run (keeps running until you stop pushing forward or click again) |
| Left trigger (hold) | Run |
| Right stick left/right | Snap turn (locked on stairs) |
| Right grip (hold) | Raise the gun and aim with your hand |
| Left grip, with the left hand at the gun | Hold the gun with both hands |
| Right trigger | Fire (with twin guns: the right gun) |
| Left trigger, while aiming twin guns | Fire the left gun |
| Left grip (hold), free left hand | Knife (cut by swinging it) |
| Left grip at the belt | Take an ammo box |
| A | Action: open doors, pick up items, examine, confirm |
| B | Cancel / back; with a gun in hand: drop the magazine |
| Y | Inventory (status screen) |
| X or right stick click | Map |
| Right stick forward / back, in the sniper scope | Zoom in / out |
| Left menu button (three lines) | Select |
| Long press of the headset's system button | Recentre the view |

Walking around your room moves Claire too. An arrow shows over a door you look at from a little
away; the "A" button over what the action button would use (a door, an item) when pressing it works.

**Reloading (by hand)**

The gun does not reload by itself: when it is empty it only clicks.

1. Press **B**: the magazine drops out of the gun in your hand.
2. Look down: an ammo box hangs at your belt, in front of your belly, with the rounds you have
   left. It is not shown when you have no ammo for that gun.
3. Bring your **left hand** to the box and squeeze the **left grip**: the box is now in your hand.
4. Keep the grip held and bring your left hand to the gun: it reloads (sound and a "tick-tick" on the gun).

With an empty gun, or to top it up with the magazine in, you can skip step 1.

**How the game works in VR**

- **Where you are**: you see the game in first person from Claire's (or Chris's, Steve's) eyes.
  The left stick walks the way your head looks; the right stick turns you in steps.
- **Aiming**: hold the **right grip** to raise the gun. The gun follows your hand: point and pull
  the trigger. The red laser shows where the shot will hit.
- **Twin guns** (the dual pistols): hold the right grip, then each trigger fires its own gun.
  A quick pull during the last shot is kept and fired as soon as the game allows it.
- **Knife**: hold the **left grip** to hold it and swing it like a real knife, edge first.
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

**HD textures in the background.** The pack's images are read on several threads while you play
and sent to the video card on a thread of its own, so entering a room no longer stutters; a texture
may show the game's own picture for a moment before the HD one appears. The sending thread is on by
default on NVIDIA and AMD cards (launcher OPTIONS → "Upload in the background": Automatic / On / Off;
`tex_upload_thread` in `save\recvx.ini`). If the game still stutters:

| Setting | Effect |
|---|---|
| `tex_hd_max` (e.g. 1024) | Bigger images are reduced to this size: less to read and send. |
| `tex_hd_mb` | Video memory for HD textures; less if the stutter comes back while playing. |
| `tex_hd = 0` | No HD textures: to check whether a stutter comes from them. |

To check it worked, start with `--log` and look in `recvx.log` for a line like
`tex: 6568 replacement textures found`. A texture you do not like can be kept as the game's
own: write its file name (one per line) in `textures\ignore.txt`.

**If the game starts on the desktop instead of the headset**

- Start your OpenXR runtime first (Virtual Desktop / VDXR, Meta Quest Link or SteamVR set as the active OpenXR runtime).
- `recvx.log` saying `xrCreateInstance ... failed`: another program's OpenXR layer is in the way. Since this version a
  layer that fails to load is skipped, so this should no longer stop VR (it did with OBS running). If it still happens,
  close OBS and other VR overlays, or use the free *OpenXR API Layers GUI* to see and disable broken layers.

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
- The left grip draws the knife; you cut by swinging, only with the edge leading, with a real swing
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

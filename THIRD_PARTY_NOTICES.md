# Third-party notices

This repository contains only the PC port's own code (MIT, see `LICENSE`). It
contains no game data: you need your own copy of Resident Evil Code: Veronica X
(PS2, US, SLUS-20184). Resident Evil and Code: Veronica are trademarks of
Capcom; this project is not affiliated with or endorsed by Capcom, Sony, Sega
or CRI Middleware.

## Git submodules (fetched, not included in this repository)

| Component | Path | Source | License |
|---|---|---|---|
| recvx-decomp (decompilation of the game), `pc-port` branch of a fork | `decomp/` | https://github.com/aikatore/recvx-decomp (upstream https://github.com/AshfordFamily/recvx-decomp) | MIT, Copyright (c) 2024-2026 AshfordFamily; see `decomp/LICENSE` |
| Sega Katana (Dreamcast) SDK headers | `decomp/include/recvx-decomp-katana` | https://github.com/SkeletonPicture/recvx-decomp-katana, a submodule of the decomp | third-party, proprietary (Sega); not redistributed here |
| CRI Middleware (ADX/Sofdec) headers and PS2 libraries | `decomp/include/recvx-decomp-cri` | https://github.com/SkeletonPicture/recvx-decomp-cri, a submodule of the decomp | third-party, proprietary (CRI Middleware); not redistributed here. The PC build uses only the headers, not the `.a` libraries |
| Metrowerks CodeWarrior compiler (`recvx-decomp-mwcc`) | `decomp/include/recvx-decomp-mwcc` | submodule of the decomp | not fetched by default (`update = none`) and not used by the PC build |

## Fetched by CMake at configure time (FetchContent)

| Component | Version | Source | License |
|---|---|---|---|
| SDL 3 (static; skipped with `-DRECVX_VENDORED_SDL=OFF`, which uses the system SDL3) | release-3.4.16 | https://github.com/libsdl-org/SDL | zlib |
| Dear ImGui | v1.92.9b | https://github.com/ocornut/imgui | MIT |
| miniupnpc | miniupnpc_2_3_3 | https://github.com/miniupnp/miniupnp | BSD-3-Clause |
| libnatpmp | 134fc89e | https://github.com/miniupnp/libnatpmp | BSD-3-Clause |
| Monocypher | 4.0.2 | https://github.com/LoupVaillant/Monocypher | BSD-2-Clause or CC0-1.0 (dual) |

The built binaries link these libraries statically; ship their license texts
with any binary release.

## Vendored single-file libraries (`pc/third_party/`)

| Component | Version | Source | License |
|---|---|---|---|
| xxHash (`xxhash.h`, XXH3 for PCSX2-compatible texture names) | v0.8.3 | https://github.com/Cyan4973/xxHash | BSD-2-Clause |
| stb_image (`stb_image.h`, PNG texture replacements) | v2.30 | https://github.com/nothings/stb | MIT or public domain (dual) |

## Snapdragon Game Super Resolution 1 (SGSR)

The super resolution shader in `pc/src/gs/gs_gl.c` (`sgsrDelta`, `fastLanczos2`, `weightY`) is `sgsr1_shader_mobile.frag` from https://github.com/SnapdragonGameStudios/snapdragon-gsr, adapted to this renderer.

Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved. SPDX-License-Identifier: BSD-3-Clause.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

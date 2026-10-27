# Contributing

Bug reports, fixes and features are welcome. Please open an issue first for larger changes.

## Build and test

See [docs/BUILDING.md](docs/BUILDING.md). Before sending a pull request:

```sh
cmake --build build && ctest --test-dir build --output-on-failure
```

Changes to rendering or game logic should keep frames byte-identical where intended: run
`pc/tools/layout_check.sh` (needs your disc image, see
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#determinism)). Changes to the map tools should pass
their Python tests.

## Where changes go

- **This repository**: everything under `pc/` (platform layer, glue, server, tools, tests),
  `cmake/` and the docs.
- **The decompilation** (`decomp/`) is a submodule pointing at the `pc-port` branch of
  [aikatore/recvx-decomp](https://github.com/aikatore/recvx-decomp), a fork of
  [AshfordFamily/recvx-decomp](https://github.com/AshfordFamily/recvx-decomp). Changes to game
  code go there as pull requests against `pc-port`, and every change must be guarded by
  `#ifdef TARGET_PC` with the PS2 original kept under `#else`, for example (`en23.c`):

  ```c
  #ifdef TARGET_PC   /* the PS2 original reads *key++ three times unsequenced in one expression */
      njRotateXYZ((NJS_MATRIX*)epw->exp0, mkaP->key[0], mkaP->key[1], mkaP->key[2]);
  #else
      njRotateXYZ((NJS_MATRIX*)epw->exp0, *mkaP->key++, *mkaP->key++, *mkaP->key++);
  #endif
  ```

  Prefer a hook into `pc/glue` over larger edits in the decomp. Then update the submodule
  pointer here.

## Guidelines

- Keep the game logic at its original behaviour unless a change is an opt-in setting.
- New settings: one entry in `g_settings[]` plus an ini key ([docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#settings)).
- Document user-visible features in the matching doc under `docs/`.
- **AI assistance**: say in the pull request if AI tools wrote substantial parts of it. The
  upstream decompilation asks for the same disclosure on any submission that reaches it.
- **No game data**: never commit disc images, files extracted from the game, generated room
  files, screenshots of copyrighted content beyond what is needed for a bug report, or anything
  derived from other games' assets.

# Shandalar SDL3 Opening Menu Port Plan

## Summary

Add Shandalar as an SDL target focused on the New Game opening-menu path:
opening menu, difficulty, color, in-process facemaker, and name entry. Use
`facemaker_core` as a static library for now, and only revisit shared-code
splitting if the wasm size or duplicate-symbol issues force it.

## Key Changes

- Keep the copied Shandalar source under `src/shandalar`, excluding generated
  build outputs.
- Add a `shandalar` executable target for Windows x86 clang/MSVC and
  Emscripten wasm32.
- Link `shandalar` against `facemaker_core`, `shandalar_platform`, `rpbits`,
  `drawcard_facemaker`, `cardart_facemaker`, SDL3, and SDL3_ttf.
- Avoid compiling facemaker/shared source files directly into `shandalar`
  unless a missing symbol requires it; prefer the static-library path first.
- Compile only the opening-menu subset: `openingMenu.c`, `iniFile.c`,
  `shandalar_global_strings.c`, plus a small Shandalar runtime/host source
  containing required legacy globals/helpers copied from old `shandalar.c`.
- Replace Windows/GDI use in place through `platform/gdi_compat.h`; extend the
  existing shim only for missing calls needed by the menu path.
- Add an SDL callback host for `shandalar` with logical resolution rendering,
  nearest scaling, SDL mouse/key translation into existing globals/queues,
  palette-indexed frame conversion, and wasm asset preload support.
- Stub non-milestone behavior: Start/New Game continues through setup and stops
  before world/gameplay; Load/Resume do not enter gameplay; Exit closes the SDL
  app.

## Facemaker Integration

- Replace `_spawnl("Facemaker.exe", "/S")` in `RunFacemakerFlow()` with an
  in-process call to `facemaker_core`.
- Add a minimal embedded facemaker entrypoint if the current frame/input/status
  API is not enough for modal use from Shandalar.
- Let facemaker produce the existing `face.pic`-style portrait output, then
  reuse Shandalar's existing post-processing path for portrait sprites, pedestal
  rendering, and name entry.
- Do not split `facemaker_core` into smaller shared libraries in this pass.

## Tests

- Build `shandalar` with `clang.bat` for 32-bit Windows.
- Build `shandalar` with `emscripten.bat` for wasm32.
- Check wasm output size after linking; only revisit direct-source/static-lib
  structure if size is unexpectedly large or duplicate symbols block linking.
- Manual acceptance: opening menu renders, controls respond, New Game reaches
  difficulty/color/facemaker/name entry, facemaker returns in-process, and the
  app stops before ingame/world logic.

## Assumptions

- First milestone is New Game path only.
- `SHANDALAR_ASSET_DIR` points at Magic program data.
- Using `facemaker_core` as a static lib is preferred unless it causes a
  concrete build or size problem.
- No parallel `sdl_*.c` menu implementation; SDL-specific work goes into the
  host, existing Shandalar code, or shared compatibility shims.

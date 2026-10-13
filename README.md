# Shandalar SDL3
A port of the 1997 video game [Magic: the Gathering](https://en.wikipedia.org/wiki/Magic:_The_Gathering_(1997_video_game)) by MicroProse (Version 1.3, English). It aims to be as accurate as possible, starting from a (somewhat) byte-matched decompilation of the original binaries.

This repository does **not** contain any game assets. An existing copy of the game is required.  Since it aims to match the original as much as possible, modded assets will probably **not** work.

A lot has been decompiled with AI assistance.  The entire GDI32/Win32 to SDL shim layer is also written almost entirely by AI, as well as the asset streaming code for the Emscripten build.

## Building
Two batch files are available for building the currently tested toolchains.  They expect their respective toolchains to be installed at their default locations.

* `clang.bat` Builds a monolithic 32bit .exe using LLVM and MSVC's STL.
* `emscripten.bat` Builds a 32bit WebAssembly binary.  This is batteries-not-included, so a harness is needed to also serve assets properly.  Currently requires `emsdk_env.bat` to be ran in advance.

## Changes for SDL3/Emscripten
To support compiling with Emscripten several technical changes were necessary:
* Shim GDI32/Win32 to adapt to SDL3.
    * Uses [Dear ImGui](https://github.com/ocornut/imgui) to implement standard Windows controls.
    * Convert `.rc` files to vanilla `.c` code.
    * Uses a `registry.ini` file to emulate Windows registry.
* Avoid using threads, relying instead on Asyncify.  This requires adding explicit yields in several spots.
* Fixes implicit animations (town load zoom, world/lair/dungeon reward "static", deck shuffle effect, etc) via explicit yields.
* Switch dynamic linking/LoadLibrary to static linking.
    * Overrides some shared functions/globals that were originally duplicated.
    * Re-implements "exports"/`GetProcAddress` for deckdll.
    * Explicitly inits dlls since `DllMain` no longer exists.
* Rework Facemaker to avoid subprocess with shared memory via `CreateFileMappingA`.
* Fix text file parsing to be line-ending agnostic.
* Tiered asset loading via `shandalar.data`, `shandalar-assets.data`, and `emscripten_fetch` to provide a fast startup.
* Convert statwin assets to more convenient formats (`.bmp` -> `.png`, `.avi` -> `.webp`+`.wav`).

## Changes for mobile
To support touchscreens (no right click, no keyboard) several UI changes were made:
* Adding buttons to open right click context menus.  Needed in the main adventure UI, deckbuilder, and duel UI.
* Add Yes/No buttons where only Y/N keys are accepted.
* Rework text menus to avoid closing from errant clicks, and increase spacing between items.
* Tweak long press/double click in several spots to act as context menus.
* Switch several actions to trigger on mouse/touch Up rather than Down.
* Add drop target in deckbuilder to show sell value and allow selling cards.

## License

Original contributions to Shandalar SDL3 are licensed under the [MIT License](LICENSE), to the extent the contributors hold rights in them. This grant does not cover the original game's assets or trademarks; their rights remain with their respective owners. Third-party code retains its own licenses.

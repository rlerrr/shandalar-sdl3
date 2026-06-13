# Facemaker SDL3 Port

## Current boundary

The active build contains only Facemaker and the portions of cardartlib,
drawcardlib, and RPBits needed to load and display the Facemaker background.
`facemaker_core` is a static library with an event/step/frame API. The SDL
executable is a thin callback-based host for Windows and Emscripten.

The logical render area is capped at 1024x768. Window resizing only changes
letterboxing; it does not change the Facemaker surface.

## Compatibility inventory

| Win32/GDI surface | SDL3 compatibility implementation | Difficulty/risk |
| --- | --- | --- |
| `HDC`, `HBITMAP`, `CreateDIBSection`, `SelectObject`, `GetObjectA` | Indexed in-memory surfaces | Low; implemented |
| `BitBlt`, `StretchBlt`, `SetDIBitsToDevice` | Software indexed-pixel copies/scaling | Medium; implemented for active raster operations |
| `GetPixel`, `SetPixelV`, `FillRect`, lines, rectangles, ellipses | Software raster operations | Medium; implemented, visual parity still needs broader UI coverage |
| `HPALETTE`, `CreatePalette`, palette entry APIs, DIB color tables | 256-entry software palettes | Low; implemented |
| GDI fonts and text metrics/output | SDL_ttf-backed font objects and rendering | High; shim exists but the Facemaker text/UI path is not active yet |
| `MessageBoxA` | `SDL_ShowSimpleMessageBox` | Low; implemented |
| `GlobalAlloc` family and device-cap queries | Portable allocation and surface metadata | Low; implemented |
| Critical-section APIs | Removed from the active target | Low; Facemaker currently has one event-loop thread |
| `CreateFileMappingA` IPC/shared surfaces | Removed | Medium; future SHANDALAR integration must exchange files or call `facemaker_core` directly |
| Win32 message loop, worker thread, blocking wait functions | SDL callback host plus `FacemakerStatus` state API | High; shell implemented, original editor workflow still needs conversion |
| Display-mode changes | Removed | Low; fixed logical surface with letterboxing |

## Build outputs

- `clang.bat`: clang-cl, Windows x86, `MinSizeRel`
- `msvc.bat`: modern MSVC, Windows x86, `MinSizeRel`
- `emscripten.bat`: Emscripten HTML/WASM, `MinSizeRel`, preloads `FaceArt`

The first incremental editor workflow is active: it loads the original `fb*`
sprite sets, composes the selected face, supports mouse and keyboard controls
for face sets and layer variants, exports `face.pic`, triggers a browser
download on Emscripten, and exits through the library status API. Remaining work
includes persistent browser storage, richer save naming, and visual parity for
the original generated buttons.

Control geometry, `FaceData.txt` parsing, and `fb*` sprite blob parsing/grouping
are shared with the legacy source. The SDL editor intentionally replaces only
the Win32 worker-loop, dialog, hit-testing, and generated-button rendering
portions of `main.c`.

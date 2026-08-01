# Tools

This directory contains small utilities for automating parts of the decompilation.

## `cardsdat_dump`

Reads `cards.dat` and writes a CSV dump.

## `carddata_dump`

Reads `card_data_t` records from `magic.exe`/`shandalar.exe` and writes a CSV dump.

## `stubgen`

Reads CSV output from `cardsdat_dump` and `carddata_dump` combining them into stub `card_*` functions in a `.c` and `.h` with `reccmp` annotations.

## `mokinc.asm`

Struct/Enum definitions (in asm of course) from Mok's post at https://slightlymagic.net/forum/viewtopic.php?f=56&t=5815&start=15

## `build_shandalar_asset_bundle.ps1`

Builds the optional, asynchronously loaded `shandalar-assets.data` and
`shandalar-assets.js` files outside of CMake. Run it after the Shandalar WASM
output directory has been created:

```powershell
.\tools\build_shandalar_asset_bundle.ps1
```

Use `-SourceDir`, `-OutputDir`, `-EmscriptenRoot`, or `-Python` to override
the local defaults.

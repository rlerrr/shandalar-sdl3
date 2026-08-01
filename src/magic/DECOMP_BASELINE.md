# Magic/Duel source baseline

The original Duel and card implementation in this directory is synchronized
from:

- Repository: `C:\dev\shandalar-decomp`
- Commit: `4068e1b41aa28c81390b20abdef33306d2710022`
- Source list: `duel_sources.cmake`

The standalone `magic/src/main.c` and MSVC-only compatibility header are
intentionally excluded. Shandalar owns the host entry point. The
`exe_stubs*.c` files are included: despite their provisional names, they
contain decompiled game implementations required by Duel.

Shared files that already existed in the SDL tree were not overwritten. They
retain the SDL portability, legacy-I/O, and shared-global ownership changes
made before the full Duel import.

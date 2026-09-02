# Asset conversion tools

This directory contains utilities for preparing original game assets for the
SDL3 ports.

## `convert_statwin_assets.ps1`

Converts the original Statwin media into the `Statwin_SDL` layout consumed by
the SDL3 implementation and its asset test.

## `convert_wavs_to_ima_adpcm.ps1`

Recursively converts WAV assets to IMA ADPCM while preserving their paths.

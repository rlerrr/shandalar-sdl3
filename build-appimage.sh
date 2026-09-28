#!/usr/bin/env bash
# Build the current checkout and package it with the sibling game assets.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
asset_dir=${SHANDALAR_ASSET_DIR:-$repo_dir/../shandalar-assets}
build_dir="$repo_dir/build-appimage"
app_dir="$repo_dir/AppDir"
tool_dir="$repo_dir/tools-appimage"

if [[ ${1:-} == --help ]]; then
  echo "Usage: SHANDALAR_ASSET_DIR=/path/to/assets $0"
  echo "Builds the current checkout into a Shandalar AppImage in the repository root."
  exit 0
fi
if (( $# != 0 )); then
  echo "Unexpected argument: $1" >&2
  exit 2
fi

for command in cmake ninja clang clang++ python3 curl; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "Missing required command: $command" >&2
    exit 1
  fi
done
if ! python3 -c 'from PIL import Image' >/dev/null 2>&1; then
  echo "Missing Python Pillow; install python3-pil or use: python3 -m pip install pillow" >&2
  exit 1
fi
if [[ ! -d $asset_dir ]] || [[ -z $(find "$asset_dir" -mindepth 1 -print -quit) ]]; then
  echo "Assets directory is missing or empty: $asset_dir" >&2
  exit 1
fi

mkdir -p "$tool_dir"
for tool in linuxdeploy linuxdeploy-plugin-appimage; do
  if [[ ! -x "$tool_dir/$tool" ]]; then
    # Both release repositories live under the linuxdeploy organization.
    curl -fL --retry 3 -o "$tool_dir/$tool" \
      "https://github.com/linuxdeploy/$tool/releases/download/continuous/${tool}-x86_64.AppImage"
    chmod +x "$tool_dir/$tool"
  fi
done

cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX="$app_dir/usr" \
  -DSHANDALAR_BUILD_FACEMAKER=OFF \
  -DSHANDALAR_BUILD_SHANDALAR=ON \
  -DSHANDALAR_BUILD_DECK=OFF \
  -DSHANDALAR_BUILD_TESTS=OFF
cmake --build "$build_dir" --parallel

# Clear the staging tree so old binaries, libraries, or assets cannot leak in.
if [[ -d $app_dir ]]; then
  find "$app_dir" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
fi
cmake --install "$build_dir"
mkdir -p "$app_dir/usr/bin" "$app_dir/usr/share/shandalar/assets"
mv "$app_dir/usr/shandalar" "$app_dir/usr/bin/shandalar"
cp -a "$asset_dir/." "$app_dir/usr/share/shandalar/assets/"
python3 - "$repo_dir/src/shandalar/shandalar.ico" "$app_dir/shandalar.png" <<'PY'
from PIL import Image
import sys
Image.open(sys.argv[1]).save(sys.argv[2])
PY
cat > "$app_dir/shandalar.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Shandalar
Comment=Magic: The Gathering (1997) SDL3 port
Exec=shandalar
Icon=shandalar
Categories=Game;CardGame;
Terminal=false
DESKTOP
cat > "$app_dir/AppRun" <<'APPRUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
SHANDALAR_WINDOW_ICON="$HERE/shandalar.png"
export SHANDALAR_WINDOW_ICON
SHANDALAR_USER_DATA_DIR="${SHANDALAR_USER_DATA_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/shandalar/userdata}"
mkdir -p -- "$SHANDALAR_USER_DATA_DIR" || exit 1
export SHANDALAR_USER_DATA_DIR
exec "$HERE/usr/bin/shandalar" "$HERE/usr/share/shandalar/assets" "$@"
APPRUN
chmod +x "$app_dir/AppRun"

# Linux cannot overwrite an AppImage while it is running. Keep the old file
# under a dated name so a new build can still finish.
output_appimage="$repo_dir/Shandalar-x86_64.AppImage"
if [[ -e $output_appimage ]]; then
  backup_appimage="$output_appimage.previous.$(date +%Y%m%d-%H%M%S)"
  mv -- "$output_appimage" "$backup_appimage"
  echo "Previous AppImage kept at: $backup_appimage"
fi

(
  cd "$repo_dir"
  export APPIMAGE_EXTRACT_AND_RUN=1
  "$tool_dir/linuxdeploy" --appdir "$app_dir" \
    -d "$app_dir/shandalar.desktop" -i "$app_dir/shandalar.png" \
    --output appimage
)
find "$repo_dir" -maxdepth 1 -name '*.AppImage' -print

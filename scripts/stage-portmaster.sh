#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
if [[ ${1:-} != --inside ]]; then
    "$ROOT/scripts/build-portmaster.sh"
    in_builder stage-portmaster.sh
    exit
fi
for tool in g++ convert identify jq; do need "$tool"; done
source_dir="$ROOT/dist/portmaster-source/port/portpad"
[[ ! -L "$ROOT/dist" && ! -L "$ROOT/dist/portmaster-source" ]] || fail 'Refusing symlinked staging root.'
rm -rf -- "$ROOT/dist/portmaster-source"
mkdir -p "$source_dir/portpad/licenses" "$source_dir/portpad/libs.aarch64"
cp "$ROOT/portmaster/PortPad.sh" "$ROOT/portmaster/port.json" \
    "$ROOT/portmaster/README.md" "$ROOT/portmaster/gameinfo.xml" "$source_dir/"
cp "$ROOT/build-portmaster/portpad" "$source_dir/portpad/"
mkdir -p "$source_dir/portpad/assets/fonts"
cp "$ROOT/assets/fonts/DejaVuSans.ttf" "$ROOT/assets/fonts/DejaVuSansMono.ttf" "$source_dir/portpad/assets/fonts/"
cp -R "$ROOT/config" "$source_dir/portpad/"
cp "$ROOT/LICENSE" "$source_dir/portpad/licenses/LICENSE.PortPad.txt"
cp "$ROOT/assets/fonts/LICENSE-DejaVu.txt" "$source_dir/portpad/licenses/LICENSE.DejaVu.txt"
# No runtime libraries are copied: use firmware SDL2 and system libraries.
readelf -d "$source_dir/portpad/portpad" > "$ROOT/dist/runtime-dependencies.txt"
g++ -std=c++17 -I"$ROOT/include" "$ROOT/scripts/capture-portmaster-screenshot.cpp" \
    "$ROOT/build-portmaster/libportpad_ui.a" "$ROOT/build-portmaster/libportpad_state.a" \
    $(pkg-config --cflags --libs sdl2 SDL2_ttf) -Wl,--wrap=SDL_RenderPresent \
    -o "$ROOT/build-portmaster/capture-screenshot"
cd "$source_dir/portpad"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$ROOT/build-portmaster/capture-screenshot" "$ROOT/build-portmaster/screenshot.bmp" \
    "$ROOT/assets/fonts/DejaVuSans.ttf" "$ROOT/config/keymap.ini"
convert "$ROOT/build-portmaster/screenshot.bmp" "$source_dir/screenshot.png"
[[ $(identify -format '%wx%h' "$source_dir/screenshot.png") == 640x480 ]] || fail 'Screenshot must be 640x480.'
find "$source_dir" -type d -exec chmod 755 {} +
find "$source_dir" -type f -exec chmod 644 {} +
chmod 755 "$source_dir/portpad/portpad"
jq -e '.name == "portpad.zip" and .items == ["PortPad.sh", "portpad"]' "$source_dir/port.json" >/dev/null
echo "PortMaster source layout ready: $source_dir"

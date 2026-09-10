#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
if [[ ${1:-} != --inside ]]; then in_builder validate-portmaster-package.sh; exit; fi
for tool in unzip zipinfo jq file readelf diff identify; do need "$tool"; done
archive="$ROOT/dist/portpad.zip"
source_dir="$ROOT/dist/portmaster-source/port/portpad"
[[ -s "$archive" ]] || fail "Missing archive: $archive"
unzip -tq "$archive"
expected=$(printf '%s\n' PortPad.sh portpad/portpad portpad/port.json portpad/portpad.md \
    portpad/gameinfo.xml portpad/screenshot.png portpad/config/keymap.ini \
    portpad/assets/fonts/DejaVuSans.ttf portpad/assets/fonts/DejaVuSansMono.ttf \
    portpad/licenses/LICENSE.PortPad.txt portpad/licenses/LICENSE.DejaVu.txt | sort)
entries=$(unzip -Z1 "$archive")
while IFS= read -r entry; do
    case "$entry" in
        portpad/|portpad/assets/|portpad/assets/fonts/|portpad/config/|portpad/licenses/|portpad/libs.aarch64/) ;;
        */) fail "Unexpected ZIP directory: $entry" ;;
        *) grep -Fxq -- "$entry" <<< "$expected" || fail "Unexpected ZIP entry: $entry" ;;
    esac
done <<< "$entries"
actual=$(sed '/\/$/d' <<< "$entries" | sort)
[[ "$actual" == "$expected" ]] || fail 'ZIP contents differ from the required file manifest (missing or duplicate files).'
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
unzip -q "$archive" -d "$tmp"
[[ -f "$tmp/PortPad.sh" && -x "$tmp/portpad/portpad" ]] || fail 'Launcher or executable missing.'
[[ -z $(find "$tmp" -type l -print) ]] || fail 'ZIP contains symlinks.'
# Official build_port_zip moves metadata/artwork into the data directory,
# renames README.md to portpad.md, and normalizes port.json through port_info_load.
diff -r "$source_dir/portpad" "$tmp/portpad" --exclude=port.json --exclude=portpad.md \
    --exclude=gameinfo.xml --exclude=screenshot.png --exclude=libs.aarch64 || fail 'Archive payload does not match the exported source.'
cmp "$source_dir/PortPad.sh" "$tmp/PortPad.sh"
cmp "$source_dir/README.md" "$tmp/portpad/portpad.md"
cmp "$source_dir/gameinfo.xml" "$tmp/portpad/gameinfo.xml"
cmp "$source_dir/screenshot.png" "$tmp/portpad/screenshot.png"
[[ $(identify -format '%wx%h' "$tmp/portpad/screenshot.png") == 640x480 ]] || fail 'Invalid screenshot dimensions.'
cd "$tmp/portpad"
jq -e '.version == 4 and .name == "portpad.zip" and .items == ["PortPad.sh", "portpad"] and .attr.arch == ["aarch64"] and .attr.runtime == [] and .attr.exp == true and (.attr.min_glibc | test("^[0-9]+\\.[0-9]+$"))' port.json >/dev/null || fail 'Invalid PortMaster metadata.'
bash -n "$tmp/PortPad.sh"
for path in portpad; do
    arm64 "$path"
    dynamic=$(readelf -d "$path")
    [[ "$dynamic" != *'(RPATH)'* && "$dynamic" != *'(RUNPATH)'* ]] || fail "Build-specific runtime search path in $path"
    while IFS= read -r dependency; do
        case "$dependency" in
            libSDL2-2.0.so.0|libSDL2_ttf-2.0.so.0|libc.so.6|libpthread.so.0|libm.so.6|libdl.so.2|ld-linux-aarch64.so.1|libstdc++.so.6|libgcc_s.so.1) ;;
            *) fail "Unexpected firmware dependency $dependency required by $path" ;;
        esac
    done < <(sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' <<< "$dynamic")
done
versions=$(readelf --version-info portpad)
glibc=$(grep -oE 'GLIBC_[0-9.]+' <<< "$versions" | sort -Vu | tail -1)
glibcxx=$(grep -oE 'GLIBCXX_[0-9.]+' <<< "$versions" | sort -Vu | tail -1)
baseline="GLIBC_$(jq -r '.attr.min_glibc' port.json)"
[[ $(printf '%s\n' "$baseline" "$glibc" | sort -V | tail -1) == "$baseline" ]] || fail "Runtime exceeds declared glibc baseline $baseline: $glibc"
echo "Package ABI requirements: $glibc, $glibcxx"
resolved=$(ldd ./portpad)
[[ "$resolved" != *'not found'* ]] || fail "Unresolved packaged dependencies: $resolved"
echo 'No bundled shared libraries. Builder smoke test uses its system SDL; firmware verification remains required.'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./portpad --smoke-test
echo 'Validated archive contents:'
unzip -l "$archive"
echo "ZIP size: $(stat -c %s "$archive") bytes"

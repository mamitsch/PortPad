#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
if [[ ${1:-} != --inside ]]; then
    in_builder build-portmaster.sh
    echo "ARM64 build succeeded: $ROOT/build-portmaster/portpad"
    exit
fi
[[ $(uname -m) == aarch64 ]] || fail 'Builder is not running ARM64; enable Docker ARM64 emulation.'
for tool in cmake ctest file readelf patchelf pkg-config; do need "$tool"; done
generator=()
if command -v ninja >/dev/null; then generator=(-G Ninja); fi
cmake -S "$ROOT" -B "$ROOT/build-portmaster" "${generator[@]}" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_SKIP_RPATH=ON
cmake --build "$ROOT/build-portmaster" --parallel
# The builder's SDL pkg-config adds /usr/local/lib even with CMAKE_SKIP_RPATH.
patchelf --remove-rpath "$ROOT/build-portmaster/portpad"
export LD_LIBRARY_PATH="$(pkg-config --variable=libdir sdl2)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --test-dir "$ROOT/build-portmaster" --output-on-failure
arm64 "$ROOT/build-portmaster/portpad"
readelf -h "$ROOT/build-portmaster/portpad"
readelf --version-info "$ROOT/build-portmaster/portpad"
readelf -d "$ROOT/build-portmaster/portpad"

#!/bin/bash
set -euo pipefail
export LC_ALL=C
ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
IMAGE=ghcr.io/monkeyx-net/portmaster-build-templates/portmaster-builder:aarch64-latest
fail() { echo "PortPad: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null || fail "Missing prerequisite: $1"; }
in_builder() {
    need docker
    docker run --rm --platform linux/arm64 --user "$(id -u):$(id -g)" \
        --mount "type=bind,source=$ROOT,target=/workspace" --workdir /workspace \
        --entrypoint /bin/bash "$IMAGE" "/workspace/scripts/$1" --inside
}
arm64() {
    [[ -f "$1" ]] || fail "Missing ELF: $1"
    file "$1"
    readelf -h "$1" | grep -q 'Machine:.*AArch64' || fail "Not AArch64: $1"
}

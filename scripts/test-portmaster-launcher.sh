#!/bin/bash
# Host-side integration test with isolated PortMaster helpers, no handheld needed.
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p "$tmp/Ports with spaces/portpad/config" "$tmp/controls"
cp "$ROOT/portmaster/PortPad.sh" "$tmp/Ports with spaces/PortPad.sh"
cat > "$tmp/controls/control.txt" <<'CONTROL'
get_controls() { sdl_controllerconfig='test mapping'; }
pm_platform_helper() { printf '%s\n' "$1" > "$TEST_ROOT/helper"; }
pm_finish() { echo finish >> "$TEST_ROOT/finish"; }
CONTROL
cat > "$tmp/Ports with spaces/portpad/portpad" <<'APP'
#!/bin/bash
[[ "$SDL_GAMECONTROLLERCONFIG" == 'test mapping' ]] || exit 91
[[ -z ${LD_LIBRARY_PATH:-} ]] || exit 92
[[ -z ${PORTPAD_KEYMAP:-} ]] || exit 93
echo 'application stdout'
echo 'application stderr' >&2
exit 23
APP
chmod 644 "$tmp/Ports with spaces/portpad/portpad"
set +e
env -u LD_LIBRARY_PATH -u PORTPAD_KEYMAP TEST_ROOT="$tmp" controlfolder="$tmp/controls" \
    bash "$tmp/Ports with spaces/PortPad.sh"
status=$?
set -e
[[ $status == 23 ]] || fail "Launcher did not preserve application exit status: $status"
[[ $(cat "$tmp/finish") == finish ]] || fail 'Cleanup did not run exactly once.'
[[ $(cat "$tmp/helper") == "$tmp/Ports with spaces/portpad/portpad" ]] || fail 'Incorrect platform helper path.'
grep -q 'application stderr' "$tmp/Ports with spaces/portpad/log.txt"
grep -q 'application stdout' "$tmp/Ports with spaces/portpad/log.txt"
mv "$tmp/Ports with spaces/portpad/portpad" "$tmp/Ports with spaces/portpad/hidden"
if TEST_ROOT="$tmp" controlfolder="$tmp/controls" bash "$tmp/Ports with spaces/PortPad.sh" > "$tmp/error" 2>&1; then
    fail 'Launcher accepted a missing executable.'
fi
grep -q 'executable missing' "$tmp/error"
[[ $(wc -l < "$tmp/finish") == 2 ]] || fail 'Missing executable skipped cleanup.'
echo 'PortMaster launcher integration checks passed.'

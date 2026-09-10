#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
repo=${1:-${PORTMASTER_REPO:-}}
[[ -n "$repo" ]] || fail 'Usage: export-portmaster.sh /path/to/PortMaster-New (or set PORTMASTER_REPO)'
repo=$(cd -- "$repo" && pwd -P)
case "$repo/" in "$ROOT/"*) fail 'Keep the official checkout outside PortPad.' ;; esac
need git
[[ $(git -C "$repo" rev-parse --show-toplevel) == "$repo" ]] || fail 'Destination must be a Git checkout root.'
remote=$(git -C "$repo" remote get-url origin)
case "$remote" in
    https://github.com/PortsMaster/PortMaster-New|https://github.com/PortsMaster/PortMaster-New.git|git@github.com:PortsMaster/PortMaster-New.git) ;;
    *) fail 'Destination origin must be the official PortsMaster/PortMaster-New repository.' ;;
esac
[[ -f "$repo/tools/prepare_repo.sh" && ! -L "$repo/ports" ]] || fail 'Checkout must include tools/prepare_repo.sh and a real ports directory.'
target="$repo/ports/portpad"
record="$repo/.portpad-export.sha256"
fingerprint() { (cd "$target" && find . -type f -print0 | sort -z | xargs -0 sha256sum); }
if [[ -e "$target" || -L "$target" ]]; then
    [[ -d "$target" && ! -L "$target" && -f "$record" ]] || fail "Refusing to replace existing unowned $target"
    [[ -z $(find "$target" -type l -print) && $(fingerprint) == "$(cat "$record")" ]] || fail 'Export was edited; preserve those changes before exporting again.'
fi
"$ROOT/scripts/stage-portmaster.sh"
mkdir -p "$repo/ports"
rm -rf -- "$target"
cp -R "$ROOT/dist/portmaster-source/port/portpad" "$target"
# WSL/Windows mounts can report synthesized modes; enforce upstream source modes
# on the destination filesystem after copying.
find "$target" -type f -exec chmod 644 {} +
chmod 755 "$target/portpad/portpad"
fingerprint > "$record"
echo "Exported source layout to $target"

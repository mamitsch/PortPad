#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
repo=${1:-${PORTMASTER_REPO:-}}
[[ -n "$repo" ]] || fail 'Usage: package-portmaster.sh /path/to/PortMaster-New (or set PORTMASTER_REPO)'
repo=$(cd -- "$repo" && pwd -P)
for tool in python3 git; do need "$tool"; done
"$ROOT/scripts/export-portmaster.sh" "$repo"
cd "$repo"
mkdir -p releases runtimes
# Run the official checkout's own tools; no copied or reimplemented release builder.
tools/prepare_repo.sh
# prepare_repo.sh does not stop on download failures; check its required outputs.
for metadata in ports.json ports_status.json manifest.json port_stats_raw.json; do
    [[ -s "releases/$metadata" ]] || fail "Official preparation did not download releases/$metadata"
done
[[ -s releases/images.zip ]] || fail 'Official preparation did not download releases/images.zip'
python3 tools/build_release.py --do-check
python3 tools/build_release.py
[[ -s releases/portpad.zip ]] || fail 'Official builder did not produce releases/portpad.zip'
cp releases/portpad.zip "$ROOT/dist/portpad.zip"
git rev-parse HEAD > "$ROOT/dist/portmaster-revision.txt"
# Keep the historical artifact out of the installable ZIP names.
if [[ -f "$ROOT/dist/PortPad.zip" && ! "$ROOT/dist/PortPad.zip" -ef "$ROOT/dist/portpad.zip" ]]; then
    mv "$ROOT/dist/PortPad.zip" "$ROOT/dist/PortPad.zip.obsolete"
fi
"$ROOT/scripts/validate-portmaster-package.sh"
echo "Official package ready: $ROOT/dist/portpad.zip"

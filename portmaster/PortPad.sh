#!/bin/bash
# PortMaster's sourced scripts are not compatible with set -u / set -e.
XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
launcher_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) || exit 1
for candidate in "${controlfolder:-}" /opt/system/Tools/PortMaster /opt/tools/PortMaster \
    "$XDG_DATA_HOME/PortMaster" "$launcher_dir/PortMaster" /roms/ports/PortMaster; do
    if [[ -n "$candidate" && -f "$candidate/control.txt" ]]; then
        controlfolder=$candidate
        break
    fi
done
if [[ ! -f "${controlfolder:-}/control.txt" ]]; then
    echo 'PortPad: PortMaster control.txt was not found.' >&2
    exit 1
fi
source "$controlfolder/control.txt" || exit 1
if [[ -f "$controlfolder/mod_${CFW_NAME}.txt" ]]; then
    source "$controlfolder/mod_${CFW_NAME}.txt"
fi
get_controls || exit 1
# Prefer the installed launcher location (including secondary SD cards).
GAMEDIR="$launcher_dir/portpad"
if [[ ! -d "$GAMEDIR" && -n "${directory:-}" ]]; then
    GAMEDIR="/${directory#/}/ports/portpad"
fi
if [[ ! -f "$GAMEDIR/portpad" ]]; then
    echo "PortPad: executable missing: $GAMEDIR/portpad" >&2
    pm_finish
    exit 1
fi
cd "$GAMEDIR" || { pm_finish; exit 1; }
chmod +x ./portpad || { pm_finish; exit 1; }
# One small log, replaced each launch, with stdout/stderr for first-device diagnosis.
exec >"$GAMEDIR/log.txt" 2>&1
echo "PortPad launch: $(date -Iseconds); architecture: $(uname -m)"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
pm_platform_helper "$GAMEDIR/portpad"
./portpad
status=$?
echo "PortPad exit: $status"
pm_finish
exit "$status"

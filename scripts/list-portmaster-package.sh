#!/bin/bash
source "$(dirname -- "${BASH_SOURCE[0]}")/portmaster-common.sh"
need unzip
[[ -s "$ROOT/dist/portpad.zip" ]] || fail 'Missing dist/portpad.zip; run package-portmaster.sh first.'
unzip -l "$ROOT/dist/portpad.zip"

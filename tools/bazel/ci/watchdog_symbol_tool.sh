#!/bin/sh
# Preserve the pinned tool's declared runfiles when this validation TAR moves.
set -eu
self=$0
case "$self" in /*) ;; *) self="$PWD/$self" ;; esac
tool=${self##*/}
tool=${tool#watchdog_}
case "$tool" in
    gdb) package=tools/build_tools ;;
    readelf|objcopy) package=toolchains/binutils ;;
    *) echo "Unknown watchdog inspection tool: $tool" >&2; exit 1 ;;
esac
RUNFILES_DIR="$self.runfiles"
export RUNFILES_DIR
selected=
for candidate in "$RUNFILES_DIR"/*/"$package/$tool"; do
    [ -x "$candidate" ] || continue
    [ -z "$selected" ] || { echo "Ambiguous declared tool: $tool" >&2; exit 1; }
    selected=$candidate
done
[ -n "$selected" ] || { echo "Missing declared tool: $tool" >&2; exit 1; }
exec "$selected" "$@"

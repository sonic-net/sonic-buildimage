#!/bin/bash
#
# Mirror hot-pluggable bus nodes (tagged "pmon-hotplug") into the running pmon container's /dev.
#
# Usage:
#   pmon-dev-bridge.sh resync                       # mirror all tagged nodes (called on pmon start)
#   pmon-dev-bridge.sh event <add|remove> <devname> # handle one hot-plug event (called by udev)

set -u

# pmon is a single global container (one instance even on multi-asic platforms).
PMON=pmon

pmon_running() {
    docker ps --format '{{.Names}}' 2>/dev/null | grep -qx "$PMON"
}

# Feed a script of shell commands to the pmon container in a single exec.
apply_in_pmon() {
    [ -n "$1" ] || return 0
    printf '%s' "$1" | docker exec -i "$PMON" sh 2>/dev/null || true
}

# Emit an idempotent mknod for one character node (major/minor read on the host).
mknod_cmd() {
    local node="$1" majorhex minorhex major minor
    [ -e "$node" ] || return 0
    [ -c "$node" ] || return 0              # character bus interfaces only
    majorhex="$(stat -c '%t' "$node" 2>/dev/null)"
    minorhex="$(stat -c '%T' "$node" 2>/dev/null)"
    major=$(( 16#${majorhex:-0} ))
    minor=$(( 16#${minorhex:-0} ))
    [ "$major" -gt 0 ] || return 0
    printf "[ -e '%s' ] || mknod '%s' c %d %d\n" "$node" "$node" "$major" "$minor"
}

# Emit idempotent mknods for every pmon-hotplug node from a single udevadm export
# (DEVNAME/MAJOR/MINOR come straight from the udev db -- no per-node udevadm or stat).
tagged_mknod_script() {
    local dn="" maj="" min="" line
    { udevadm info -e --tag-match=pmon-hotplug 2>/dev/null; echo; } | while IFS= read -r line; do
        case "$line" in
            "E: DEVNAME="*) dn="${line#E: DEVNAME=}" ;;
            "E: MAJOR="*)   maj="${line#E: MAJOR=}" ;;
            "E: MINOR="*)   min="${line#E: MINOR=}" ;;
            "")
                if [ -n "$dn" ] && [ -n "$maj" ] && [ "$maj" != "0" ]; then
                    printf "[ -e '%s' ] || mknod '%s' c %s %s\n" "$dn" "$dn" "$maj" "$min"
                fi
                dn=""; maj=""; min=""
                ;;
        esac
    done
}

case "${1:-}" in
    resync)
        # Mirror all currently-tagged nodes into the container in a single docker exec.
        pmon_running || exit 0
        apply_in_pmon "$(tagged_mknod_script)"
        ;;
    event)
        action="${2:-}"
        devname="${3:-}"
        [ -n "$devname" ] || exit 0
        case "$devname" in /dev/*) ;; *) devname="/dev/$devname" ;; esac
        pmon_running || exit 0
        case "$action" in
            add)    apply_in_pmon "$(mknod_cmd "$devname")" ;;
            remove) apply_in_pmon "rm -f '$devname'" ;;
        esac
        ;;
    *)
        echo "usage: $0 {resync | event <add|remove> <devname>}" >&2
        exit 2
        ;;
esac
exit 0

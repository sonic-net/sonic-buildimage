#!/bin/bash
#
# Mirror hot-pluggable bus nodes (tagged "pmon-hotplug") into the running pmon container's /dev.
#
# Usage:
#   pmon-dev-bridge.sh resync                       # mirror all tagged nodes (called on pmon start)
#   pmon-dev-bridge.sh event <add|remove> <devname> # handle one hot-plug event (called by udev)

set -u

# Names of running pmon containers (single-asic: "pmon"; multi-asic: "pmon0", "pmon1", ...).
pmon_containers() {
    docker ps --format '{{.Names}}' 2>/dev/null | grep -E '^pmon[0-9]*$'
}

# DEVNAMEs of every device carrying the pmon-hotplug udev tag.
tagged_devnodes() {
    local syspath
    for syspath in $(udevadm trigger --verbose --dry-run --type=devices --tag-match=pmon-hotplug 2>/dev/null); do
        udevadm info --query=property --path="$syspath" 2>/dev/null | sed -n 's/^DEVNAME=//p'
    done
}

# Create the node inside every running pmon container (idempotent).
mirror_add() {
    local node="$1" c majorhex minorhex major minor
    [ -e "$node" ] || return 0
    [ -c "$node" ] || return 0              # character bus interfaces only
    majorhex="$(stat -c '%t' "$node" 2>/dev/null)"
    minorhex="$(stat -c '%T' "$node" 2>/dev/null)"
    major=$(( 16#${majorhex:-0} ))
    minor=$(( 16#${minorhex:-0} ))
    [ "$major" -gt 0 ] || return 0
    for c in $(pmon_containers); do
        docker exec "$c" sh -c "[ -e '$node' ] || mknod '$node' c $major $minor" 2>/dev/null || true
    done
}

# Remove the node from every running pmon container (best-effort).
mirror_remove() {
    local node="$1" c
    for c in $(pmon_containers); do
        docker exec "$c" sh -c "rm -f '$node'" 2>/dev/null || true
    done
}

case "${1:-}" in
    resync)
        # Mirror all currently-tagged nodes into the container.
        for node in $(tagged_devnodes); do
            mirror_add "$node"
        done
        ;;
    event)
        action="${2:-}"
        devname="${3:-}"
        [ -n "$devname" ] || exit 0
        case "$devname" in /dev/*) ;; *) devname="/dev/$devname" ;; esac
        case "$action" in
            add)    mirror_add "$devname" ;;
            remove) mirror_remove "$devname" ;;
        esac
        ;;
    *)
        echo "usage: $0 {resync | event <add|remove> <devname>}" >&2
        exit 2
        ;;
esac
exit 0

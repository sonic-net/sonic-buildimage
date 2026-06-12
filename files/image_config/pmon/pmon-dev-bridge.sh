#!/bin/bash
#
# pmon-dev-bridge.sh -- mirror hot-pluggable bus nodes (i2c/cpld/fpga/ipmi/uio/spidev/gpiochip/mtd)
# into the running pmon container's /dev, so a node that appears after 'docker create' becomes
# visible without recreating the container.
#
# Usage:
#   pmon-dev-bridge.sh resync                       # converge all allowed nodes (called on pmon start)
#   pmon-dev-bridge.sh event <add|remove> <devname> # one hot-plug event (called by udev)

set -u

# Hot-pluggable bus-class allowlist. MUST stay in sync with $hotplugregex in
# get_pmon_device_cgroup_rules() (files/build_templates/docker_image_ctl.j2): we only mirror
# nodes whose class major was actually granted, so visibility and permission stay aligned.
# Anchored to whole node names.
PMON_HOTPLUG_ALLOW='^(i2c-[0-9]+|cpld[0-9]|fpga[0-9]|ipmi[0-9]+|uio.*|spidev.*|gpiochip[0-9]+|mtd[0-9]+(ro)?)$'

# Names of running pmon containers (single-asic: "pmon"; multi-asic: "pmon0", "pmon1", ...).
pmon_containers() {
    docker ps --format '{{.Names}}' 2>/dev/null | grep -E '^pmon[0-9]*$'
}

# True if the node's basename is a hot-plug bus class pmon is granted. Cheapest check -> run
# it first so the flood of unrelated uevents at boot exits before we ever touch docker.
is_allowed() {
    basename "$1" | grep -Eq "$PMON_HOTPLUG_ALLOW"
}

# Create the node inside every running pmon container (idempotent; guarded by existence
# check). mknod succeeds because the container already holds 'c <major>:* rwm' (the
# 'm'/mknod bit) for this class and Docker's default cap set includes CAP_MKNOD.
mirror_add() {
    local node="$1" c majorhex minorhex major minor
    is_allowed "$node" || return 0
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
    is_allowed "$node" || return 0
    for c in $(pmon_containers); do
        docker exec "$c" sh -c "rm -f '$node'" 2>/dev/null || true
    done
}

case "${1:-}" in
    resync)
        # Converge the container with whatever the host currently exposes: idempotent.
        # Covers anything that appeared between 'docker create' and now.
        for node in $(find /dev -maxdepth 1 -type c 2>/dev/null); do
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

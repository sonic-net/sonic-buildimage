#!/bin/bash
set -euo pipefail

cache=${SONIC_DPKG_CACHE_SOURCE:?}
mode=${SONIC_DPKG_CACHE_METHOD:?}
case "$mode" in
    rcache|rwcache) ;;
    *) printf 'Unsupported NFS package cache mode: %s\n' "$mode" >&2; exit 1 ;;
esac
if ! sudo -n test -d "$cache"; then
    printf 'NFS package cache is missing or inaccessible to privileged host access: %s\n' "$cache" >&2
    exit 1
fi
# statfs triggers automounts and reports the resolved filesystem, not the autofs layer.
filesystem=$(sudo -n stat --file-system --format=%T -- "$cache")
case "$filesystem" in
    nfs|nfs4) ;;
    *) printf 'Package cache must be NFS-backed, found %s: %s\n' "$filesystem" "$cache" >&2; exit 1 ;;
esac
printf 'Validated NFS package cache source: %s (mode=%s; cache access handled by the slave)\n' "$cache" "$mode"

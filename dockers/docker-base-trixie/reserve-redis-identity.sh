#!/bin/sh
set -eu

# Reserve the service identity before installing packages that allocate users.
fail() { echo "Redis service identity: $*" >&2; exit 1; }

normalize_id() {
    case "$1" in
        ''|*[!0-9]*) fail "$2 must be a decimal integer" ;;
    esac
    printf '%s\n' "$1" | sed 's/^0*//; s/^$/0/'
}

[ "$#" -eq 2 ] || fail 'requires the configured UID and GID'
redis_uid=$(normalize_id "$1" UID)
redis_gid=$(normalize_id "$2" GID)
[ "$(id -u)" = 0 ] || fail 'requires root'

if getent passwd | awk -F: -v uid="$redis_uid" '$3 == uid && $1 != "redis" { found=1 } END { exit !found }'; then
    fail 'reserved UID is already assigned'
fi
if getent group | awk -F: -v gid="$redis_gid" '$3 == gid && $1 != "redis" { found=1 } END { exit !found }'; then
    fail 'reserved GID is already assigned'
fi

group=$(getent group redis || true)
if [ -n "$group" ]; then
    [ "$(echo "$group" | cut -d: -f3)" = "$redis_gid" ] || fail 'redis group has an unexpected GID'
else
    groupadd --system --gid "$redis_gid" redis
fi

account=$(getent passwd redis || true)
if [ -n "$account" ]; then
    [ "$(echo "$account" | cut -d: -f3-4)" = "$redis_uid:$redis_gid" ] || fail 'redis account has an unexpected UID/GID'
    [ "$(echo "$account" | cut -d: -f7)" = /usr/sbin/nologin ] || fail 'redis account must not permit login'
else
    useradd --system --uid "$redis_uid" --gid "$redis_gid" --no-create-home \
        --home-dir /var/lib/redis --shell /usr/sbin/nologin redis
fi

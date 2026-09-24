#!/bin/bash
#
# bmcweb-start.sh - start bmcweb on the configured port.
#
# bmcweb takes its listening socket from systemd socket activation and, when
# there is none, falls back to a port compiled into the binary. Nothing in a
# container can give it the former, so without help it always serves on that
# built-in port regardless of what the platform is configured for.
#
# bmcweb-activate binds the port we want and hands the socket over the way
# systemd would, so bmcweb serves where it is configured to. exec keeps the pid
# supervisord is watching.

set -u

BMCWEB_BIN="${BMCWEB_BIN:-/usr/bin/bmcweb}"
BMCWEB_ACTIVATE="${BMCWEB_ACTIVATE:-/usr/bin/bmcweb-activate}"

DEFAULT_PORT=443

# sonic-db-cli aborts in this container, so read the mounted redis socket
# directly. The database id is the fixed SONiC one from
# /var/run/redis/sonic-db/database_config.json.
REDIS_SOCK="${BMCWEB_REDIS_SOCK:-/var/run/redis/redis.sock}"
CONFIG_DB_ID="${BMCWEB_CONFIG_DB_ID:-4}"

log() { logger -t bmcweb-start "$*"; echo "bmcweb-start: $*"; }

# Any read failure leaves the default in place: an unreachable CONFIG_DB must
# not stop the API from coming up, and the standard port is the safe answer.
PORT="$(redis-cli -s "${REDIS_SOCK}" -n "${CONFIG_DB_ID}" \
        HGET 'REDFISH|config' port 2>/dev/null)"
case "${PORT}" in
    ''|*[!0-9]*) PORT="${DEFAULT_PORT}" ;;
esac
if [ "${PORT}" -lt 1 ] || [ "${PORT}" -gt 65535 ]; then
    log "configured port is out of range; using ${DEFAULT_PORT}"
    PORT="${DEFAULT_PORT}"
fi

log "starting bmcweb on port ${PORT}"
exec "${BMCWEB_ACTIVATE}" "${PORT}" "${BMCWEB_BIN}"

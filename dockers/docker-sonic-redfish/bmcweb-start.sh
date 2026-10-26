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
#
# The port is read from CONFIG_DB (REDFISH|config|port) on every start, so a
# change takes effect when bmcweb or the redfish service is restarted.

set -u

BMCWEB_BIN="${BMCWEB_BIN:-/usr/bin/bmcweb}"
BMCWEB_ACTIVATE="${BMCWEB_ACTIVATE:-/usr/bin/bmcweb-activate}"

DEFAULT_PORT=443

log() { logger -t bmcweb-start "$*"; echo "bmcweb-start: $*"; }

# An unset, unreadable or invalid port must not stop the API from coming up;
# the standard port is the safe answer.
PORT="$(sonic-db-cli CONFIG_DB HGET "REDFISH|config" port 2>/dev/null)"
PORT="${PORT:-${DEFAULT_PORT}}"
case "${PORT}" in
    ''|*[!0-9]*)
        log "configured port '${PORT}' is not a number; using ${DEFAULT_PORT}"
        PORT="${DEFAULT_PORT}"
        ;;
esac
if [ "${PORT}" -lt 1 ] || [ "${PORT}" -gt 65535 ]; then
    log "configured port is out of range; using ${DEFAULT_PORT}"
    PORT="${DEFAULT_PORT}"
fi

log "starting bmcweb on port ${PORT}"
exec "${BMCWEB_ACTIVATE}" "${PORT}" "${BMCWEB_BIN}"

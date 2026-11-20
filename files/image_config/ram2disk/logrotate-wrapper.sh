#!/bin/bash
# /usr/sbin/logrotate-wrapper.sh - serialize logrotate with ram2disk.
#
# Installed via dpkg-divert so it transparently replaces the stock binary:
#   dpkg-divert --divert /usr/sbin/logrotate.real --rename --add /usr/sbin/logrotate
#   cp logrotate-wrapper.sh /usr/sbin/logrotate && chmod 755 /usr/sbin/logrotate
#

set -u
set -o pipefail

ORIG_LOGROTATE=/usr/sbin/logrotate.real
: "${R2D_LOCK:=/dev/shm/logrotate/synchronization.lock}"
: "${R2D_LOCK_WAIT:=300}"

[ -x "$ORIG_LOGROTATE" ] || { echo "logrotate : $ORIG_LOGROTATE not found" >&2; exit 127; }

# Bash defers these traps until a foreground child finishes, keeping the lock
# while original logrotate runs.
SIGNAL_RC=0
trap 'SIGNAL_RC=143' TERM
trap 'SIGNAL_RC=130' INT

if ! { mkdir -p -- "$(dirname -- "$R2D_LOCK")" &&
       exec {R2D_LOCK_FD}>>"$R2D_LOCK" &&
       flock -w "$R2D_LOCK_WAIT" "$R2D_LOCK_FD"; }; then
    echo "logrotate: cannot acquire rotation/sync lock" >&2
    exit 1
fi
[ "$SIGNAL_RC" -eq 0 ] || exit "$SIGNAL_RC"

# Only the wrapper owns the lock, not logrotate or its hook processes.
"$ORIG_LOGROTATE" "$@" {R2D_LOCK_FD}>&-
RC=$?

[ "$SIGNAL_RC" -eq 0 ] || RC="$SIGNAL_RC"
exit "$RC"

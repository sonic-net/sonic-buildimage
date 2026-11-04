#!/bin/bash
# /usr/sbin/logrotate-wrapper.sh - counting wrapper around the real logrotate binary.
#
# Installed via dpkg-divert so it transparently replaces the stock binary:
#   dpkg-divert --divert /usr/sbin/logrotate.real --rename --add /usr/sbin/logrotate
#   cp logrotate-wrapper.sh /usr/sbin/logrotate && chmod 755 /usr/sbin/logrotate
#

set -u
set -o pipefail

ORIG_LOGROTATE=/usr/sbin/logrotate.real
COUNTER_FUNCS=/usr/local/bin/logrotate-counter-functions.sh

[ -x "$ORIG_LOGROTATE" ] || { echo "logrotate : $ORIG_LOGROTATE not found" >&2; exit 127; }

# In debug/dry-run mode logrotate PRINTS "rotating log ..." without actually
# rotating, so we must never count it -- just pass straight through.
debug=0
verbose=0
for a in "$@"; do
    case "$a" in
        -d|--debug)   debug=1 ;;
        -v|--verbose) verbose=1 ;;
    esac
done

if [ "$debug" -eq 1 ]; then
    exec "$ORIG_LOGROTATE" "$@"
fi

# Force verbose so we can detect which files actually rotated. logrotate writes
# verbose diagnostics to stderr; stdout is left untouched for the caller.
args=("$@")
[ "$verbose" -eq 1 ] || args=(-v "${args[@]}")

TMP_STATUS_FILE=$(mktemp)
if [ -z "$TMP_STATUS_FILE" ]; then
    echo "logrotate: cannot create rotation status file" >&2
    exit 1
fi
trap 'rm -f "$TMP_STATUS_FILE"' EXIT
# Bash defers these traps until a foreground child finishes, keeping the lock
# while original logrotate runs.
SIGNAL_RC=0
trap 'SIGNAL_RC=143' TERM
trap 'SIGNAL_RC=130' INT

if ! . "$COUNTER_FUNCS"; then
    echo "logrotate: cannot load $COUNTER_FUNCS" >&2
    exit 1
fi
if ! lrc_operation_lock; then
    echo "logrotate: cannot acquire rotation/sync lock" >&2
    exit 1
fi
[ "$SIGNAL_RC" -eq 0 ] || exit "$SIGNAL_RC"

"$ORIG_LOGROTATE" "${args[@]}" {LRC_LOCK_FD}>&- 2>"$TMP_STATUS_FILE"
RC=$?

# Re-emit stderr to the caller when they asked for -v (preserve requested
# output) or when logrotate failed (never hide real errors). Otherwise stay
# quiet, matching plain non-verbose logrotate behaviour.
if [ "$verbose" -eq 1 ] || [ "$RC" -ne 0 ]; then
    cat "$TMP_STATUS_FILE" >&2
fi

# Record completed rotations before releasing the lock, even after a signal.
mapfile -t ROTATED < <(sed -n 's/^rotating log \(.*\), log->rotateCount.*/\1/p' "$TMP_STATUS_FILE")
if [ "${#ROTATED[@]}" -gt 0 ]; then
    if ! lrc_file_increment "${ROTATED[@]}"; then
        echo "logrotate: cannot record rotation counters" >&2
        [ "$RC" -ne 0 ] || RC=1
    fi
fi

{LRC_LOCK_FD}>&-

[ "$SIGNAL_RC" -eq 0 ] || RC="$SIGNAL_RC"
exit "$RC"

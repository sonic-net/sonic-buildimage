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
trap 'rm -f "$TMP_STATUS_FILE"' EXIT

"$ORIG_LOGROTATE" "${args[@]}" 2>"$TMP_STATUS_FILE"
RC=$?

# Re-emit stderr to the caller when they asked for -v (preserve requested
# output) or when logrotate failed (never hide real errors). Otherwise stay
# quiet, matching plain non-verbose logrotate behaviour.
if [ "$verbose" -eq 1 ] || [ "$RC" -ne 0 ]; then
    cat "$TMP_STATUS_FILE" >&2
fi

# Count the rotations that actually happened. Source the counter library and
# increment all rotated files.
mapfile -t ROTATED < <(sed -n 's/^rotating log \(.*\), log->rotateCount.*/\1/p' "$TMP_STATUS_FILE")
if [ "${#ROTATED[@]}" -gt 0 ] && [ -r "$COUNTER_FUNCS" ]; then
    . "$COUNTER_FUNCS"
    lrc_file_increment "${ROTATED[@]}"
fi

exit "$RC"

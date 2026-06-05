#!/bin/sh

set -eu

PMON_SH="/usr/bin/pmon.sh"
TMP_FILE="$(mktemp)"
PATTERN='i2c-[0-9]+|cpld[0-9]+|fpga[0-9]+|ipmi[0-9]+|sd[a-z]+|mmcblk[0-9].*|nvme[0-9].*|uio.*|watchdog[0-9]*'

cleanup() {
    rm -f "$TMP_FILE"
}

trap cleanup EXIT

if [ ! -f "$PMON_SH" ]; then
    exit 0
fi

if grep -q 'cpld\[0-9\]\+\|fpga\[0-9\]\+' "$PMON_SH"; then
    exit 0
fi

sed "/local devregex=/c\\    local devregex='$PATTERN'" \
    "$PMON_SH" > "$TMP_FILE"

if cmp -s "$PMON_SH" "$TMP_FILE"; then
    exit 0
fi

if ! install -m 0755 "$TMP_FILE" "$PMON_SH"; then
    echo "micas-pmon-prestart: warning: failed to update $PMON_SH" >&2
fi

exit 0

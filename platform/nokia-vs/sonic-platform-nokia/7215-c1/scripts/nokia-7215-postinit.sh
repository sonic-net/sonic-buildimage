#!/bin/bash
#
# Nokia 7215 post-init platform script.
#
# - Creates /dev/ttyCR<n+1> -> /dev/ttyCO<n> symlinks so console clients
#   can address ports by their physical 1-based numbering.

set -u

POSTINIT_STAMP="/run/nokia-7215-postinit.done"
RENAME_MONITOR="/usr/local/bin/intf_sync.py"

run_one_time_setup()
{
    # - ttyCR symlinks for ttyCO0..ttyCO47

    echo "[nokia-7215-postinit] waiting for /dev/ttyCO0"

    tty_timeout=10
    i=0
    while [ $i -lt $tty_timeout ]; do
        if [ -e /dev/ttyCO0 ]; then
            break
        fi
        sleep 1
        i=$((i + 1))
    done

    if [ ! -e /dev/ttyCO0 ]; then
        echo "[nokia-7215-postinit] timeout (${tty_timeout}s) waiting for /dev/ttyCO0; skipping ttyCR symlinks"
        return
    fi

    echo "[nokia-7215-postinit] creating /dev/ttyCR1..48 -> /dev/ttyCO0..47 symlinks"
    for n in $(seq 0 47); do
        src="/dev/ttyCO${n}"
        dst="/dev/ttyCR$((n + 1))"
        if [ -e "$src" ]; then
            chmod 666 "$src"
            ln -sf "$src" "$dst"
        fi
    done
}

if [ ! -e "$POSTINIT_STAMP" ]; then
    run_one_time_setup
    if ! touch "$POSTINIT_STAMP"; then
        echo "[nokia-7215-postinit] failed to create $POSTINIT_STAMP"
    fi
else
    echo "[nokia-7215-postinit] one-time setup already completed; skipping"
fi

if [ ! -x "$RENAME_MONITOR" ]; then
    echo "[nokia-7215-postinit] $RENAME_MONITOR is not executable"
    exit 1
fi

echo "[nokia-7215-postinit] starting intf sync"
exec "$RENAME_MONITOR"

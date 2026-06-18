#!/bin/bash
#
# Nokia 7215 post-init platform script.
#
# - Creates /dev/ttyCR<n+1> -> /dev/ttyCO<n> symlinks so console clients
#   can address ports by their physical 1-based numbering.

set -u

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
    exit 0
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

exit 0

#!/bin/sh
set -eu

# ssh-keygen -A is idempotent: it generates only missing host keys, so this
# one-shot is safe with persistent /etc/ssh volumes and container restarts.
if [ ! -d /etc/ssh ]; then
    echo "/etc/ssh does not exist; cannot generate SSH host keys" >&2
    exit 1
fi

# If host keys are already present, there is nothing to generate. This lets a
# read-only /etc/ssh (e.g. mounted from an immutable volume or secret) work as
# long as it is already provisioned with valid host keys.
if ls /etc/ssh/ssh_host_*_key >/dev/null 2>&1; then
    exit 0
fi

if [ ! -w /etc/ssh ]; then
    echo "/etc/ssh is not writable and no SSH host keys exist; cannot generate them" >&2
    exit 1
fi

old_umask=$(umask)
umask 077
if ssh-keygen -A; then
    rc=0
else
    rc=$?
fi
umask "$old_umask"

if [ "$rc" -ne 0 ]; then
    echo "failed to generate SSH host keys" >&2
    exit "$rc"
fi

# Keep host key file permissions explicit for sshd StrictModes and scanners.
find /etc/ssh -maxdepth 1 -type f -name 'ssh_host_*_key' -exec chmod 0600 {} +
find /etc/ssh -maxdepth 1 -type f -name 'ssh_host_*_key.pub' -exec chmod 0644 {} +

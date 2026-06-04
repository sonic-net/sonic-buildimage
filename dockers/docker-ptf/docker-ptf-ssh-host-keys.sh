#!/bin/sh
set -eu

# ssh-keygen -A is idempotent: it generates only missing host keys, so this
# helper is safe to run before every sshd start and with persistent /etc/ssh
# volumes. Read-only /etc/ssh is accepted only when all required keys exist.
required_key_types="rsa ecdsa ed25519"

collect_missing_keys() {
    missing_keys=""
    for key_type in $required_key_types; do
        key_path="/etc/ssh/ssh_host_${key_type}_key"
        if [ ! -f "$key_path" ]; then
            missing_keys="${missing_keys:+$missing_keys }$key_path"
        fi
    done
}

if [ ! -d /etc/ssh ]; then
    echo "/etc/ssh does not exist; cannot generate SSH host keys" >&2
    exit 1
fi

collect_missing_keys

if [ ! -w /etc/ssh ]; then
    if [ -z "$missing_keys" ]; then
        exit 0
    fi
    echo "/etc/ssh is not writable and missing SSH host keys: $missing_keys" >&2
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

collect_missing_keys

if [ -n "$missing_keys" ]; then
    echo "missing SSH host keys after generation: $missing_keys" >&2
    exit 1
fi

# Keep host key file permissions explicit for sshd StrictModes and scanners.
find /etc/ssh -maxdepth 1 -type f -name 'ssh_host_*_key' -exec chmod 0600 {} +
find /etc/ssh -maxdepth 1 -type f -name 'ssh_host_*_key.pub' -exec chmod 0644 {} +

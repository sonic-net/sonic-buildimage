#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUDOERS_FILE="${SCRIPT_DIR}/../sudoers"
TEST_IMAGE="${SUDOERS_TEST_IMAGE:-debian:trixie-slim}"

docker run --rm -i \
    --mount "type=bind,src=${SUDOERS_FILE},dst=/tmp/sonic-sudoers,readonly" \
    "$TEST_IMAGE" bash <<'EOF'
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq smartmontools sudo

install -o root -g root -m 0440 /tmp/sonic-sudoers /etc/sudoers
useradd --create-home readonly
visudo -cf /etc/sudoers

cat >/usr/sbin/smartctl <<'SMARTCTL'
#!/bin/sh
printf '%s|%s\n' "$(id -u)" "$*"
SMARTCTL
chmod 0755 /usr/sbin/smartctl

allowed_output="$(
    su -s /bin/sh readonly -c \
        'sudo -n /usr/sbin/smartctl -a /dev/sda'
)"
test "$allowed_output" = "0|-a /dev/sda"

assert_denied() {
    if sudo -l -U readonly "$@" >/dev/null 2>&1; then
        printf 'Unexpectedly authorized:' >&2
        printf ' %q' "$@" >&2
        printf '\n' >&2
        exit 1
    fi
}

assert_denied /usr/sbin/smartctl -a /dev/sdb
assert_denied /usr/sbin/smartctl -a /dev/sda --json
assert_denied /usr/sbin/smartctl -t long /dev/sda
assert_denied /usr/sbin/smartctl -i /dev/sda
assert_denied /usr/sbin/smartctl -a /dev/sda extra

readonly_uid="$(id -u readonly)"
semicolon_output="$(
    su -s /bin/sh readonly -c \
        'sudo -n /usr/sbin/smartctl -a /dev/sda; id -u'
)"
test "$semicolon_output" = "$(printf '0|-a /dev/sda\n%s' "$readonly_uid")"

and_output="$(
    su -s /bin/sh readonly -c \
        'sudo -n /usr/sbin/smartctl -a /dev/sda && id -u'
)"
test "$and_output" = "$(printf '0|-a /dev/sda\n%s' "$readonly_uid")"
EOF

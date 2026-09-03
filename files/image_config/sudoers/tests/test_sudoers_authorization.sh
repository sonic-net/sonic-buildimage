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
apt-get install -y -qq sudo

install -o root -g root -m 0440 /tmp/sonic-sudoers /etc/sudoers
useradd --create-home readonly
visudo -cf /etc/sudoers

install_stub() {
    cat >"$1" <<'STUB'
#!/bin/sh
printf '%s|%s|%s\n' "$(id -u)" "$0" "$*"
STUB
    chmod 0755 "$1"
}

install_stub /usr/bin/TSC
install_stub /usr/bin/chage
install_stub /usr/bin/dmesg
install_stub /usr/bin/docker
install_stub /usr/bin/systemctl
install_stub /usr/sbin/dmidecode

assert_allowed() {
    command="$1"
    expected="$2"
    output="$(su -s /bin/sh readonly -c "sudo -n ${command}")"
    test "$output" = "0|${expected}"
}

assert_denied() {
    if sudo -l -U readonly "$@" >/dev/null 2>&1; then
        printf 'Unexpectedly authorized:' >&2
        printf ' %q' "$@" >&2
        printf '\n' >&2
        exit 1
    fi
}

assert_allowed "/usr/bin/systemctl status" "/usr/bin/systemctl|status"
assert_allowed "/usr/bin/systemctl status swss.service" \
    "/usr/bin/systemctl|status swss.service"
assert_allowed "/usr/bin/TSC" "/usr/bin/TSC|"
assert_allowed "/usr/sbin/dmidecode -s system-product-name" \
    "/usr/sbin/dmidecode|-s system-product-name"
assert_allowed "/usr/bin/dmesg -D" "/usr/bin/dmesg|-D"
assert_allowed "/usr/bin/chage -l readonly" "/usr/bin/chage|-l readonly"
assert_allowed "/usr/bin/docker exec swss md5sum /usr/bin/arp_update" \
    "/usr/bin/docker|exec swss md5sum /usr/bin/arp_update"

assert_denied /usr/bin/systemctl restart swss.service
assert_denied /usr/bin/TSC no-stats
assert_denied /usr/sbin/dmidecode -s system-serial-number
assert_denied /usr/bin/dmesg -C
assert_denied /usr/bin/chage -M 30 readonly
assert_denied /usr/bin/chage -l readonly extra
assert_denied /usr/bin/docker exec swss md5sum /usr/bin/other
assert_denied /usr/bin/docker exec swss md5sum /usr/bin/arp_update extra
assert_denied /usr/bin/rvtysh -c configure
EOF

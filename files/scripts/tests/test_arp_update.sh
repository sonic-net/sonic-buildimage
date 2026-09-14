#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/arp_update"
SOURCE_SCRIPT="$(mktemp)"
DB_CALL_LOG="$(mktemp)"
DB_KEY_LOG="$(mktemp)"
MARKER_FILE="/tmp/arp-update-test-marker"
rm -f "$MARKER_FILE"
trap 'rm -f "${SOURCE_SCRIPT}" "${DB_CALL_LOG}" "${DB_KEY_LOG}" "${MARKER_FILE}"' EXIT

# Load only the helper functions; the remainder of arp_update is an infinite loop.
sed '/^while \/bin\/true; do$/,$d' "$SCRIPT" > "$SOURCE_SCRIPT"
# shellcheck source=/dev/null
source "$SOURCE_SCRIPT"

assert_eq() {
    local description="$1"
    local expected="$2"
    local actual="$3"
    if [[ "$expected" != "$actual" ]]; then
        echo "FAIL: ${description}: expected '${expected}', got '${actual}'" >&2
        exit 1
    fi
}

logger() {
    :
}

timeout() {
    TIMEOUT_ARGS=("$@")
}

sonic-db-cli() {
    printf '%s\n' "$*" > "$DB_CALL_LOG"
    printf '%s\n' "$3" > "$DB_KEY_LOG"
}

ip() {
    IP_ARGS=("$@")
}

arping() {
    ARPING_ARGS=("$@")
}

ndisc6() {
    NDISC6_ARGS=("$@")
}

TIMEOUT_ARGS=()
run_ipv6_multicast_ping 0.2 "Ethernet 0"
assert_eq "multicast interface remains one argument" "Ethernet 0" "${TIMEOUT_ARGS[3]}"

TIMEOUT_ARGS=()
run_ipv6_multicast_ping 0.2 'Ethernet 0; touch /tmp/arp-update-test-marker'
assert_eq "multicast command preserves an untrusted interface" \
    'Ethernet 0; touch /tmp/arp-update-test-marker' "${TIMEOUT_ARGS[3]}"

ARPING_ARGS=()
run_arping_with_retry 'Ethernet 0; touch /tmp/arp-update-test-marker' '192.0.2.1; touch /tmp/arp-update-test-marker'
assert_eq "arping preserves the interface argument" \
    'Ethernet 0; touch /tmp/arp-update-test-marker' "${ARPING_ARGS[6]}"
assert_eq "arping preserves the address argument" \
    '192.0.2.1; touch /tmp/arp-update-test-marker' "${ARPING_ARGS[7]}"

NDISC6_ARGS=()
run_ndisc6_with_retry 'Ethernet 0; touch /tmp/arp-update-test-marker' '2001:db8::1; touch /tmp/arp-update-test-marker'
assert_eq "ndisc6 preserves the address argument" \
    '2001:db8::1; touch /tmp/arp-update-test-marker' "${NDISC6_ARGS[4]}"
assert_eq "ndisc6 preserves the interface argument" \
    'Ethernet 0; touch /tmp/arp-update-test-marker' "${NDISC6_ARGS[5]}"

IP_ARGS=()
flush_unsynced_neighbors "Vlan1000" "2001:db8::1 dev Vlan1000 FAILED"
assert_eq "APPL_DB neighbor lookup" \
    "APPL_DB hget NEIGH_TABLE:Vlan1000:2001:db8::1 neigh" "$(<"$DB_CALL_LOG")"
assert_eq "flush address remains one argument" "2001:db8::1" "${IP_ARGS[2]}"

IP_ARGS=()
flush_unsynced_neighbors \
    'Vlan1000;touch' \
    '2001:db8::1;touch dev Vlan1000 FAILED'
assert_eq "DB key preserves an untrusted VLAN argument" \
    'NEIGH_TABLE:Vlan1000;touch:2001:db8::1;touch' \
    "$(<"$DB_KEY_LOG")"
assert_eq "flush preserves an untrusted address argument" \
    '2001:db8::1;touch' "${IP_ARGS[2]}"

IP_ARGS=()
replace_failed_neighbors "2001:db8::2 dev Vlan1000 FAILED"
assert_eq "replace address remains one argument" "2001:db8::2" "${IP_ARGS[2]}"
assert_eq "replace interface remains one argument" "Vlan1000" "${IP_ARGS[4]}"

FILTERED_NEIGHBORS=$(printf '%s\n' \
    '192.0.2.1 dev Vlan10 lladdr 00:11:22:33:44:55 REACHABLE' \
    '192.0.2.2 dev Vlan100 lladdr 00:11:22:33:44:66 REACHABLE' \
    '2001:db8::1 dev Vlan10 FAILED' | filter_neighbors_by_vlan 'Vlan10')
assert_eq "neighbor filtering matches the complete VLAN interface name" \
    $'192.0.2.1 dev Vlan10 lladdr 00:11:22:33:44:55 REACHABLE\n2001:db8::1 dev Vlan10 FAILED' \
    "$FILTERED_NEIGHBORS"

[[ ! -e "$MARKER_FILE" ]]

echo "arp_update helper tests passed"

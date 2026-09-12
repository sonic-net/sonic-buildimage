#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/arp_update"
SOURCE_SCRIPT="$(mktemp)"
DB_CALL_LOG="$(mktemp)"
trap 'rm -f "${SOURCE_SCRIPT}" "${DB_CALL_LOG}"' EXIT

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
}

ip() {
    IP_ARGS=("$@")
}

TIMEOUT_ARGS=()
run_ipv6_multicast_ping 0.2 "Ethernet 0"
assert_eq "multicast interface remains one argument" "Ethernet 0" "${TIMEOUT_ARGS[3]}"

IP_ARGS=()
flush_unsynced_neighbors "Vlan1000" "2001:db8::1 dev Vlan1000 FAILED"
assert_eq "APPL_DB neighbor lookup" \
    "APPL_DB hget NEIGH_TABLE:Vlan1000:2001:db8::1 neigh" "$(<"$DB_CALL_LOG")"
assert_eq "flush address remains one argument" "2001:db8::1" "${IP_ARGS[2]}"

IP_ARGS=()
replace_failed_neighbors "2001:db8::2 dev Vlan1000 FAILED"
assert_eq "replace address remains one argument" "2001:db8::2" "${IP_ARGS[2]}"
assert_eq "replace interface remains one argument" "Vlan1000" "${IP_ARGS[4]}"

echo "arp_update helper tests passed"

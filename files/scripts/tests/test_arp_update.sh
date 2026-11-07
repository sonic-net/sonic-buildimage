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
    printf '%s\n' "$4" > "$DB_KEY_LOG"
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
    "-s APPL_DB hget NEIGH_TABLE:Vlan1000:2001:db8::1 neigh" "$(<"$DB_CALL_LOG")"
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

# MAC-mismatch flush: APPL_DB rows are emulated by key, and a row that
# neighsyncd wrote but orchagent has not popped yet lives only in _NEIGH_TABLE.
declare -A APPL_DB_ROWS
sonic-db-cli() {
    if [[ "$1" == "-s" && "$2" == "APPL_DB" && "$3" == "hget" && "$5" == "neigh" ]]; then
        printf '%s' "${APPL_DB_ROWS[$4]-}"
    fi
}

# Prints "flushed" or "kept", or what went wrong.
mismatch_flushes() {
    IP_ARGS=()
    TIMEOUT_ARGS=()
    if flush_mac_mismatch_neighbor "$@"; then
        if [[ "${IP_ARGS[*]}" != "neigh flush $2" || "${TIMEOUT_ARGS[3]-}" != "$1" ]]; then
            echo "flushed with ip '${IP_ARGS[*]}' and ping '${TIMEOUT_ARGS[*]}'"
            return
        fi
        echo flushed
    elif [[ ${#IP_ARGS[@]} -ne 0 || ${#TIMEOUT_ARGS[@]} -ne 0 ]]; then
        echo "kept but ran ip '${IP_ARGS[*]}' and ping '${TIMEOUT_ARGS[*]}'"
    else
        echo kept
    fi
}

APPL_DB_ROWS=(["NEIGH_TABLE:Vlan1000:192.0.2.10"]="02:00:00:00:00:10")
assert_eq "kernel MAC matches NEIGH_TABLE" "kept" \
    "$(mismatch_flushes Vlan1000 192.0.2.10 02:00:00:00:00:10)"

APPL_DB_ROWS=(["_NEIGH_TABLE:Vlan1000:192.0.2.11"]="02:00:00:00:00:11")
assert_eq "new neighbor queued for orchagent" "kept" \
    "$(mismatch_flushes Vlan1000 192.0.2.11 02:00:00:00:00:11)"

APPL_DB_ROWS=(["_NEIGH_TABLE:Vlan1000:2001:db8::11"]="02:00:00:00:00:11")
assert_eq "new IPv6 neighbor queued for orchagent" "kept" \
    "$(mismatch_flushes Vlan1000 2001:db8::11 02:00:00:00:00:11)"

APPL_DB_ROWS=(["NEIGH_TABLE:Vlan1000:192.0.2.12"]="02:00:00:00:00:01"
              ["_NEIGH_TABLE:Vlan1000:192.0.2.12"]="02:00:00:00:00:12")
assert_eq "changed MAC queued for orchagent" "kept" \
    "$(mismatch_flushes Vlan1000 192.0.2.12 02:00:00:00:00:12)"

APPL_DB_ROWS=(["NEIGH_TABLE:Vlan1000:192.0.2.13"]="02:00:00:00:00:01"
              ["_NEIGH_TABLE:Vlan1000:192.0.2.13"]="02:00:00:00:00:99")
assert_eq "neither APPL_DB row has the kernel MAC" "flushed" \
    "$(mismatch_flushes Vlan1000 192.0.2.13 02:00:00:00:00:13)"

APPL_DB_ROWS=(["NEIGH_TABLE:Vlan1000:192.0.2.14"]="02:00:00:00:00:01")
assert_eq "NEIGH_TABLE MAC differs from the kernel" "flushed" \
    "$(mismatch_flushes Vlan1000 192.0.2.14 02:00:00:00:00:14)"

APPL_DB_ROWS=()
assert_eq "neighbor missing from APPL_DB" "flushed" \
    "$(mismatch_flushes Vlan1000 192.0.2.15 02:00:00:00:00:15)"

[[ ! -e "$MARKER_FILE" ]]

echo "arp_update helper tests passed"

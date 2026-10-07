#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/arp_update"
SOURCE_SCRIPT="$(mktemp)"
RECHECK_SCRIPT="$(mktemp)"
DB_CALL_LOG="$(mktemp)"
DB_KEY_LOG="$(mktemp)"
KERNEL_CALL_LOG="$(mktemp)"
MARKER_FILE="/tmp/arp-update-test-marker"
rm -f "$MARKER_FILE"
trap 'rm -f "${SOURCE_SCRIPT}" "${RECHECK_SCRIPT}" "${DB_CALL_LOG}" "${DB_KEY_LOG}" "${KERNEL_CALL_LOG}" "${MARKER_FILE}"' EXIT

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

APPL_DB_MAC=""
DB_READ_STATUS=0
RECHECK_AFTER_SLEEP=false
KERNEL_RECHECK_IP="192.0.2.1"
KERNEL_RECHECK_INTF="Vlan1000"
KERNEL_RECHECK_OUTPUT=""
KERNEL_READ_STATUS=0
KERNEL_FLUSH_STATUS=0
KERNEL_FLUSH_FAIL_INTF=""
SCOPED_FLUSH_CALLS=()

sonic-db-cli() {
    printf '%s\n' "$*" > "$DB_CALL_LOG"
    printf '%s\n' "$3" > "$DB_KEY_LOG"
    assert_eq "neighbor lookup uses APPL_DB" "APPL_DB" "$1"
    if [[ "$RECHECK_AFTER_SLEEP" == true ]]; then
        assert_eq "recheck follows the existing wait" "120" "${SLEEP_ARGS[*]}"
    fi
    if [[ "$DB_READ_STATUS" -ne 0 ]]; then
        return "$DB_READ_STATUS"
    fi
    printf '%s' "$APPL_DB_MAC"
}

ip() {
    case "$*" in
        "neigh show")
            printf '%s\n' "$KERNEL_NEIGH_OUTPUT"
            ;;
        "-4 neigh show to "*|"-6 neigh show to "*)
            printf '%s\n' "$*" >> "$KERNEL_CALL_LOG"
            if [[ "$KERNEL_READ_STATUS" -ne 0 ]]; then
                return "$KERNEL_READ_STATUS"
            fi
            if [[ "$5" == "$KERNEL_RECHECK_IP" && ( "$7" == "$KERNEL_RECHECK_INTF" || "$KERNEL_RECHECK_INTF" == "*" ) ]]; then
                printf '%s' "$KERNEL_RECHECK_OUTPUT"
            fi
            ;;
        "-4 neigh flush to "*|"-6 neigh flush to "*)
            IP_ARGS=("$@")
            SCOPED_FLUSH_CALLS+=("$*")
            if [[ -z "$KERNEL_FLUSH_FAIL_INTF" || "$7" == "$KERNEL_FLUSH_FAIL_INTF" ]]; then
                return "$KERNEL_FLUSH_STATUS"
            fi
            ;;
        *)
            IP_ARGS=("$@")
            ;;
    esac
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

APPL_DB_MAC=""
KERNEL_RECHECK_OUTPUT="192.0.2.1 dev Vlan1000 lladdr 00:11:22:33:44:55 REACHABLE"
LOGGER_ARGS=()
IP_ARGS=()
TIMEOUT_ARGS=()
logger() { LOGGER_ARGS=("$@"); }
recheck_missing_neighbors $'192.0.2.1,Vlan1000\n'
assert_eq "recheck reports the missing neighbor" \
    "Neighbor 192.0.2.1 on Vlan1000 present in kernel but still missing from APPL_DB after recheck; flushing without probing" \
    "${LOGGER_ARGS[2]}"
assert_eq "recheck flushes only the exact neighbor" \
    "-4 neigh flush to 192.0.2.1 dev Vlan1000 nud all" "${IP_ARGS[*]}"
assert_eq "recheck does not recreate the neighbor with a probe" "0" "${#TIMEOUT_ARGS[@]}"

APPL_DB_MAC="00:11:22:33:44:55"
LOGGER_ARGS=()
: > "$KERNEL_CALL_LOG"
recheck_missing_neighbors $'192.0.2.1,Vlan1000\n'
assert_eq "recheck stays quiet once APPL_DB catches up" "0" "${#LOGGER_ARGS[@]}"
assert_eq "recovered APPL_DB entry needs no kernel lookup" "" "$(<"$KERNEL_CALL_LOG")"

# Exercise the existing inline MAC loop without starting the daemon.
sed -n '/^  # Flush neighbor entries with MAC mismatch/,/^  VLAN=/{
    /^  VLAN=/d
    p
}' "$SCRIPT" > "$SOURCE_SCRIPT"

sed -n '/^  # sleep here before handling the mismatch/,/^  # refresh neighbor entries/{
    /^  # refresh neighbor entries/d
    p
}' "$SCRIPT" > "$RECHECK_SCRIPT"

logger() {
    LOGGER_MESSAGES+=("$*")
}

sleep() {
    SLEEP_ARGS+=("$@")
}

run_mac_mismatch_loop() {
    IP_ARGS=()
    TIMEOUT_ARGS=()
    SLEEP_ARGS=()
    LOGGER_MESSAGES=()
    : > "$KERNEL_CALL_LOG"
    source "$SOURCE_SCRIPT"
}

for neighbor_ip in "192.0.2.1" "2001:db8::1"; do
    KERNEL_NEIGH_OUTPUT="$neighbor_ip dev Vlan1000 lladdr 00:11:22:33:44:55 REACHABLE"
    KERNEL_RECHECK_IP="$neighbor_ip"
    KERNEL_RECHECK_INTF="Vlan1000"
    KERNEL_RECHECK_OUTPUT="$KERNEL_NEIGH_OUTPUT"
    if [[ "$neighbor_ip" == *":"* ]]; then
        expected_family=-6
    else
        expected_family=-4
    fi
    APPL_DB_MAC=""
    run_mac_mismatch_loop
    assert_eq "missing entry keeps the existing mismatch warning" \
        "-p warning MAC mismatch for $neighbor_ip on Vlan1000 - kernel: 00:11:22:33:44:55, APPL_DB: " \
        "${LOGGER_MESSAGES[0]}"
    assert_eq "missing entry is not flushed" "0" "${#IP_ARGS[@]}"
    assert_eq "missing entry is not probed" "0" "${#TIMEOUT_ARGS[@]}"
    assert_eq "missing entry does not add a sleep to the MAC loop" "0" "${#SLEEP_ARGS[@]}"
    assert_eq "missing-entry recheck is deferred" "1" "${#LOGGER_MESSAGES[@]}"
    assert_eq "missing entry is confirmed immediately in the live kernel" \
        "$expected_family neigh show to $neighbor_ip dev Vlan1000" "$(<"$KERNEL_CALL_LOG")"
    assert_eq "only the confirmed candidate is queued" "$neighbor_ip,Vlan1000"$'\n' "$MISSING_APPL_DB_NEIGH"

    : > "$KERNEL_CALL_LOG"
    RECHECK_AFTER_SLEEP=true
    source "$RECHECK_SCRIPT"
    RECHECK_AFTER_SLEEP=false
    assert_eq "deferred recheck uses only the existing wait" "120" "${SLEEP_ARGS[*]}"
    assert_eq "kernel recheck selects the exact IP, interface and family" \
        "$expected_family neigh show to $neighbor_ip dev Vlan1000" "$(<"$KERNEL_CALL_LOG")"
    assert_eq "persistent mismatch and mitigation are logged" \
        "-p warning Neighbor $neighbor_ip on Vlan1000 present in kernel but still missing from APPL_DB after recheck; flushing without probing" \
        "${LOGGER_MESSAGES[1]}"
    assert_eq "persistent mismatch is flushed by exact family, IP and interface" \
        "$expected_family neigh flush to $neighbor_ip dev Vlan1000 nud all" "${IP_ARGS[*]}"
    assert_eq "delayed cleanup does not probe" "0" "${#TIMEOUT_ARGS[@]}"

    run_mac_mismatch_loop
    KERNEL_RECHECK_OUTPUT=""
    RECHECK_AFTER_SLEEP=true
    source "$RECHECK_SCRIPT"
    RECHECK_AFTER_SLEEP=false
    assert_eq "deletion from both tables keeps only the initial warning" "1" "${#LOGGER_MESSAGES[@]}"
    assert_eq "deletion from both tables does not flush" "0" "${#IP_ARGS[@]}"
    assert_eq "deletion from both tables does not probe" "0" "${#TIMEOUT_ARGS[@]}"

    KERNEL_RECHECK_OUTPUT="$KERNEL_NEIGH_OUTPUT"
    run_mac_mismatch_loop
    KERNEL_RECHECK_INTF="Vlan2000"
    : > "$KERNEL_CALL_LOG"
    RECHECK_AFTER_SLEEP=true
    source "$RECHECK_SCRIPT"
    RECHECK_AFTER_SLEEP=false
    assert_eq "neighbor on a different interface is not a mismatch" "1" "${#LOGGER_MESSAGES[@]}"
    assert_eq "delayed lookup remains scoped to the original interface" \
        "$expected_family neigh show to $neighbor_ip dev Vlan1000" "$(<"$KERNEL_CALL_LOG")"
    assert_eq "a neighbor on another interface is not flushed" "0" "${#IP_ARGS[@]}"
    KERNEL_RECHECK_INTF="Vlan1000"

    for recovered_mac in "00:11:22:33:44:55" "00:00:00:00:00:00"; do
        APPL_DB_MAC=""
        run_mac_mismatch_loop
        APPL_DB_MAC="$recovered_mac"
        : > "$KERNEL_CALL_LOG"
        RECHECK_AFTER_SLEEP=true
        source "$RECHECK_SCRIPT"
        RECHECK_AFTER_SLEEP=false
        assert_eq "recovered entry is rechecked after the existing wait" "120" "${SLEEP_ARGS[*]}"
        assert_eq "entry recovered during the wait does not log an error" "1" "${#LOGGER_MESSAGES[@]}"
        assert_eq "entry recovered during the wait needs no kernel lookup" "" "$(<"$KERNEL_CALL_LOG")"
        assert_eq "recovered entry is not flushed" "0" "${#IP_ARGS[@]}"
        assert_eq "recovered entry is not probed" "0" "${#TIMEOUT_ARGS[@]}"
    done

    APPL_DB_MAC=""
    KERNEL_RECHECK_OUTPUT=""
    run_mac_mismatch_loop
    assert_eq "an already deleted kernel neighbor is not queued" "" "$MISSING_APPL_DB_NEIGH"
    assert_eq "an already deleted kernel neighbor is not flushed" "0" "${#IP_ARGS[@]}"
    assert_eq "an already deleted kernel neighbor is not probed" "0" "${#TIMEOUT_ARGS[@]}"
    : > "$DB_CALL_LOG"
    : > "$KERNEL_CALL_LOG"
    RECHECK_AFTER_SLEEP=true
    source "$RECHECK_SCRIPT"
    RECHECK_AFTER_SLEEP=false
    assert_eq "an already deleted neighbor needs no delayed DB lookup" "" "$(<"$DB_CALL_LOG")"
    assert_eq "an already deleted neighbor needs no delayed kernel lookup" "" "$(<"$KERNEL_CALL_LOG")"
    KERNEL_RECHECK_OUTPUT="$KERNEL_NEIGH_OUTPUT"

    KERNEL_RECHECK_INTF="Vlan2000"
    run_mac_mismatch_loop
    assert_eq "a neighbor only on another interface is not queued" "" "$MISSING_APPL_DB_NEIGH"
    assert_eq "another interface is not flushed" "0" "${#IP_ARGS[@]}"
    KERNEL_RECHECK_INTF="Vlan1000"

    DB_READ_STATUS=1
    run_mac_mismatch_loop
    assert_eq "initial DB read failure is reported separately" \
        "-p error Failed to read APPL_DB neighbor $neighbor_ip on Vlan1000 during MAC check" "${LOGGER_MESSAGES[0]}"
    assert_eq "initial DB read failure does not authorize a kernel lookup" "" "$(<"$KERNEL_CALL_LOG")"
    assert_eq "initial DB read failure does not queue cleanup" "" "$MISSING_APPL_DB_NEIGH"
    assert_eq "initial DB read failure does not flush" "0" "${#IP_ARGS[@]}"
    assert_eq "initial DB read failure does not probe" "0" "${#TIMEOUT_ARGS[@]}"
    DB_READ_STATUS=0

    KERNEL_READ_STATUS=1
    run_mac_mismatch_loop
    assert_eq "initial kernel read failure is reported separately" \
        "-p error Failed to read kernel neighbor $neighbor_ip on Vlan1000 during MAC check" "${LOGGER_MESSAGES[1]}"
    assert_eq "initial kernel read failure does not queue cleanup" "" "$MISSING_APPL_DB_NEIGH"
    assert_eq "initial kernel read failure does not flush" "0" "${#IP_ARGS[@]}"
    assert_eq "initial kernel read failure does not probe" "0" "${#TIMEOUT_ARGS[@]}"
    KERNEL_READ_STATUS=0

    APPL_DB_MAC="00:11:22:33:44:55"
    run_mac_mismatch_loop
    assert_eq "matching MACs are not flushed" "0" "${#IP_ARGS[@]}"
    assert_eq "matching MACs are not probed" "0" "${#TIMEOUT_ARGS[@]}"
    assert_eq "matching MACs do not add a delay" "0" "${#SLEEP_ARGS[@]}"
    assert_eq "matching MACs do not warn" "0" "${#LOGGER_MESSAGES[@]}"
    : > "$DB_CALL_LOG"
    source "$RECHECK_SCRIPT"
    assert_eq "no pending entries leave the existing wait unchanged" "120" "${SLEEP_ARGS[*]}"
    assert_eq "no pending entries cause no extra DB lookups" "" "$(<"$DB_CALL_LOG")"
    assert_eq "no pending entries cause no kernel lookups" "" "$(<"$KERNEL_CALL_LOG")"

    if [[ "$neighbor_ip" == *":"* ]]; then
        expected_ping=ping6
    else
        expected_ping=ping
    fi
    for APPL_DB_MAC in "00:11:22:33:44:66" "00:00:00:00:00:00"; do
        run_mac_mismatch_loop
        assert_eq "nonempty mismatch keeps the existing flush" \
            "neigh flush $neighbor_ip" "${IP_ARGS[*]}"
        assert_eq "nonempty mismatch keeps the family and interface-specific probe" \
            "0.2 $expected_ping -I Vlan1000 -n -q -i 0 -c 1 -W 1 $neighbor_ip" "${TIMEOUT_ARGS[*]}"
        assert_eq "nonempty mismatch does not add a delay" "0" "${#SLEEP_ARGS[@]}"
    done

    APPL_DB_MAC=""
    DB_READ_STATUS=1
    LOGGER_MESSAGES=()
    IP_ARGS=()
    TIMEOUT_ARGS=()
    : > "$KERNEL_CALL_LOG"
    if recheck_missing_neighbors "$neighbor_ip,Vlan1000"; then
        echo "FAIL: APPL_DB read failure passed the recheck" >&2
        exit 1
    fi
    assert_eq "APPL_DB read failure is not reported as a mismatch" \
        "-p error Failed to read APPL_DB neighbor $neighbor_ip on Vlan1000 during recheck" "${LOGGER_MESSAGES[0]}"
    assert_eq "APPL_DB read failure causes no kernel lookup" "" "$(<"$KERNEL_CALL_LOG")"
    assert_eq "APPL_DB read failure does not flush" "0" "${#IP_ARGS[@]}"
    assert_eq "APPL_DB read failure does not probe" "0" "${#TIMEOUT_ARGS[@]}"
    DB_READ_STATUS=0

    KERNEL_READ_STATUS=1
    LOGGER_MESSAGES=()
    if recheck_missing_neighbors "$neighbor_ip,Vlan1000"; then
        echo "FAIL: kernel read failure passed the recheck" >&2
        exit 1
    fi
    assert_eq "kernel read failure is not mistaken for deletion" \
        "-p error Failed to read kernel neighbor $neighbor_ip on Vlan1000 during recheck" "${LOGGER_MESSAGES[0]}"
    assert_eq "kernel read failure does not flush" "0" "${#IP_ARGS[@]}"
    assert_eq "kernel read failure does not probe" "0" "${#TIMEOUT_ARGS[@]}"
    KERNEL_READ_STATUS=0

    KERNEL_FLUSH_STATUS=1
    LOGGER_MESSAGES=()
    if recheck_missing_neighbors "$neighbor_ip,Vlan1000"; then
        echo "FAIL: kernel flush failure passed the recheck" >&2
        exit 1
    fi
    assert_eq "kernel flush failure is reported" \
        "-p error Failed to flush kernel neighbor $neighbor_ip on Vlan1000 during recheck" "${LOGGER_MESSAGES[1]}"
    assert_eq "kernel flush failure does not fall back to probing" "0" "${#TIMEOUT_ARGS[@]}"
    KERNEL_FLUSH_STATUS=0

    for neighbor_state in REACHABLE STALE FAILED PERMANENT; do
        KERNEL_RECHECK_OUTPUT="$neighbor_ip dev Vlan1000 lladdr 00:11:22:33:44:55 $neighbor_state"
        IP_ARGS=()
        recheck_missing_neighbors "$neighbor_ip,Vlan1000"
        assert_eq "confirmed cleanup selects all neighbor states without widening its scope" \
            "$expected_family neigh flush to $neighbor_ip dev Vlan1000 nud all" "${IP_ARGS[*]}"
    done

    LOGGER_MESSAGES=()
    IP_ARGS=()
    : > "$KERNEL_CALL_LOG"
    recheck_missing_neighbors $'\n'"$neighbor_ip,Vlan2000"$'\n'"$neighbor_ip,Vlan1000"$'\n'
    assert_eq "skipping an absent candidate does not skip later candidates" "1" "${#LOGGER_MESSAGES[@]}"
    assert_eq "a mixed candidate list flushes only the matching interface" \
        "$expected_family neigh flush to $neighbor_ip dev Vlan1000 nud all" "${IP_ARGS[*]}"

    KERNEL_RECHECK_INTF="*"
    KERNEL_FLUSH_STATUS=1
    KERNEL_FLUSH_FAIL_INTF="Vlan2000"
    SCOPED_FLUSH_CALLS=()
    LOGGER_MESSAGES=()
    if recheck_missing_neighbors "$neighbor_ip,Vlan2000"$'\n'"$neighbor_ip,Vlan1000"; then
        echo "FAIL: a mixed cleanup hid a flush failure" >&2
        exit 1
    fi
    assert_eq "cleanup continues after one candidate fails" "2" "${#SCOPED_FLUSH_CALLS[@]}"
    assert_eq "the first candidate remains scoped when its flush fails" \
        "$expected_family neigh flush to $neighbor_ip dev Vlan2000 nud all" "${SCOPED_FLUSH_CALLS[0]}"
    assert_eq "the second candidate is still mitigated after the first fails" \
        "$expected_family neigh flush to $neighbor_ip dev Vlan1000 nud all" "${SCOPED_FLUSH_CALLS[1]}"
    assert_eq "a mixed cleanup reports the failed interface" \
        "-p error Failed to flush kernel neighbor $neighbor_ip on Vlan2000 during recheck" "${LOGGER_MESSAGES[1]}"
    assert_eq "a mixed cleanup does not fall back to probing" "0" "${#TIMEOUT_ARGS[@]}"
    KERNEL_RECHECK_INTF="Vlan1000"
    KERNEL_FLUSH_STATUS=0
    KERNEL_FLUSH_FAIL_INTF=""
done

KERNEL_RECHECK_IP='192.0.2.1; touch /tmp/arp-update-test-marker'
KERNEL_RECHECK_INTF='Vlan1000; touch /tmp/arp-update-test-marker'
KERNEL_RECHECK_OUTPUT="$KERNEL_RECHECK_IP dev $KERNEL_RECHECK_INTF lladdr 00:11:22:33:44:55 REACHABLE"
recheck_missing_neighbors "$KERNEL_RECHECK_IP,$KERNEL_RECHECK_INTF"
assert_eq "scoped cleanup preserves the address as one argument" "$KERNEL_RECHECK_IP" "${IP_ARGS[4]}"
assert_eq "scoped cleanup preserves the interface as one argument" "$KERNEL_RECHECK_INTF" "${IP_ARGS[6]}"

[[ ! -e "$MARKER_FILE" ]]

echo "arp_update helper tests passed"

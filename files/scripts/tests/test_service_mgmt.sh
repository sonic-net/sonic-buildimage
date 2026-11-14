#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${1:-${SCRIPT_DIR}/service_mgmt.sh}"
SOURCE_SCRIPT="$(mktemp)"
trap 'rm -f "$SOURCE_SCRIPT"' EXIT

# Load the functions without dispatching a real service command.
sed '/^DEV=\$2$/,$d' "$SCRIPT" > "$SOURCE_SCRIPT"
# shellcheck source=/dev/null
source "$SOURCE_SCRIPT"

debug() {
    LOG_MESSAGES+=("$1")
    return 0
}

/usr/bin/database.sh() {
    CHILD_ARGS=("$@")
    CHILD_CALLS=$((CHILD_CALLS + 1))
    return "$CHILD_STATUS"
}

/usr/bin/restapi.sh() {
    CHILD_ARGS=("$@")
    CHILD_CALLS=$((CHILD_CALLS + 1))
    return "$CHILD_STATUS"
}

assert_eq() {
    if [[ "$2" != "$3" ]]; then
        echo "FAIL: $1: expected '$2', got '$3'" >&2
        exit 1
    fi
}

CASES=0
check_start() {
    SERVICE="$1"
    DEV="$2"
    CHILD_STATUS="$3"
    CHILD_CALLS=0
    CHILD_ARGS=()
    LOG_MESSAGES=()
    local actual
    if start; then actual=0; else actual=$?; fi
    assert_eq "child status for $SERVICE/$DEV" "$CHILD_STATUS" "$actual"
    assert_eq "TERM trap is cleared" "" "$(trap -p TERM)"
    assert_eq "child invoked once" 1 "$CHILD_CALLS"
    assert_eq "child action" start "${CHILD_ARGS[0]}"
    if [[ -n "$DEV" ]]; then
        assert_eq "device argument count" 2 "${#CHILD_ARGS[@]}"
        assert_eq "device argument" "$DEV" "${CHILD_ARGS[1]}"
    else
        assert_eq "default device argument count" 1 "${#CHILD_ARGS[@]}"
    fi
    if [[ "$CHILD_STATUS" == 0 ]]; then
        assert_eq "successful start logs" 2 "${#LOG_MESSAGES[@]}"
        assert_eq "successful start message" "Started ${SERVICE}$DEV service..." "${LOG_MESSAGES[1]}"
    else
        assert_eq "failed start has no success log" 1 "${#LOG_MESSAGES[@]}"
    fi
    CASES=$((CASES + 1))
}

for status in 0 1 37 126 127 143; do check_start database '' "$status"; done
for device in 0 dpu0; do
    for status in 0 37; do check_start database "$device" "$status"; done
done
for status in 0 37; do check_start restapi '' "$status"; done

echo "$CASES service_mgmt start cases passed"

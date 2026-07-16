#!/usr/bin/env bash

set -euo pipefail

PASS=0
FAIL=0
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/gnmi-native.sh"

# shellcheck disable=SC1090,SC2034
source "$SCRIPT"

pass() {
    echo "PASS: $1"
    (( ++PASS ))
}

fail() {
    echo "FAIL: $1" >&2
    (( ++FAIL ))
}

assert_eq() {
    local name="$1" expected="$2" actual="$3"
    if [[ "$expected" == "$actual" ]]; then
        pass "$name"
    else
        fail "$name (expected $(printf '%q' "$expected"), got $(printf '%q' "$actual"))"
    fi
}

assert_contains() {
    local name="$1" expected="$2" actual="$3"
    if [[ "$actual" == *"$expected"* ]]; then
        pass "$name"
    else
        fail "$name (missing $(printf '%q' "$expected") in $(printf '%q' "$actual"))"
    fi
}

assert_fails() {
    local name="$1"
    if configure_listener_mode >/dev/null 2>&1; then
        fail "$name"
    else
        pass "$name"
    fi
}

reset_config() {
    unset GNMI_LISTENER_MODE launch_by
    RUNTIME_OWNER="local"
    PORT=50052
    TELEMETRY_ARGS=" -logtostderr"
}

reset_config
configure_listener_mode >/dev/null
assert_eq "local default preserves configured port" "50052" "$PORT"
assert_eq "local default adds no listener arguments" " -logtostderr" "$TELEMETRY_ARGS"

reset_config
GNMI_LISTENER_MODE="config"
launch_by="k8s"
RUNTIME_OWNER="kube"
configure_listener_mode >/dev/null
assert_eq "explicit config mode preserves configured port" "50052" "$PORT"

reset_config
GNMI_LISTENER_MODE="uds-only"
launch_by="k8s"
RUNTIME_OWNER="kube"
configure_listener_mode >/dev/null
assert_eq "uds-only disables TCP port" "0" "$PORT"
assert_contains "uds-only selects the standard socket" \
    "--unix_socket /var/run/gnmi/gnmi.sock" "$TELEMETRY_ARGS"

reset_config
launch_by="k8s"
assert_fails "Kubernetes launch rejects a missing listener mode"

reset_config
RUNTIME_OWNER="kube"
assert_fails "Kubernetes owner rejects a missing listener mode"

reset_config
GNMI_LISTENER_MODE="invalid"
assert_fails "invalid listener mode is rejected"

echo "Results: ${PASS} passed, ${FAIL} failed"
(( FAIL == 0 ))

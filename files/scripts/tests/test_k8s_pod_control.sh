#!/bin/bash
# Unit tests for k8s_pod_control.sh
# Uses a mock docker command to test container discovery and restart logic.
#
# Usage: bash files/scripts/tests/test_k8s_pod_control.sh

set -euo pipefail

PASS=0
FAIL=0
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/k8s_pod_control.sh"

# ── helpers ──────────────────────────────────────────────────────────
red()   { printf '\033[31m%s\033[0m' "$*"; }
green() { printf '\033[32m%s\033[0m' "$*"; }

assert_eq() {
  local test_name="$1" expected="$2" actual="$3"
  if [[ "$expected" == "$actual" ]]; then
    echo "  $(green PASS): ${test_name}"
    (( ++PASS ))
  else
    echo "  $(red FAIL): ${test_name}"
    echo "    expected: $(printf '%q' "$expected")"
    echo "    actual:   $(printf '%q' "$actual")"
    (( ++FAIL ))
  fi
}

# ── Mock environment ──────────────────────────────────────────────────
MOCK_DIR="$(mktemp -d)"
trap 'rm -rf "${MOCK_DIR}"' EXIT

# Mock docker: handles ps and restart subcommands
cat > "${MOCK_DIR}/docker" <<'MOCK_DOCKER'
#!/bin/bash
subcmd="$1"; shift
case "$subcmd" in
  ps)
    quiet=false; all=false; format=""
    container_name=""; pod_namespace=""
    while (( $# > 0 )); do
      case "$1" in
        -q) quiet=true; shift ;;
        -a) all=true; shift ;;
        --filter)
          case "$2" in
            label=io.kubernetes.container.name=*)
              container_name="${2#label=io.kubernetes.container.name=}"
              ;;
            label=io.kubernetes.pod.namespace=*)
              pod_namespace="${2#label=io.kubernetes.pod.namespace=}"
              ;;
          esac
          shift 2
          ;;
        --format) format="$2"; shift 2 ;;
        *) shift ;;
      esac
    done
    # Only respond when namespace is sonic (matches script's NS).
    if [[ "$pod_namespace" != "sonic" ]]; then
      exit 0
    fi
    case "$container_name" in
      "telemetry")
        if $quiet; then
          echo "abc123def456"
        elif [[ "$format" == *".State"* ]]; then
          echo "ds-leafrouter-telemetry-zgmsl running"
        else
          echo "ds-leafrouter-telemetry-zgmsl"
        fi
        ;;
      "multi")
        if $quiet; then
          printf '%s\n' "aaa111" "bbb222"
        elif [[ "$format" == *".State"* ]]; then
          printf '%s\n' "pod-multi-1 running" "pod-multi-2 exited"
        else
          printf '%s\n' "pod-multi-1" "pod-multi-2"
        fi
        ;;
      "nonexistent")
        ;;  # no output
    esac
    ;;
  restart)
    cid="$1"
    case "$cid" in
      "abc123def456"|"aaa111"|"bbb222") ;;
      *) exit 1 ;;
    esac
    ;;
esac
MOCK_DOCKER
chmod +x "${MOCK_DIR}/docker"

# Mock logger (absorb all log output)
cat > "${MOCK_DIR}/logger" <<'MOCK_LOGGER'
#!/bin/bash
exit 0
MOCK_LOGGER
chmod +x "${MOCK_DIR}/logger"

export PATH="${MOCK_DIR}:${PATH}"

# Create a sourceable version of the script:
# - replace /usr/bin/logger with plain logger (so our PATH mock is used)
# - strip the case dispatch block at the bottom
SOURCE_SCRIPT="${MOCK_DIR}/k8s_pod_control_funcs.sh"
sed -e 's|/usr/bin/logger|logger|g' \
    -e '/^case "\${1:-}" in$/,/^esac$/d' \
    "$SCRIPT" > "$SOURCE_SCRIPT"

# Source functions with SERVICE_NAME pre-set
export SERVICE_NAME="telemetry"
source "$SOURCE_SCRIPT"

# ── Test suite ────────────────────────────────────────────────────────
echo "=== DOCKER_FILTERS ==="

assert_eq "DOCKER_FILTERS contains container.name filter for telemetry" \
  "label=io.kubernetes.container.name=telemetry" "${DOCKER_FILTERS[1]}"
assert_eq "DOCKER_FILTERS contains pod.namespace filter" \
  "label=io.kubernetes.pod.namespace=sonic" "${DOCKER_FILTERS[3]}"

# Helper to rebuild DOCKER_FILTERS for a different SERVICE_NAME (mirrors script)
set_filters_for() {
  DOCKER_FILTERS=(
    --filter "label=io.kubernetes.container.name=$1"
    --filter "label=io.kubernetes.pod.namespace=${NS}"
  )
}

echo ""
echo "=== container_ids_on_node ==="

result="$(container_ids_on_node)"
assert_eq "returns container ID for telemetry" "abc123def456" "$result"

# Switch to multi-container service
set_filters_for "multi"
result="$(container_ids_on_node)"
line_count=$(printf '%s\n' "$result" | grep -c .)
assert_eq "returns multiple container IDs" "2" "$line_count"

# No match
set_filters_for "nonexistent"
result="$(container_ids_on_node)"
assert_eq "no match returns empty" "" "$result"

echo ""
echo "=== pods_on_node ==="

set_filters_for "telemetry"
result="$(pods_on_node)"
assert_eq "returns pod name and state" \
  "ds-leafrouter-telemetry-zgmsl running" "$result"

set_filters_for "multi"
result="$(pods_on_node)"
line_count=$(printf '%s\n' "$result" | grep -c .)
assert_eq "returns multiple pods" "2" "$line_count"

set_filters_for "nonexistent"
result="$(pods_on_node)"
assert_eq "no match returns empty" "" "$result"

echo ""
echo "=== restart_containers ==="

set_filters_for "telemetry"
SERVICE_NAME="telemetry"
restart_containers; rc=$?
assert_eq "restart single container succeeds" "0" "$rc"

set_filters_for "multi"
SERVICE_NAME="multi"
restart_containers; rc=$?
assert_eq "restart multiple containers succeeds" "0" "$rc"

set_filters_for "nonexistent"
SERVICE_NAME="nonexistent"
restart_containers; rc=$?
assert_eq "restart with no containers is no-op" "0" "$rc"

echo ""
echo "=== cmd_status awk filter ==="

has_running() {
  awk '$2=="running"{found=1} END{exit found?0:1}' <<<"$1" && echo "yes" || echo "no"
}

result="$(has_running "ds-leafrouter-telemetry-zgmsl running")"
assert_eq "awk detects running" "yes" "$result"

result="$(has_running "ds-leafrouter-restapi-abc12 exited")"
assert_eq "awk detects exited" "no" "$result"

result="$(has_running $'pod1 running\npod2 exited')"
assert_eq "awk mixed (has running)" "yes" "$result"

result="$(has_running $'pod1 exited\npod2 exited')"
assert_eq "awk all exited" "no" "$result"

# ── Summary ───────────────────────────────────────────────────────────
echo ""
echo "=============================="
echo "Results: $(green "${PASS} passed"), $(red "${FAIL} failed")"
echo "=============================="

if (( FAIL > 0 )); then
  exit 1
fi
echo "PASS!!"
exit 0

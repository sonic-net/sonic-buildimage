#!/bin/bash
# Unit tests for k8s_pod_control.sh
# Validates jq filtering logic by mocking kubelet /pods JSON responses.
#
# Usage: bash files/scripts/tests/test_k8s_pod_control.sh

set -euo pipefail

PASS=0
FAIL=0
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

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

# ── sample kubelet /pods JSON ─────────────────────────────────────────
# Mimics the structure returned by https://localhost:10250/pods
MOCK_PODS_JSON='{
  "kind": "PodList",
  "apiVersion": "v1",
  "items": [
    {
      "metadata": {
        "name": "ds-leafrouter-ussouth-az01-4vvnd",
        "namespace": "sonic",
        "labels": {
          "controller-revision-hash": "5796d79c5f",
          "name": "ds-leafrouter",
          "pod-template-generation": "1743567"
        }
      },
      "status": { "phase": "Running" }
    },
    {
      "metadata": {
        "name": "ds-leafrouter-telemetry-zgmsl",
        "namespace": "sonic",
        "labels": {
          "controller-revision-hash": "fb47dd8b7",
          "name": "ds-leafrouter-telemetry",
          "pod-template-generation": "9",
          "raw_container_name": "telemetry"
        }
      },
      "status": { "phase": "Running" }
    },
    {
      "metadata": {
        "name": "ds-leafrouter-restapi-abc12",
        "namespace": "sonic",
        "labels": {
          "raw_container_name": "restapi"
        }
      },
      "status": { "phase": "Pending" }
    },
    {
      "metadata": {
        "name": "other-ns-pod",
        "namespace": "kube-system",
        "labels": {
          "raw_container_name": "telemetry"
        }
      },
      "status": { "phase": "Running" }
    }
  ]
}'

# Empty pod list
MOCK_EMPTY_JSON='{
  "kind": "PodList",
  "apiVersion": "v1",
  "items": []
}'

# Pod with no labels
MOCK_NO_LABELS_JSON='{
  "kind": "PodList",
  "apiVersion": "v1",
  "items": [
    {
      "metadata": {
        "name": "no-labels-pod",
        "namespace": "sonic"
      },
      "status": { "phase": "Running" }
    }
  ]
}'

# ── extract jq filters from the script to test them directly ─────────
# We test the exact same jq expressions used in pods_on_node / pod_names_on_node

jq_pods_on_node() {
  local ns="$1" key="$2" val="$3" json="$4"
  printf '%s' "$json" | jq -r \
    --arg ns "$ns" --arg key "$key" --arg val "$val" \
    '.items[] | select(.metadata.namespace == $ns) |
     select(.metadata.labels[$key] == $val) |
     "\(.metadata.name) \(.status.phase)"' || true
}

jq_pod_names_on_node() {
  local ns="$1" key="$2" val="$3" json="$4"
  printf '%s' "$json" | jq -r \
    --arg ns "$ns" --arg key "$key" --arg val "$val" \
    '.items[] | select(.metadata.namespace == $ns) |
     select(.metadata.labels[$key] == $val) |
     .metadata.name' || true
}

# ── Test suite ────────────────────────────────────────────────────────
echo "=== pods_on_node jq filter ==="

# Test 1: Match telemetry pod in sonic namespace
result="$(jq_pods_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_PODS_JSON")"
assert_eq "match telemetry pod" \
  "ds-leafrouter-telemetry-zgmsl Running" \
  "$result"

# Test 2: Match restapi pod (Pending phase)
result="$(jq_pods_on_node "sonic" "raw_container_name" "restapi" "$MOCK_PODS_JSON")"
assert_eq "match restapi pod (Pending)" \
  "ds-leafrouter-restapi-abc12 Pending" \
  "$result"

# Test 3: Should NOT match pods in other namespaces
result="$(jq_pods_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_PODS_JSON")"
# Should only return the sonic-namespace pod, not the kube-system one
line_count=$(echo "$result" | wc -l)
assert_eq "excludes other namespaces (single result)" "1" "$line_count"

# Test 4: No match when label value differs
result="$(jq_pods_on_node "sonic" "raw_container_name" "nonexistent" "$MOCK_PODS_JSON")"
assert_eq "no match for unknown service" "" "$result"

# Test 5: No match when label key differs
result="$(jq_pods_on_node "sonic" "app" "telemetry" "$MOCK_PODS_JSON")"
assert_eq "no match for wrong label key" "" "$result"

# Test 6: Empty pod list
result="$(jq_pods_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_EMPTY_JSON")"
assert_eq "empty pod list returns empty" "" "$result"

# Test 7: Pod with no labels (should not error, just no match)
result="$(jq_pods_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_NO_LABELS_JSON")"
assert_eq "pod with no labels returns empty" "" "$result"

echo ""
echo "=== pod_names_on_node jq filter ==="

# Test 8: Match telemetry pod name only
result="$(jq_pod_names_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_PODS_JSON")"
assert_eq "match telemetry pod name" \
  "ds-leafrouter-telemetry-zgmsl" \
  "$result"

# Test 9: Match restapi pod name
result="$(jq_pod_names_on_node "sonic" "raw_container_name" "restapi" "$MOCK_PODS_JSON")"
assert_eq "match restapi pod name" \
  "ds-leafrouter-restapi-abc12" \
  "$result"

# Test 10: No match
result="$(jq_pod_names_on_node "sonic" "raw_container_name" "nonexistent" "$MOCK_PODS_JSON")"
assert_eq "no match returns empty" "" "$result"

# Test 11: Empty list
result="$(jq_pod_names_on_node "sonic" "raw_container_name" "telemetry" "$MOCK_EMPTY_JSON")"
assert_eq "empty list returns empty" "" "$result"

echo ""
echo "=== POD_SELECTOR parsing ==="

# Test 12-13: Verify the bash parameter expansion used to split key=value
selector="raw_container_name=telemetry"
key="${selector%%=*}"
val="${selector#*=}"
assert_eq "selector key parsing" "raw_container_name" "$key"
assert_eq "selector val parsing" "telemetry" "$val"

# Test 14-15: Custom selector
selector="app=my-service"
key="${selector%%=*}"
val="${selector#*=}"
assert_eq "custom selector key" "app" "$key"
assert_eq "custom selector val" "my-service" "$val"

echo ""
echo "=== HTTP status code parsing ==="

# Test 16-19: Simulate the -w '%{http_code}' output parsing from kubelet_get_pods
parse_http_response() {
  local out="$1" http_code="" body=""
  if (( ${#out} >= 3 )); then
    http_code="${out: -3}"
    body="${out:0:${#out}-3}"
  fi
  echo "${http_code}|${body}"
}

result="$(parse_http_response '{"items":[]}200')"
assert_eq "parse 200 response code" '200|{"items":[]}' "$result"

result="$(parse_http_response '401')"
assert_eq "parse 401 with empty body" '401|' "$result"

result="$(parse_http_response 'Unauthorized403')"
assert_eq "parse 403 with error body" '403|Unauthorized' "$result"

result="$(parse_http_response '{"items":[{"metadata":{"name":"pod1"}}]}200')"
assert_eq "parse 200 with pod data" '200|{"items":[{"metadata":{"name":"pod1"}}]}' "$result"

echo ""
echo "=== cmd_status awk filter ==="

# Test 20-21: Verify the awk expression used in cmd_status/cmd_wait
has_running() {
  awk '$2=="Running"{found=1} END{exit found?0:1}' <<<"$1" && echo "yes" || echo "no"
}

result="$(has_running "ds-leafrouter-telemetry-zgmsl Running")"
assert_eq "awk detects Running" "yes" "$result"

result="$(has_running "ds-leafrouter-restapi-abc12 Pending")"
assert_eq "awk detects non-Running" "no" "$result"

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

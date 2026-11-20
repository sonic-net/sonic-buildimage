#!/bin/bash
# Switch CPU (x86 host) Reboot Cause Retrieval Script for NextHop BMC
# Queries the x86 host's gNMI server over the internal USB-over-Ethernet
# link (SHOW /reboot-cause, or SHOW /reboot-cause/history with -H), parses
# the JSON_IETF payload, and prints it as plain text.

set -euo pipefail

# The BMC<->x86 USB-over-Ethernet link is an internal, always-present
# interface, so the gNMI container name and the x86 target are fixed.
CONTAINER="gnmi"
TARGET="169.254.100.2:8080"
XPATH="/reboot-cause"
PY_ARGS=()

usage() {
    cat <<EOF
Usage: $(basename "$0") [-H] [-j] [-h]

Query the switch CPU (x86 host) reboot cause via gNMI over the internal
USB-over-Ethernet link and print it as plain text.

  -H  show the reboot-cause history (SHOW /reboot-cause/history)
      instead of the previous reboot cause (SHOW /reboot-cause)
  -j  print the raw JSON payload instead of plain text
  -h  show this help
EOF
}

while getopts "Hjh" opt; do
    case "$opt" in
        H) XPATH="/reboot-cause/history"; PY_ARGS+=(--history) ;;
        j) PY_ARGS+=(--json) ;;
        h) usage; exit 0 ;;
        *) usage >&2; exit 1 ;;
    esac
done

RESPONSE_FILE=$(mktemp)
trap 'rm -f "${RESPONSE_FILE}"' EXIT

if ! docker exec "${CONTAINER}" gnmi_get \
    -target_addr "${TARGET}" \
    -xpath_target SHOW \
    -xpath "${XPATH}" \
    -notls \
    -encoding JSON_IETF >"${RESPONSE_FILE}" 2>&1; then
    echo "Error: gnmi_get failed against ${TARGET} (${XPATH})" >&2
    cat "${RESPONSE_FILE}" >&2
    exit 1
fi

exec python3 "$(dirname "$0")/switch_cpu_reboot_cause.py" \
    --response-file "${RESPONSE_FILE}" \
    "${PY_ARGS[@]}"

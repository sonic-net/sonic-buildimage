#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/telemetry.sh"
TEST_ROOT="$(mktemp -d)"
TEST_SCRIPT="${TEST_ROOT}/telemetry.sh"
STUB_BIN="${TEST_ROOT}/bin"
TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
trap 'rm -rf "${TEST_ROOT}"' EXIT

mkdir -p "${STUB_BIN}"
touch "${TEMPLATE_FILE}"

# shellcheck disable=SC2016
sed \
    -e "s|^TELEMETRY_VARS_FILE=.*$|TELEMETRY_VARS_FILE=${TEMPLATE_FILE}|" \
    -e 's|^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$|printf "telemetry args:%s\\n" "${TELEMETRY_ARGS}"|' \
    "${SCRIPT}" > "${TEST_SCRIPT}"
chmod +x "${TEST_SCRIPT}"

cat > "${STUB_BIN}/sonic-cfggen" <<'EOF'
#!/bin/bash
printf '%s\n' "${TELEMETRY_TEST_CONFIG}"
EOF

chmod +x "${STUB_BIN}/sonic-cfggen"

assert_contains_once() {
    local description="$1"
    local output="$2"
    local expected="$3"
    local count

    count="$(grep -o -- "${expected}" <<< "${output}" | wc -l)"
    if [[ "${count}" != "1" ]]; then
        printf "FAIL: %s: expected one '%s', got %s\n%s\n" \
            "${description}" "${expected}" "${count}" "${output}" >&2
        exit 1
    fi
}

assert_not_contains() {
    local description="$1"
    local output="$2"
    local unexpected="$3"

    if grep -q -- "${unexpected}" <<< "${output}"; then
        printf "FAIL: %s: unexpected '%s'\n%s\n" \
            "${description}" "${unexpected}" "${output}" >&2
        exit 1
    fi
}

run_launcher() {
    TELEMETRY_TEST_CONFIG="$1"
    export TELEMETRY_TEST_CONFIG
    PATH="${STUB_BIN}:${PATH}" TELEMETRY_WATCHDOG_SERIALNUMBER_PROBE_ENABLED=false \
        "${TEST_SCRIPT}" 2>&1
}

config="$(jq -cn '{
    x509: "",
    certs: "",
    gnmi: {
        port: "50051",
        client_auth: "false",
        user_auth: "password",
        vrf: "mgmt"
    }
}')"
output="$(run_launcher "${config}")"
args_output="$(tail -n 1 <<< "${output}")"
assert_contains_once "certificate-free fallback remains plaintext loopback" \
    "${args_output}" "--noTLS --bind_address 127.0.0.1"
assert_not_contains "certificate-free fallback does not bind to management VRF" \
    "${args_output}" "--gnmi_vrf"
assert_contains_once "certificate-free management VRF emits warning" \
    "${output}" "certificate-free fallback is restricted to localhost"

config="$(jq -cn '{
    x509: "",
    certs: "",
    gnmi: {
        port: "50051",
        client_auth: "false",
        user_auth: "password",
        vrf: "default"
    }
}')"
output="$(run_launcher "${config}")"
args_output="$(tail -n 1 <<< "${output}")"
assert_contains_once "certificate-free default VRF remains supported" \
    "${args_output}" "--gnmi_vrf default"
assert_not_contains "certificate-free default VRF emits no warning" \
    "${output}" "certificate-free fallback is restricted to localhost"

config="$(jq -cn '{
    x509: "",
    certs: {
        server_crt: "/server.crt",
        server_key: "/server.key",
        ca_crt: "/ca.crt"
    },
    gnmi: {
        port: "50051",
        client_auth: "false",
        user_auth: "password",
        vrf: "mgmt"
    }
}')"
output="$(run_launcher "${config}")"
args_output="$(tail -n 1 <<< "${output}")"
assert_contains_once "TLS listener preserves management VRF" \
    "${args_output}" "--gnmi_vrf mgmt"
assert_not_contains "TLS listener does not use plaintext fallback" \
    "${args_output}" "--noTLS"

echo "telemetry transport launcher tests passed"

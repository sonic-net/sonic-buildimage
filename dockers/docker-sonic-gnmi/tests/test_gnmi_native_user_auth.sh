#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/gnmi-native.sh"
TEST_ROOT="$(mktemp -d)"
TEST_SCRIPT="${TEST_ROOT}/gnmi-native.sh"
STUB_BIN="${TEST_ROOT}/bin"
TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
trap 'rm -rf "${TEST_ROOT}"' EXIT

mkdir -p "${STUB_BIN}"
touch "${TEMPLATE_FILE}"

# Run the production launcher with only its fixed template and telemetry paths
# redirected to test fixtures. If either production line changes, these checks
# fail instead of silently testing a different implementation.
# shellcheck disable=SC2016
sed \
    -e "s|^TELEMETRY_VARS_FILE=.*$|TELEMETRY_VARS_FILE=${TEMPLATE_FILE}|" \
    -e "s|/var/run/gnmi|${TEST_ROOT}/run/gnmi|g" \
    -e 's|^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$|printf "telemetry args:%s\\n" "${TELEMETRY_ARGS}"|' \
    "${SCRIPT}" > "${TEST_SCRIPT}"
chmod +x "${TEST_SCRIPT}"

cat > "${STUB_BIN}/sonic-cfggen" <<'EOF'
#!/bin/bash
printf '%s\n' "${GNMI_TEST_CONFIG}"
EOF

cat > "${STUB_BIN}/sonic-db-cli" <<'EOF'
#!/bin/bash
if [[ "${3:-}" == "DEVICE_METADATA|localhost" && "${4:-}" == "type" ]]; then
    printf '%s\n' "${GNMI_TEST_DEVICE_TYPE:-}"
fi
exit 0
EOF

chmod +x "${STUB_BIN}/sonic-cfggen" "${STUB_BIN}/sonic-db-cli"

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
    if (( $# == 0 )); then
        GNMI_TEST_CONFIG="$(jq -cn '{
            certs: {
                server_crt: "/server.crt",
                server_key: "/server.key",
                ca_crt: "/ca.crt"
            },
            gnmi: {port: "50052", client_auth: "true"}
        }')"
    else
        GNMI_TEST_CONFIG="$(jq -cn --arg user_auth "$1" '{
            certs: {
                server_crt: "/server.crt",
                server_key: "/server.key",
                ca_crt: "/ca.crt"
            },
            gnmi: {
                port: "50052",
                client_auth: "true",
                user_auth: $user_auth
            }
        }')"
    fi
    export GNMI_TEST_CONFIG
    GNMI_TEST_DEVICE_TYPE="" PATH="${STUB_BIN}:${PATH}" "${TEST_SCRIPT}" | tail -n 1
}

run_no_tls_launcher() {
    if (( $# == 0 )); then
        GNMI_TEST_CONFIG="$(jq -cn '{
            certs: null,
            x509: null,
            gnmi: {port: "50052", client_auth: "false"}
        }')"
    else
        GNMI_TEST_CONFIG="$(jq -cn --arg user_auth "$1" '{
            certs: null,
            x509: null,
            gnmi: {
                port: "50052",
                client_auth: "false",
                user_auth: $user_auth
            }
        }')"
    fi
    export GNMI_TEST_CONFIG
    GNMI_TEST_DEVICE_TYPE="" PATH="${STUB_BIN}:${PATH}" "${TEST_SCRIPT}" | tail -n 1
}

run_dpu_launcher() {
    GNMI_TEST_CONFIG="$(jq -cn '{
        certs: null,
        x509: null,
        gnmi: {port: "50052", client_auth: "false"}
    }')"
    export GNMI_TEST_CONFIG
    GNMI_TEST_DEVICE_TYPE="SmartSwitchDPU" PATH="${STUB_BIN}:${PATH}" \
        "${TEST_SCRIPT}" | tail -n 1
}

run_uds_only_launcher() {
    GNMI_TEST_CONFIG="$(jq -cn '{
        certs: null,
        x509: null,
        gnmi: {port: "50052", client_auth: "false"}
    }')"
    export GNMI_TEST_CONFIG
    GNMI_LISTENER_MODE="uds-only" GNMI_TEST_DEVICE_TYPE="" \
        PATH="${STUB_BIN}:${PATH}" "${TEST_SCRIPT}" | tail -n 1
}

output="$(run_launcher)"
assert_contains_once "missing user_auth uses the secure default" \
    "${output}" "--client_auth cert"
assert_contains_once "missing user_auth configures certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_launcher '')"
assert_contains_once "empty user_auth uses the secure default" \
    "${output}" "--client_auth cert"

output="$(run_launcher 'none')"
assert_contains_once "explicit none is forwarded" \
    "${output}" "--client_auth none"
assert_not_contains "explicit none does not configure certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_launcher 'cert')"
assert_contains_once "explicit cert is forwarded" \
    "${output}" "--client_auth cert"
assert_contains_once "explicit cert configures certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_launcher 'password')"
assert_contains_once "explicit password is forwarded" \
    "${output}" "--client_auth password"
assert_not_contains "explicit password does not configure certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_no_tls_launcher)"
assert_contains_once "missing noTLS user_auth uses application authentication" \
    "${output}" "--client_auth password,jwt"
assert_not_contains "missing noTLS user_auth does not request certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_no_tls_launcher '')"
assert_contains_once "empty noTLS user_auth uses application authentication" \
    "${output}" "--client_auth password,jwt"

output="$(run_no_tls_launcher 'cert')"
assert_contains_once "explicit noTLS certificate mode is forwarded for fail-closed startup" \
    "${output}" "--client_auth cert"

output="$(run_uds_only_launcher)"
assert_contains_once "UDS-only disables the TCP listener" \
    "${output}" "--port 0"
assert_contains_once "UDS-only retains its existing authentication default" \
    "${output}" "--client_auth cert"
assert_not_contains "UDS-only does not enable password or JWT authentication by default" \
    "${output}" "--client_auth password,jwt"

output="$(run_dpu_launcher)"
assert_contains_once "DPU without certificates uses ephemeral TLS" \
    "${output}" "--insecure"
assert_contains_once "DPU ephemeral TLS accepts a proxy without a client certificate" \
    "${output}" "--allow_no_client_auth"
assert_contains_once "DPU TLS retains its certificate default" \
    "${output}" "--client_auth cert"
assert_not_contains "DPU compatibility mode does not require proxy password or JWT" \
    "${output}" "--client_auth password,jwt"

echo "gnmi-native user_auth launcher tests passed"

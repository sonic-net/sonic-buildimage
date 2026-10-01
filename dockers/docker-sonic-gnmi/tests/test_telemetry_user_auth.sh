#!/bin/bash

set -euo pipefail

DOCKERS_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT="${DOCKERS_DIR}/docker-sonic-telemetry/telemetry.sh"
TEST_ROOT="$(mktemp -d)"
TEST_SCRIPT="${TEST_ROOT}/telemetry.sh"
STUB_BIN="${TEST_ROOT}/bin"
TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
trap 'rm -rf "${TEST_ROOT}"' EXIT

mkdir -p "${STUB_BIN}"
touch "${TEMPLATE_FILE}"

sed \
    -e "s|^TELEMETRY_VARS_FILE=.*$|TELEMETRY_VARS_FILE=${TEMPLATE_FILE}|" \
    -e 's|^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$|printf "telemetry args:%s\\n" "${TELEMETRY_ARGS}"|' \
    "${SCRIPT}" > "${TEST_SCRIPT}"
chmod +x "${TEST_SCRIPT}"

cat > "${STUB_BIN}/sonic-cfggen" <<'EOF'
#!/bin/bash
printf '%s\n' "${GNMI_TEST_CONFIG}"
EOF

cat > "${STUB_BIN}/sonic-db-cli" <<'EOF'
#!/bin/bash
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
    GNMI_TEST_CONFIG="$1"
    export GNMI_TEST_CONFIG
    PATH="${STUB_BIN}:${PATH}" TELEMETRY_WATCHDOG_SERIALNUMBER_PROBE_ENABLED=false \
        "${TEST_SCRIPT}" | tail -n 1
}

output="$(run_launcher "$(jq -cn '{
    certs: "",
    x509: "",
    gnmi: {port: "8080", client_auth: "false"}
}')")"
assert_contains_once "missing noTLS user_auth uses application authentication" \
    "${output}" "--client_auth password,jwt"
assert_not_contains "missing noTLS user_auth does not request certificate lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

output="$(run_launcher "$(jq -cn '{
    certs: {
        server_crt: "/server.crt",
        server_key: "/server.key",
        ca_crt: "/ca.crt"
    },
    gnmi: {port: "8080", client_auth: "true"}
}')")"
assert_contains_once "missing TLS user_auth retains certificate authentication" \
    "${output}" "--client_auth cert"
assert_contains_once "certificate authentication configures role lookup" \
    "${output}" "--config_table_name GNMI_CLIENT_CERT"

echo "telemetry user_auth launcher tests passed"

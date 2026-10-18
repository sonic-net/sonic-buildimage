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
    if (( $# == 0 )); then
        GNMI_TEST_CONFIG="$(jq -cn '{
            certs: {
                server_crt: "/server.crt",
                server_key: "/server.key",
                ca_crt: "/ca.crt"
            },
            gnmi: {port: "50052", client_auth: "false"}
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
                client_auth: "false",
                user_auth: $user_auth
            }
        }')"
    fi
    export GNMI_TEST_CONFIG
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

echo "gnmi-native user_auth launcher tests passed"

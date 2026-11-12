#!/usr/bin/env bats

setup() {
    SCRIPT_DIR="$(cd "${BATS_TEST_DIRNAME}/.." && pwd)"
    SCRIPT="${SCRIPT_DIR}/gnmi-native.sh"
    TEST_ROOT="${BATS_TEST_TMPDIR}"
    TEST_SCRIPT="${TEST_ROOT}/gnmi-native.sh"
    STUB_BIN="${TEST_ROOT}/bin"
    TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
    export GNMI_LISTENER_MODE=config

    mkdir -p "${STUB_BIN}"
    touch "${TEMPLATE_FILE}"

    # Run the production launcher with only its fixed template and telemetry paths
    # redirected to test fixtures. If either production line changes, these checks
    # fail instead of silently testing a different implementation.
    [[ "$(grep -c '^TELEMETRY_VARS_FILE=' "${SCRIPT}")" == 1 ]]
    # shellcheck disable=SC2016
    [[ "$(grep -c '^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$' "${SCRIPT}")" == 1 ]]
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
}

assert_contains_once() {
    local description="$1"
    local output="$2"
    local expected="$3"
    local count

    count="$(grep -o -- "${expected}" <<< "${output}" | wc -l)"
    if [[ "${count}" != "1" ]]; then
        printf "FAIL: %s: expected one '%s', got %s\n%s\n" \
            "${description}" "${expected}" "${count}" "${output}" >&2
        return 1
    fi
}

assert_not_contains() {
    local description="$1"
    local output="$2"
    local unexpected="$3"

    if grep -q -- "${unexpected}" <<< "${output}"; then
        printf "FAIL: %s: unexpected '%s'\n%s\n" \
            "${description}" "${unexpected}" "${output}" >&2
        return 1
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
    run_launcher_config "${GNMI_TEST_CONFIG}"
    output="${lines[${#lines[@]}-1]}"
}

run_launcher_config() {
    export GNMI_TEST_CONFIG="$1"
    run env PATH="${STUB_BIN}:${PATH}" "${TEST_SCRIPT}"
    [[ "${status}" == 0 ]]
}

@test "missing user_auth uses the secure default and certificate lookup" {
    run_launcher
    assert_contains_once "missing user_auth uses the secure default" \
        "${output}" "--client_auth cert"
    assert_contains_once "missing user_auth configures certificate lookup" \
        "${output}" "--config_table_name GNMI_CLIENT_CERT"
}

@test "empty user_auth uses the secure default" {
    run_launcher ''
    assert_contains_once "empty user_auth uses the secure default" \
        "${output}" "--client_auth cert"
}

@test "explicit none is forwarded without certificate lookup" {
    run_launcher 'none'
    assert_contains_once "explicit none is forwarded" \
        "${output}" "--client_auth none"
    assert_not_contains "explicit none does not configure certificate lookup" \
        "${output}" "--config_table_name GNMI_CLIENT_CERT"
}

@test "explicit cert is forwarded with certificate lookup" {
    run_launcher 'cert'
    assert_contains_once "explicit cert is forwarded" \
        "${output}" "--client_auth cert"
    assert_contains_once "explicit cert configures certificate lookup" \
        "${output}" "--config_table_name GNMI_CLIENT_CERT"
}

@test "explicit password is forwarded without certificate lookup" {
    run_launcher 'password'
    assert_contains_once "explicit password is forwarded" \
        "${output}" "--client_auth password"
    assert_not_contains "explicit password does not configure certificate lookup" \
        "${output}" "--config_table_name GNMI_CLIENT_CERT"
}

@test "certificate-free management VRF remains on loopback and emits a warning" {
    config="$(jq -cn '{
        gnmi: {port: "50052", client_auth: "false", user_auth: "password", vrf: "mgmt"}
    }')"
    run_launcher_config "${config}"
    args_output="${lines[${#lines[@]}-1]}"
    assert_contains_once "certificate-free fallback remains plaintext loopback" \
        "${args_output}" "--noTLS --bind_address 127.0.0.1"
    assert_not_contains "certificate-free fallback does not bind to management VRF" \
        "${args_output}" "--gnmi_vrf"
    assert_contains_once "certificate-free management VRF emits warning" \
        "${output}" "certificate-free fallback is restricted to localhost"
}

@test "certificate-free default VRF remains supported without a warning" {
    config="$(jq -cn '{
        gnmi: {port: "50052", client_auth: "false", user_auth: "password", vrf: "default"}
    }')"
    run_launcher_config "${config}"
    args_output="${lines[${#lines[@]}-1]}"
    assert_contains_once "certificate-free default VRF remains supported" \
        "${args_output}" "--gnmi_vrf default"
    assert_not_contains "certificate-free default VRF emits no warning" \
        "${output}" "certificate-free fallback is restricted to localhost"
}

@test "configured TLS preserves management VRF without plaintext fallback" {
    config="$(jq -cn '{
        certs: {server_crt: "/server.crt", server_key: "/server.key", ca_crt: "/ca.crt"},
        gnmi: {port: "50052", client_auth: "false", user_auth: "password", vrf: "mgmt"}
    }')"
    run_launcher_config "${config}"
    args_output="${lines[${#lines[@]}-1]}"
    assert_contains_once "TLS listener preserves management VRF" \
        "${args_output}" "--gnmi_vrf mgmt"
    assert_not_contains "TLS listener does not use plaintext fallback" \
        "${args_output}" "--noTLS"
}

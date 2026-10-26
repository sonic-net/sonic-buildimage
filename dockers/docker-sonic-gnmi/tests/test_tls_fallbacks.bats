#!/usr/bin/env bats

setup() {
    bats_require_minimum_version 1.10.0
    DOCKERS_DIR="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
    TEST_ROOT="${BATS_TEST_TMPDIR}"
    STUB_BIN="${TEST_ROOT}/bin"
    TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
    export GNMI_LISTENER_MODE=config

    mkdir -p "${STUB_BIN}"
    touch "${TEMPLATE_FILE}"

    cat > "${STUB_BIN}/sonic-cfggen" <<'EOF'
#!/bin/bash
printf '%s\n' "${F050_CONFIG}"
EOF

    cat > "${STUB_BIN}/sonic-db-cli" <<'EOF'
#!/bin/bash
case "${*: -1}" in
    type) printf '%s\n' "${F050_DEVICE_TYPE-}" ;;
    switch_type) printf '%s\n' "${F050_SWITCH_TYPE-}" ;;
esac
EOF

    cat > "${STUB_BIN}/telemetry" <<'EOF'
#!/bin/bash
printf '%s\0' "$@" > "${F050_ARGS_FILE}"
touch "${F050_INVOKED_FILE}"
EOF

    chmod +x "${STUB_BIN}/sonic-cfggen" "${STUB_BIN}/sonic-db-cli" "${STUB_BIN}/telemetry"

    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        script="${DOCKERS_DIR}/${image}/$([[ "${image}" == docker-sonic-gnmi ]] && printf gnmi-native.sh || printf telemetry.sh)"
        test_script="${TEST_ROOT}/${image}.sh"

        [[ "$(grep -c '^TELEMETRY_VARS_FILE=' "${script}")" == 1 ]]
        # shellcheck disable=SC2016
        [[ "$(grep -c '^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$' "${script}")" == 1 ]]
        # shellcheck disable=SC2016
        sed \
            -e "s|^TELEMETRY_VARS_FILE=.*$|TELEMETRY_VARS_FILE=${TEMPLATE_FILE}|" \
            -e 's|^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$|exec "${F050_TEST_TELEMETRY}" ${TELEMETRY_ARGS}|' \
            "${script}" > "${test_script}"
        chmod +x "${test_script}"
    done
    setup_configs
}

run_launcher() {
    local image=$1
    local config=$2
    local device_type=${3-}
    local switch_type=${4-}
    local test_script="${TEST_ROOT}/${image}.sh"

    F050_ARGS_FILE="${TEST_ROOT}/${image}.args"
    F050_INVOKED_FILE="${TEST_ROOT}/${image}.invoked"
    rm -f "${F050_ARGS_FILE}" "${F050_INVOKED_FILE}"

    export F050_CONFIG="${config}"
    export F050_DEVICE_TYPE="${device_type}"
    export F050_SWITCH_TYPE="${switch_type}"
    export F050_ARGS_FILE F050_INVOKED_FILE
    export F050_TEST_TELEMETRY="${STUB_BIN}/telemetry"

    run --separate-stderr env TELEMETRY_WATCHDOG_SERIALNUMBER_PROBE_ENABLED=false \
        PATH="${STUB_BIN}:${PATH}" "${test_script}"
}

load_args() {
    mapfile -d '' -t RUN_ARGS < "${F050_ARGS_FILE}"
}

assert_has_arg() {
    local expected=$1
    local arg
    for arg in "${RUN_ARGS[@]}"; do
        [[ "${arg}" != "${expected}" ]] || return 0
    done
    printf "FAIL: missing argument '%s': %s\n" "${expected}" "${RUN_ARGS[*]}" >&2
    return 1
}

assert_no_arg() {
    local unexpected=$1
    local arg
    for arg in "${RUN_ARGS[@]}"; do
        if [[ "${arg}" == "${unexpected}" ]]; then
            printf "FAIL: unexpected argument '%s': %s\n" "${unexpected}" "${RUN_ARGS[*]}" >&2
            return 1
        fi
    done
}

assert_started() {
    [[ "${status}" == 0 ]]
    [[ -f "${F050_INVOKED_FILE}" ]]
    load_args
}

assert_failed_closed() {
    [[ "${status}" != 0 ]]
    [[ ! -e "${F050_INVOKED_FILE}" ]]
    # Provided by Bats run --separate-stderr.
    # shellcheck disable=SC2154
    [[ "${stderr}" == *"requires both server_crt and server_key"* ]]
}

setup_configs() {
    valid_certs="$(jq -cn '{
        certs: {server_crt: "/server.crt", server_key: "/server.key", ca_crt: "/ca.crt"},
        gnmi: {port: "50052", client_auth: "true"}
    }')"
    valid_x509="$(jq -cn '{
        certs: "",
        x509: {server_crt: "/legacy.crt", server_key: "/legacy.key", ca_crt: "/legacy-ca.crt"},
        gnmi: {port: "50052", client_auth: "true"}
    }')"
    partial_certs="$(jq -cn '{
        certs: {server_crt: "/server.crt"},
        gnmi: {port: "50052", client_auth: "true"}
    }')"
    partial_x509="$(jq -cn '{
        certs: "",
        x509: {server_key: "/legacy.key"},
        gnmi: {port: "50052", client_auth: "true"}
    }')"
    empty_certs="$(jq -c '.certs = {
        server_crt: "", server_key: "", ca_crt: ""
    }' <<< "${valid_certs}")"
    empty_x509="$(jq -c '.x509 = {
        server_crt: "", server_key: "", ca_crt: ""
    }' <<< "${valid_x509}")"
    no_tls_config="$(jq -cn '{certs: "", x509: "", gnmi: ""}')"
}

@test "both non-DPU launchers use complete configured certificates" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${valid_certs}"
        assert_started
        assert_has_arg --server_crt
        assert_has_arg /server.crt
        assert_has_arg --server_key
        assert_has_arg /server.key
        assert_no_arg --insecure
        assert_no_arg --noTLS
    done
}

@test "both non-DPU launchers use complete legacy x509 credentials" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${valid_x509}"
        assert_started
        assert_has_arg /legacy.crt
        assert_has_arg /legacy.key
        assert_no_arg --insecure
    done
}

@test "both non-DPU launchers reject partial configured certificates" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${partial_certs}"
        assert_failed_closed
    done
}

@test "both non-DPU launchers reject partial legacy x509 credentials" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${partial_x509}"
        assert_failed_closed
    done
}

@test "both non-DPU launchers reject empty configured certificates" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${empty_certs}"
        assert_failed_closed
    done
}

@test "both non-DPU launchers reject empty legacy x509 credentials" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${empty_x509}"
        assert_failed_closed
    done
}

@test "both non-DPU launchers keep certificate-free fallback on loopback" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        run_launcher "${image}" "${no_tls_config}"
        assert_started
        assert_has_arg --noTLS
        assert_has_arg --bind_address
        assert_has_arg 127.0.0.1
        assert_no_arg --insecure
    done
}

@test "both DPU metadata forms retain ephemeral TLS for absent or incomplete credentials" {
    for dpu_marker in type switch_type; do
        device_type=SmartSwitchDPU
        switch_type=
        if [[ "${dpu_marker}" == switch_type ]]; then
            device_type=
            switch_type=dpu
        fi

        for config in "${no_tls_config}" "${partial_certs}" "${partial_x509}" \
            "${empty_certs}" "${empty_x509}"; do
            run_launcher docker-sonic-gnmi "${config}" "${device_type}" "${switch_type}"
            assert_started
            assert_has_arg --insecure
            assert_has_arg --allow_no_client_auth
            assert_no_arg --noTLS
            assert_no_arg --server_crt
            assert_no_arg --server_key
        done
    done
}

@test "both DPU metadata forms honor configured TLS and explicit optional client auth" {
    for dpu_marker in type switch_type; do
        device_type=SmartSwitchDPU
        switch_type=
        if [[ "${dpu_marker}" == switch_type ]]; then
            device_type=
            switch_type=dpu
        fi
        for config in "${valid_certs}" "${valid_x509}"; do
            run_launcher docker-sonic-gnmi "${config}" "${device_type}" "${switch_type}"
            assert_started
            assert_has_arg --server_crt
            assert_has_arg --server_key
            assert_no_arg --insecure
            assert_no_arg --noTLS
            assert_no_arg --allow_no_client_auth

            optional_client_auth="$(jq -c '.gnmi.client_auth = "false"' <<< "${config}")"
            run_launcher docker-sonic-gnmi "${optional_client_auth}" "${device_type}" "${switch_type}"
            assert_started
            assert_has_arg --allow_no_client_auth
            assert_has_arg --server_crt
            assert_has_arg --server_key
            assert_no_arg --insecure
            assert_no_arg --noTLS
        done
    done
}

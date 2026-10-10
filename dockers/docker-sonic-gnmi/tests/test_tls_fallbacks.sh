#!/bin/bash

set -euo pipefail

DOCKERS_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
TEST_ROOT="$(mktemp -d)"
STUB_BIN="${TEST_ROOT}/bin"
TEMPLATE_FILE="${TEST_ROOT}/telemetry_vars.j2"
trap 'rm -rf "${TEST_ROOT}"' EXIT

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
    [[ "$(grep -c '^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$' "${script}")" == 1 ]]
    sed \
        -e "s|^TELEMETRY_VARS_FILE=.*$|TELEMETRY_VARS_FILE=${TEMPLATE_FILE}|" \
        -e 's|^exec /usr/sbin/telemetry ${TELEMETRY_ARGS}$|exec "${F050_TEST_TELEMETRY}" ${TELEMETRY_ARGS}|' \
        "${script}" > "${test_script}"
    chmod +x "${test_script}"
done

run_launcher() {
    local image=$1
    local config=$2
    local device_type=${3-}
    local switch_type=${4-}
    local test_script="${TEST_ROOT}/${image}.sh"

    F050_ARGS_FILE="${TEST_ROOT}/${image}.args"
    F050_INVOKED_FILE="${TEST_ROOT}/${image}.invoked"
    F050_STDOUT_FILE="${TEST_ROOT}/${image}.stdout"
    F050_STDERR_FILE="${TEST_ROOT}/${image}.stderr"
    rm -f "${F050_ARGS_FILE}" "${F050_INVOKED_FILE}" "${F050_STDOUT_FILE}" "${F050_STDERR_FILE}"

    export F050_CONFIG="${config}"
    export F050_DEVICE_TYPE="${device_type}"
    export F050_SWITCH_TYPE="${switch_type}"
    export F050_ARGS_FILE F050_INVOKED_FILE
    export F050_TEST_TELEMETRY="${STUB_BIN}/telemetry"

    RUN_STATUS=0
    TELEMETRY_WATCHDOG_SERIALNUMBER_PROBE_ENABLED=false \
        PATH="${STUB_BIN}:${PATH}" "${test_script}" \
        > "${F050_STDOUT_FILE}" 2> "${F050_STDERR_FILE}" || RUN_STATUS=$?
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
    exit 1
}

assert_no_arg() {
    local unexpected=$1
    local arg
    for arg in "${RUN_ARGS[@]}"; do
        if [[ "${arg}" == "${unexpected}" ]]; then
            printf "FAIL: unexpected argument '%s': %s\n" "${unexpected}" "${RUN_ARGS[*]}" >&2
            exit 1
        fi
    done
}

assert_started() {
    [[ "${RUN_STATUS}" == 0 ]]
    [[ -f "${F050_INVOKED_FILE}" ]]
    load_args
}

assert_failed_closed() {
    [[ "${RUN_STATUS}" != 0 ]]
    [[ ! -e "${F050_INVOKED_FILE}" ]]
    grep -q 'requires both server_crt and server_key' "${F050_STDERR_FILE}"
}

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
no_tls_config="$(jq -cn '{certs: "", x509: "", gnmi: ""}')"

for image in docker-sonic-gnmi docker-sonic-telemetry; do
    run_launcher "${image}" "${valid_certs}"
    assert_started
    assert_has_arg --server_crt
    assert_has_arg /server.crt
    assert_has_arg --server_key
    assert_has_arg /server.key
    assert_no_arg --insecure
    assert_no_arg --noTLS

    run_launcher "${image}" "${valid_x509}"
    assert_started
    assert_has_arg /legacy.crt
    assert_has_arg /legacy.key
    assert_no_arg --insecure

    run_launcher "${image}" "${partial_certs}"
    assert_failed_closed

    run_launcher "${image}" "${partial_x509}"
    assert_failed_closed

    run_launcher "${image}" "${no_tls_config}"
    assert_started
    assert_has_arg --noTLS
    assert_has_arg --bind_address
    assert_has_arg 127.0.0.1
    assert_no_arg --insecure
done

run_launcher docker-sonic-gnmi "${no_tls_config}" SmartSwitchDPU
[[ "${RUN_STATUS}" != 0 ]]
[[ ! -e "${F050_INVOKED_FILE}" ]]
grep -q 'SmartSwitch DPU requires configured server_crt and server_key' "${F050_STDERR_FILE}"

run_launcher docker-sonic-gnmi "${valid_certs}" SmartSwitchDPU
assert_started
assert_no_arg --insecure
assert_no_arg --allow_no_client_auth

optional_client_auth="$(jq -c '.gnmi.client_auth = "false"' <<< "${valid_certs}")"
run_launcher docker-sonic-gnmi "${optional_client_auth}" SmartSwitchDPU
assert_started
assert_has_arg --allow_no_client_auth
assert_has_arg /server.crt
assert_has_arg /server.key
assert_no_arg --insecure
assert_no_arg --noTLS

echo "native gNMI and legacy telemetry TLS fallback tests passed"

#!/usr/bin/env bats

setup() {
    DOCKERS_DIR="$(cd "${BATS_TEST_DIRNAME}/../.." && pwd)"
    TEST_ROOT="${BATS_TEST_TMPDIR}"

    export DIALOUT_TEST_CLIENT="${TEST_ROOT}/dialout client"
    export DIALOUT_TEST_OUTPUT="${TEST_ROOT}/output"

    cat > "${DIALOUT_TEST_CLIENT}" <<'EOF'
#!/bin/bash
set -euo pipefail
printf '%s\0' "$@" > "${DIALOUT_TEST_OUTPUT}.args"
printf '%s\0' "${CVL_SCHEMA_PATH-}" "${SSL_CERT_FILE-unset}" "${SSL_CERT_DIR-unset}" \
    > "${DIALOUT_TEST_OUTPUT}.env"
exit "${DIALOUT_TEST_EXIT:-0}"
EOF
    chmod +x "${DIALOUT_TEST_CLIENT}"
}

prepare_launcher() {
    local image=$1
    local script="${DOCKERS_DIR}/${image}/dialout.sh"
    test_script="${TEST_ROOT}/${image}.sh"

    # Redirect only the fixed executable; fail if the production invocation drifts.
    [[ "$(grep -c '^exec /usr/sbin/dialout_client_cli ' "${script}")" == 1 ]]
    # shellcheck disable=SC2016
    sed 's|^exec /usr/sbin/dialout_client_cli |exec "${DIALOUT_TEST_CLIENT}" |' \
        "${script}" > "${test_script}"
}

@test "both dial-out launchers keep TLS defaults and ignore caller overrides" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        prepare_launcher "${image}"
        run env -u SSL_CERT_FILE -u SSL_CERT_DIR -u DIALOUT_TEST_EXIT \
            CVL_SCHEMA_PATH=unexpected bash "${test_script}" --insecure
        [[ "${status}" == 0 ]]
        mapfile -d '' -t args < "${DIALOUT_TEST_OUTPUT}.args"
        mapfile -d '' -t env_values < "${DIALOUT_TEST_OUTPUT}.env"
        [[ "${#args[@]}" == 3 && "${args[0]}" == -logtostderr && "${args[1]}" == -v && "${args[2]}" == 2 ]]
        [[ "${env_values[0]}" == /usr/sbin/schema ]]
        [[ "${env_values[1]}" == unset && "${env_values[2]}" == unset ]]
    done
}

@test "both dial-out launchers preserve custom trust paths including spaces" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        prepare_launcher "${image}"
        run env -u DIALOUT_TEST_EXIT CVL_SCHEMA_PATH=unexpected \
            SSL_CERT_FILE="${TEST_ROOT}/private CA.pem" \
            SSL_CERT_DIR="${TEST_ROOT}/CA directory" bash "${test_script}" --insecure
        [[ "${status}" == 0 ]]
        mapfile -d '' -t args < "${DIALOUT_TEST_OUTPUT}.args"
        mapfile -d '' -t env_values < "${DIALOUT_TEST_OUTPUT}.env"
        [[ "${#args[@]}" == 3 && "${args[0]}" == -logtostderr && "${args[1]}" == -v && "${args[2]}" == 2 ]]
        [[ "${env_values[0]}" == /usr/sbin/schema ]]
        [[ "${env_values[1]}" == "${TEST_ROOT}/private CA.pem" ]]
        [[ "${env_values[2]}" == "${TEST_ROOT}/CA directory" ]]
    done
}

@test "both dial-out launchers propagate the client exit status" {
    for image in docker-sonic-gnmi docker-sonic-telemetry; do
        prepare_launcher "${image}"
        run env DIALOUT_TEST_EXIT=37 bash "${test_script}"
        [[ "${status}" == 37 ]]
    done
}

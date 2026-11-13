#!/bin/bash

set -euo pipefail
trap 'echo "FAIL: ${image:-setup}: line ${LINENO}" >&2' ERR

DOCKERS_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
TEST_ROOT="$(mktemp -d)"
trap 'rm -rf "${TEST_ROOT}"' EXIT

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

for image in docker-sonic-gnmi docker-sonic-telemetry; do
    script="${DOCKERS_DIR}/${image}/dialout.sh"
    test_script="${TEST_ROOT}/${image}.sh"

    # Redirect only the fixed executable; fail if the production invocation drifts.
    [[ "$(grep -c '^exec /usr/sbin/dialout_client_cli ' "${script}")" == 1 ]]
    sed 's|^exec /usr/sbin/dialout_client_cli |exec "${DIALOUT_TEST_CLIENT}" |' \
        "${script}" > "${test_script}"

    for trust in default custom; do
        (
            unset SSL_CERT_FILE SSL_CERT_DIR
            export CVL_SCHEMA_PATH=unexpected
            if [[ "${trust}" == custom ]]; then
                export SSL_CERT_FILE="${TEST_ROOT}/private CA.pem"
                export SSL_CERT_DIR="${TEST_ROOT}/CA directory"
            fi
            # Caller arguments must not become an implicit production override.
            bash "${test_script}" --insecure
        )
        mapfile -d '' -t args < "${DIALOUT_TEST_OUTPUT}.args"
        mapfile -d '' -t env_values < "${DIALOUT_TEST_OUTPUT}.env"
        [[ "${#args[@]}" == 3 && "${args[0]}" == -logtostderr && "${args[1]}" == -v && "${args[2]}" == 2 ]]
        [[ "${env_values[0]}" == /usr/sbin/schema ]]
        if [[ "${trust}" == custom ]]; then
            [[ "${env_values[1]}" == "${TEST_ROOT}/private CA.pem" ]]
            [[ "${env_values[2]}" == "${TEST_ROOT}/CA directory" ]]
        else
            [[ "${env_values[1]}" == unset && "${env_values[2]}" == unset ]]
        fi
    done

    status=0
    DIALOUT_TEST_EXIT=37 bash "${test_script}" || status=$?
    [[ "${status}" == 37 ]]
    echo "${image}: verified TLS defaults, trust environment, and exec status"
done

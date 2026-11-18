#!/bin/bash

set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
. "${SCRIPT_DIR}/config-setup"

fail()
{
    echo "FAIL: $*" >&2
    exit 1
}

assert_status()
{
    [ "$2" -eq "$1" ] || fail "expected status $1, got $2"
}

assert_file_exists()
{
    [ -e "$1" ] || fail "expected $1 to exist"
}

assert_file_absent()
{
    [ ! -e "$1" ] || fail "expected $1 to be absent"
}

assert_file_executable()
{
    [ -x "$1" ] || fail "expected $1 to be executable"
}

assert_log_contains()
{
    grep -Fqx -- "$1" "${CALL_LOG}" || fail "missing log entry: $1"
}

assert_log_excludes()
{
    if grep -Fq -- "$1" "${CALL_LOG}"; then
        fail "unexpected log entry containing: $1"
    fi
}

assert_log_count()
{
    local actual
    actual=$(grep -Fc -- "$2" "${CALL_LOG}" || true)
    [ "${actual}" -eq "$1" ] || fail "expected $1 log entries containing '$2', got ${actual}"
}

assert_log_before()
{
    local first
    local second
    first=$(grep -nF -- "$1" "${CALL_LOG}" | head -n 1 | cut -d: -f1)
    second=$(grep -nF -- "$2" "${CALL_LOG}" | head -n 1 | cut -d: -f1)
    [ -n "${first}" ] || fail "missing log entry containing: $1"
    [ -n "${second}" ] || fail "missing log entry containing: $2"
    [ "${first}" -lt "${second}" ] ||
        fail "expected '$1' before '$2'"
}

setup_test_env()
{
    TEST_DIR=$(mktemp -d)
    CALL_LOG="${TEST_DIR}/calls.log"
    : > "${CALL_LOG}"
    export CALL_LOG
    LIVE_CONFIG_DB_DIR="${TEST_DIR}/live-config-db"
    mkdir -p "${LIVE_CONFIG_DB_DIR}"
    export LIVE_CONFIG_DB_DIR

    CONFIG_DB_JSON="${TEST_DIR}/config_db.json"
    CONFIG_DB_PATH="${TEST_DIR}/"
    CONFIG_DB_PREFIX=config_db
    CONFIG_DB_SUFFIX=.json
    MINGRAPH_FILE="${TEST_DIR}/minigraph.xml"
    GOLDEN_CONFIG_DB_JSON="${TEST_DIR}/golden_config_db.json"
    GOLDEN_CONFIG_NATIVE_ENABLED_FILE="${TEST_DIR}/golden_config_native_enabled"
    GOLDEN_NATIVE_TRANSITION_FILE="${TEST_DIR}/pending_golden_native_transition"
    GOLDEN_NATIVE_SEALED_DIR="${TEST_DIR}/sealed"
    GOLDEN_NATIVE_SEALED_CONFIG="${GOLDEN_NATIVE_SEALED_DIR}/golden_config_db.json"
    GOLDEN_NATIVE_SEALED_SHA256="${GOLDEN_NATIVE_SEALED_CONFIG}.sha256"
    GOLDEN_NATIVE_PREVIOUS_GOLD_ABSENT="${GOLDEN_CONFIG_DB_JSON}.pre_golden_native.absent"
    INIT_CFG_JSON="${TEST_DIR}/init_cfg.json"
    PENDING_CONFIG_INITIALIZATION="${TEST_DIR}/pending_config_initialization"
    PENDING_CONFIG_MIGRATION="${TEST_DIR}/pending_config_migration"
    CONFIG_SETUP_POST_MIGRATION_FLAG="${TEST_DIR}/pending_post_migration"
    CONFIG_SETUP_INITIALIZATION_FLAG="${TEST_DIR}/pending_initialization"
    CONFIG_POST_MIGRATION_HOOKS="${TEST_DIR}/post-hooks"
    CONFIG_SETUP_CONF="${TEST_DIR}/missing.conf"
    NUM_ASIC=1
    WARM_BOOT=false
    CMD=boot
    PLATFORM=test-platform
    SONIC_DEVICE_PATH="${TEST_DIR}/device"
    OLD_CONFIG_PATH="${TEST_DIR}/old_config"
    mkdir -p "${SONIC_DEVICE_PATH}/${PLATFORM}" "${OLD_CONFIG_PATH}"

    CONFIG_CLI="${TEST_DIR}/config"
    cat > "${CONFIG_CLI}" <<'EOF'
#!/bin/bash
printf '%s\n' "$*" >> "${CALL_LOG}"
status="${CONFIG_CLI_STATUS:-0}"
case " $* " in
    *" recover_golden_native_transactions --boot "*)
        status="${CONFIG_CLI_RECOVERY_STATUS:-${status}}"
        ;;
    *" --check-only "*)
        status="${CONFIG_CLI_CHECK_STATUS:-${status}}"
        if [ "${status}" -eq 0 ] && [ ! -r "${@: -1}" ]; then
            status=1
        fi
        ;;
    *" --migrate-only "*)
        status="${CONFIG_CLI_MIGRATE_STATUS:-${status}}"
        ;;
    *" reload "*)
        status="${CONFIG_CLI_LEGACY_RELOAD_STATUS:-${status}}"
        if [ "${status}" -eq 0 ]; then
            config_file_csv="${@: -1}"
            IFS=',' read -r -a config_files <<< "${config_file_csv}"
            rm -f -- "${LIVE_CONFIG_DB_DIR}"/*.json
            for index in "${!config_files[@]}"; do
                if [ "${index}" -eq 0 ]; then
                    scope=localhost
                else
                    scope="asic$((index - 1))"
                fi
                if [ "${CONFIG_CLI_RELOAD_SKIP_SCOPE:-}" = "${scope}" ]; then
                    continue
                fi
                destination="${LIVE_CONFIG_DB_DIR}/${scope}.json"
                if [ "${CONFIG_CLI_RELOAD_CORRUPT_SCOPE:-}" = "${scope}" ]; then
                    printf '%s\n' '{"unexpected":true}' > "${destination}"
                else
                    cp -- "${config_files[$index]}" "${destination}" || exit 1
                fi
            done
        fi
        ;;
    *" load_golden_config "*)
        status="${CONFIG_CLI_RELOAD_STATUS:-${status}}"
        ;;
esac
exit "${status}"
EOF
    chmod +x "${CONFIG_CLI}"
    export CONFIG_CLI_STATUS=0
    unset CONFIG_CLI_CHECK_STATUS
    unset CONFIG_CLI_MIGRATE_STATUS
    unset CONFIG_CLI_RELOAD_STATUS
    unset CONFIG_CLI_RECOVERY_STATUS
    unset CONFIG_CLI_LEGACY_RELOAD_STATUS
    unset CONFIG_CLI_RELOAD_SKIP_SCOPE
    unset CONFIG_CLI_RELOAD_CORRUPT_SCOPE

    DB_MIGRATOR="${TEST_DIR}/db_migrator.py"
    cat > "${DB_MIGRATOR}" <<'EOF'
#!/bin/bash
printf 'db_migrator %s\n' "$*" >> "${CALL_LOG}"
exit "${DB_MIGRATOR_STATUS:-0}"
EOF
    chmod +x "${DB_MIGRATOR}"
    export DB_MIGRATOR_STATUS=0

    function sonic-db-cli()
    {
        local namespace=""
        local scope=localhost
        local live_file
        printf 'sonic-db-cli %s\n' "$*" >> "${CALL_LOG}"
        if [ "${1:-}" = "-n" ]; then
            namespace="${2:-}"
            scope="${namespace}"
        fi
        if [ -n "${SONIC_DB_CLI_FAIL_NAMESPACE:-}" ] &&
                [ "${namespace}" = "${SONIC_DB_CLI_FAIL_NAMESPACE}" ]; then
            return "${SONIC_DB_CLI_FAIL_STATUS:-1}"
        fi
        if [[ " $* " == *" CONFIG_DB GET CONFIG_DB_INITIALIZED "* ]]; then
            live_file="${LIVE_CONFIG_DB_DIR}/${scope}.json"
            [ -f "${live_file}" ] || return 1
            printf '%s\n' 1
        fi
        return "${SONIC_DB_CLI_STATUS:-0}"
    }
    export SONIC_DB_CLI_STATUS=0
    unset SONIC_DB_CLI_FAIL_NAMESPACE
    unset SONIC_DB_CLI_FAIL_STATUS

    function sonic-cfggen()
    {
        local namespace=""
        local operation=""
        local previous_arg=""
        local read_live=false
        local scope=localhost
        local live_file
        printf 'sonic-cfggen %s\n' "$*" >> "${CALL_LOG}"
        for arg in "$@"; do
            if [ "${previous_arg}" = "-n" ]; then
                namespace="${arg}"
                scope="${namespace}"
            fi
            case "${arg}" in
                -d)
                    read_live=true
                    ;;
                --print-data)
                    operation=preflight
                    ;;
                --write-to-db)
                    operation=write
                    ;;
            esac
            previous_arg="${arg}"
        done
        if [ "${operation}" = "preflight" ] &&
                [ -n "${SONIC_CFGGEN_PREFLIGHT_FAIL_NAMESPACE:-}" ] &&
                [ "${namespace}" = "${SONIC_CFGGEN_PREFLIGHT_FAIL_NAMESPACE}" ]; then
            return "${SONIC_CFGGEN_PREFLIGHT_FAIL_STATUS:-1}"
        fi
        if [ "${operation}" = "write" ] &&
                [ -n "${SONIC_CFGGEN_WRITE_FAIL_NAMESPACE:-}" ] &&
                [ "${namespace}" = "${SONIC_CFGGEN_WRITE_FAIL_NAMESPACE}" ]; then
            return "${SONIC_CFGGEN_WRITE_FAIL_STATUS:-1}"
        fi
        if "${read_live}"; then
            live_file="${LIVE_CONFIG_DB_DIR}/${scope}.json"
            [ -f "${live_file}" ] || return 1
            cat -- "${live_file}"
        fi
        return "${SONIC_CFGGEN_STATUS:-0}"
    }
    export SONIC_CFGGEN_STATUS=0
    unset SONIC_CFGGEN_PREFLIGHT_FAIL_NAMESPACE
    unset SONIC_CFGGEN_PREFLIGHT_FAIL_STATUS
    unset SONIC_CFGGEN_WRITE_FAIL_NAMESPACE
    unset SONIC_CFGGEN_WRITE_FAIL_STATUS

    function ztp_is_enabled()
    {
        printf 'ztp_is_enabled\n' >> "${CALL_LOG}"
        return 1
    }
}

stub_migration_helpers()
{
    function copy_config_files_and_directories()
    {
        printf 'copy %s\n' "$*" >> "${CALL_LOG}"
        for file_dir in "$@"; do
            case "${file_dir}" in
                golden_config_db.json)
                    if [ -f "${OLD_CONFIG_PATH}/${file_dir}" ]; then
                        command cp "${OLD_CONFIG_PATH}/${file_dir}" "${GOLDEN_CONFIG_DB_JSON}"
                    fi
                    ;;
                golden_config_native_enabled)
                    if [ -f "${OLD_CONFIG_PATH}/${file_dir}" ]; then
                        command cp "${OLD_CONFIG_PATH}/${file_dir}" "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
                    fi
                    ;;
            esac
        done
        return 0
    }

    function get_config_db_file_list()
    {
        echo "config_db.json"
    }

    function copy_post_migration_hooks()
    {
        return 0
    }

    function run_hookdir()
    {
        return 0
    }
}

stage_golden_native_migration_source()
{
    local gold_content=${1:-'{}'}

    touch "${OLD_CONFIG_PATH}/golden_config_native_enabled"
    printf '%s\n' "${gold_content}" > "${OLD_CONFIG_PATH}/golden_config_db.json"
}

stage_golden_native_recovery_inputs()
{
    local mode="$1"
    local asic_num=0

    mkdir -p "${GOLDEN_NATIVE_SEALED_DIR}"
    printf '%s\n' '{"gold":true}' > "${GOLDEN_CONFIG_DB_JSON}"
    cp "${GOLDEN_CONFIG_DB_JSON}" "${GOLDEN_NATIVE_SEALED_CONFIG}"
    sha256sum "${GOLDEN_NATIVE_SEALED_CONFIG}" |
        awk '{print $1}' > "${GOLDEN_NATIVE_SEALED_SHA256}"

    printf '%s\n' \
        '{"DEVICE_METADATA":{"localhost":{"hostname":"host-current"}}}' \
        > "${CONFIG_DB_JSON}"
    printf '%s\n' \
        '{"DEVICE_METADATA":{"localhost":{"hostname":"host-backup"}}}' \
        > "${CONFIG_DB_JSON}.pre_golden_native"
    while [[ ${asic_num} -lt ${NUM_ASIC} && ${NUM_ASIC} -gt 1 ]]; do
        printf \
            '{"DEVICE_METADATA":{"localhost":{"asic_name":"asic%s-current"}}}\n' \
            "${asic_num}" \
            > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}${asic_num}${CONFIG_DB_SUFFIX}"
        printf \
            '{"DEVICE_METADATA":{"localhost":{"asic_name":"asic%s-backup"}}}\n' \
            "${asic_num}" \
            > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}${asic_num}${CONFIG_DB_SUFFIX}.pre_golden_native"
        ((asic_num = asic_num + 1))
    done
    printf '%s\n' '<Minigraph>current</Minigraph>' > "${MINGRAPH_FILE}"
    printf '%s\n' '<Minigraph>backup</Minigraph>' > "${MINGRAPH_FILE}.pre_golden_native"
    printf '%s\n' absent > "${GOLDEN_NATIVE_PREVIOUS_GOLD_ABSENT}"
    printf '%s\n' "${mode}" > "${GOLDEN_NATIVE_TRANSITION_FILE}"
}

test_config_setup_entrypoint_is_executable()
(
    assert_file_executable "${SCRIPT_DIR}/config-setup"
)

test_flag_off_preserves_minigraph_path()
(
    setup_test_env
    touch "${MINGRAPH_FILE}" "${PENDING_CONFIG_INITIALIZATION}"

    reload_minigraph()
    {
        printf 'reload_minigraph\n' >> "${CALL_LOG}"
        return 0
    }

    reload_golden_config_native()
    {
        printf 'UNEXPECTED reload_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    do_config_initialization
    assert_status 0 $?
    assert_log_contains "reload_minigraph"
    assert_log_excludes "UNEXPECTED"
    assert_file_absent "${PENDING_CONFIG_INITIALIZATION}"
)

test_flag_on_prefers_complete_gold()
(
    setup_test_env
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${MINGRAPH_FILE}" \
          "${PENDING_CONFIG_INITIALIZATION}"

    reload_minigraph()
    {
        printf 'UNEXPECTED reload_minigraph\n' >> "${CALL_LOG}"
        return 99
    }

    do_config_initialization
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "save -y"
    assert_log_excludes "UNEXPECTED"
    assert_log_excludes "ztp_is_enabled"
    assert_file_absent "${PENDING_CONFIG_INITIALIZATION}"
)

test_committed_initialization_cleanup_warning_clears_pending_marker()
(
    setup_test_env
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_INITIALIZATION}"
    export CONFIG_CLI_RELOAD_STATUS=3

    do_config_initialization
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_file_absent "${PENDING_CONFIG_INITIALIZATION}"
)

test_missing_gold_fails_closed()
(
    setup_test_env
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"

    validate_golden_config_native
    assert_status 1 $?
    assert_log_excludes "load_golden_config"
)

test_db_migration_uses_explicit_gold_source()
(
    setup_test_env
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${GOLDEN_CONFIG_DB_JSON}"

    do_db_migration
    assert_status 0 $?
    assert_log_contains "db_migrator -o migrate --config-source golden --config-source-file ${GOLDEN_CONFIG_DB_JSON}"
)

test_cold_migration_prefers_gold()
(
    setup_test_env
    stub_migration_helpers
    stage_golden_native_migration_source
    touch "${MINGRAPH_FILE}" "${PENDING_CONFIG_MIGRATION}"

    reload_configdb()
    {
        printf 'UNEXPECTED reload_configdb\n' >> "${CALL_LOG}"
        return 99
    }

    reload_minigraph()
    {
        printf 'UNEXPECTED reload_minigraph\n' >> "${CALL_LOG}"
        return 99
    }

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "copy minigraph.xml snmp.yml acl.json port_config.json frr telemetry credentials golden_config_db.json golden_config_native_enabled hft otel_config.yml"
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_log_excludes "db_migrator "
    assert_log_excludes "UNEXPECTED"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_committed_cold_migration_cleanup_warning_clears_pending_marker()
(
    setup_test_env
    stub_migration_helpers
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_RELOAD_STATUS=3

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_flag_off_preserves_minigraph_migration()
(
    setup_test_env
    stub_migration_helpers
    touch "${MINGRAPH_FILE}" "${PENDING_CONFIG_MIGRATION}"

    reload_minigraph()
    {
        printf 'reload_minigraph\n' >> "${CALL_LOG}"
        return 0
    }

    reload_golden_config_native()
    {
        printf 'UNEXPECTED reload_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    do_db_migration()
    {
        printf 'do_db_migration\n' >> "${CALL_LOG}"
        return 0
    }

    do_config_migration
    assert_status 0 $?
    assert_log_contains "reload_minigraph"
    assert_log_contains "do_db_migration"
    assert_log_excludes "UNEXPECTED"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_flag_off_empty_migration_continues_initialization()
(
    setup_test_env
    stub_migration_helpers
    touch "${PENDING_CONFIG_MIGRATION}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    do_config_initialization()
    {
        printf 'do_config_initialization\n' >> "${CALL_LOG}"
        touch "${CONFIG_DB_JSON}"
        return 0
    }

    boot_config
    assert_status 0 $?
    assert_log_contains "do_config_initialization"
    assert_log_count 1 "do_config_initialization"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_failed_gold_migration_preserves_pending_marker()
(
    setup_test_env
    stub_migration_helpers
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_RELOAD_STATUS=8

    do_config_migration
    status=$?
    assert_status 8 "${status}"
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "copy minigraph.xml snmp.yml acl.json port_config.json frr telemetry credentials golden_config_db.json golden_config_native_enabled hft otel_config.yml"
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_support_rejection_precedes_migration_copy()
(
    setup_test_env
    stub_migration_helpers
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_CHECK_STATUS=8

    do_config_migration
    status=$?
    assert_status 8 "${status}"
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_excludes "copy "
    assert_log_count 1 "load_golden_config"
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_missing_old_gold_precedes_migration_copy()
(
    setup_test_env
    stub_migration_helpers
    touch "${OLD_CONFIG_PATH}/golden_config_native_enabled" \
          "${PENDING_CONFIG_MIGRATION}"

    do_config_migration
    status=$?
    assert_status 1 "${status}"
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_excludes "copy "
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_warm_migration_uses_snapshot_held_migrate_only()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"

    do_db_migration()
    {
        printf 'do_db_migration %s\n' "${1:-false}" >> "${CALL_LOG}"
        return 0
    }

    reload_golden_config_native()
    {
        printf 'UNEXPECTED reload_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "load_golden_config -y --migrate-only --persist ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_contains "do_db_migration true"
    assert_log_excludes "UNEXPECTED"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_committed_warm_migration_cleanup_warning_clears_pending_marker()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_MIGRATE_STATUS=3

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --migrate-only --persist ${GOLDEN_CONFIG_DB_JSON}"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_failed_warm_gold_migration_preserves_pending_marker()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    stage_golden_native_migration_source
    touch "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_MIGRATE_STATUS=8

    do_config_migration
    status=$?
    assert_status 8 "${status}"
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "load_golden_config -y --migrate-only --persist ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "db_migrator "
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_legacy_warm_migration_boot_is_idempotent()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    touch "${MINGRAPH_FILE}" \
          "${PENDING_CONFIG_MIGRATION}" \
          "${PENDING_CONFIG_INITIALIZATION}"

    check_system_warm_boot()
    {
        WARM_BOOT=true
        return 0
    }

    set_config_db_initialized()
    {
        printf 'set_config_db_initialized %s\n' "$1" >> "${CALL_LOG}"
        return 0
    }

    do_db_migration()
    {
        printf 'do_db_migration\n' >> "${CALL_LOG}"
        set_config_db_initialized "1"
    }

    boot_config
    assert_status 0 $?
    assert_log_contains "do_db_migration"
    assert_log_count 1 "set_config_db_initialized 1"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
    assert_file_absent "${PENDING_CONFIG_INITIALIZATION}"
)

test_steady_state_boot_does_not_revalidate_gold()
(
    setup_test_env
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${CONFIG_DB_JSON}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    validate_golden_config_native()
    {
        printf 'UNEXPECTED validate_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    reload_golden_config_native()
    {
        printf 'UNEXPECTED reload_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    boot_config
    assert_status 0 $?
    assert_log_excludes "UNEXPECTED"
)

test_multi_asic_initialization_uses_wrapped_gold()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${PENDING_CONFIG_INITIALIZATION}"
    printf '%s\n' '{"localhost":{},"asic0":{},"asic1":{}}' > "${GOLDEN_CONFIG_DB_JSON}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    boot_config
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "save -y"
    assert_log_excludes "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_file_absent "${PENDING_CONFIG_INITIALIZATION}"
)

test_multi_asic_invalid_wrapped_gold_preserves_pending_marker()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${PENDING_CONFIG_INITIALIZATION}"
    printf '%s\n' '{"localhost":{},"asic0":{}}' > "${GOLDEN_CONFIG_DB_JSON}"
    export CONFIG_CLI_STATUS=8

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    boot_config
    assert_status 8 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "save -y"
    assert_file_exists "${PENDING_CONFIG_INITIALIZATION}"
)

test_multi_asic_initialization_failure_preserves_pending_marker()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${PENDING_CONFIG_INITIALIZATION}"
    printf '%s\n' '{"localhost":{},"asic0":{},"asic1":{}}' > "${GOLDEN_CONFIG_DB_JSON}"
    export CONFIG_CLI_RELOAD_STATUS=17

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    boot_config
    assert_status 17 $?
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_file_exists "${PENDING_CONFIG_INITIALIZATION}"
)

test_multi_asic_cold_migration_uses_wrapped_gold()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=2
    stage_golden_native_migration_source '{"localhost":{},"asic0":{},"asic1":{}}'
    touch "${PENDING_CONFIG_MIGRATION}"

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "copy minigraph.xml snmp.yml acl.json port_config.json frr telemetry credentials golden_config_db.json golden_config_native_enabled hft otel_config.yml"
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "save -y"
    assert_log_excludes "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_log_excludes "db_migrator "
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_warm_migration_uses_wrapped_gold()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=2
    WARM_BOOT=true
    stage_golden_native_migration_source '{"localhost":{},"asic0":{},"asic1":{}}'
    touch "${PENDING_CONFIG_MIGRATION}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config --check-only ${OLD_CONFIG_PATH}/golden_config_db.json"
    assert_log_contains "load_golden_config -y --migrate-only --persist --preload-namespace-config asic0 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX} --preload-namespace-config asic1 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX} ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "sonic-cfggen "
    assert_log_excludes "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_log_excludes "db_migrator "
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_warm_migration_rejects_missing_namespace_file()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=2
    WARM_BOOT=true
    stage_golden_native_migration_source '{"localhost":{},"asic0":{},"asic1":{}}'
    touch "${PENDING_CONFIG_MIGRATION}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}"

    do_config_migration
    assert_status 1 $?
    assert_log_excludes "sonic-cfggen "
    assert_log_excludes "load_golden_config -y --migrate-only --persist"
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_warm_migration_utility_failure_writes_nothing()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=3
    WARM_BOOT=true
    stage_golden_native_migration_source \
        '{"localhost":{},"asic0":{},"asic1":{},"asic2":{}}'
    touch "${PENDING_CONFIG_MIGRATION}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX}" \
          "${INIT_CFG_JSON}"
    export CONFIG_CLI_MIGRATE_STATUS=23

    do_config_migration
    assert_status 23 $?
    assert_log_contains "load_golden_config -y --migrate-only --persist --preload-namespace-config asic0 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX} --preload-namespace-config asic1 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX} --preload-namespace-config asic2 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX} ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "sonic-cfggen "
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_warm_migration_passes_only_cold_namespace_inputs()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=3
    WARM_BOOT=true
    stage_golden_native_migration_source \
        '{"localhost":{},"asic0":{},"asic1":{},"asic2":{}}'
    touch "${PENDING_CONFIG_MIGRATION}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}" \
          "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX}"

    check_namespace_warm_fast_boot()
    {
        [ "$1" = "asic1" ]
    }

    do_config_migration
    assert_status 0 $?
    assert_log_contains "load_golden_config -y --migrate-only --persist --preload-namespace-config asic0 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX} --preload-namespace-config asic2 ${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX} ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_excludes "--preload-namespace-config asic1"
    assert_log_excludes "sonic-cfggen "
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_chassis_initialization_fails_closed()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_INITIALIZATION}" \
          "${SONIC_DEVICE_PATH}/${PLATFORM}/chassisdb.conf"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    boot_config
    assert_status 1 $?
    assert_log_excludes "load_golden_config"
    assert_file_exists "${PENDING_CONFIG_INITIALIZATION}"
)

test_multi_asic_chassis_migration_fails_closed()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=2
    stage_golden_native_migration_source '{"localhost":{},"asic0":{},"asic1":{}}'
    touch "${PENDING_CONFIG_MIGRATION}" \
          "${SONIC_DEVICE_PATH}/${PLATFORM}/chassisdb.conf"

    do_config_migration
    assert_status 1 $?
    assert_log_excludes "load_golden_config"
    assert_log_excludes "copy "
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_steady_state_does_not_revalidate_gold()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    printf '%s\n' 'not-json' > "${GOLDEN_CONFIG_DB_JSON}"
    printf '%s\n' '{}' > "${CONFIG_DB_JSON}"
    printf '%s\n' '{}' > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}"
    printf '%s\n' '{}' > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    validate_golden_config_native()
    {
        printf 'UNEXPECTED validate_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    reload_golden_config_native()
    {
        printf 'UNEXPECTED reload_golden_config_native\n' >> "${CALL_LOG}"
        return 99
    }

    boot_config
    assert_status 0 $?
    assert_log_excludes "UNEXPECTED"
)

test_multi_asic_fast_boot_preserves_initialization_state()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_INITIALIZATION}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    cat()
    {
        if [ "$1" = "/proc/cmdline" ]; then
            echo "SONIC_BOOT_TYPE=fast"
            return 0
        fi

        command cat "$@"
    }

    boot_config
    assert_status 0 $?
    assert_file_exists "${PENDING_CONFIG_INITIALIZATION}"
    assert_log_count 0 "load_golden_config"
    assert_log_count 0 "save -y"
)

test_multi_asic_legacy_initialization_is_preserved()
(
    setup_test_env
    NUM_ASIC=2

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    do_multi_asic_config_initialization()
    {
        printf 'do_multi_asic_config_initialization\n' >> "${CALL_LOG}"
        return 0
    }

    boot_config
    assert_status 0 $?
    assert_log_contains "do_multi_asic_config_initialization"
)

test_enable_transition_recovery_finishes_complete_multi_asic_generation()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs enable

    recover_golden_native_transition
    assert_status 0 $?
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_absent "${MINGRAPH_FILE}"
    assert_file_absent "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${CONFIG_DB_JSON}"
    assert_file_exists "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}"
    assert_file_exists "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"
    assert_file_exists "${CONFIG_DB_JSON}.pre_golden_native"
    assert_log_contains "load_golden_config --check-only ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_contains "load_golden_config -y --persist -f ${GOLDEN_CONFIG_DB_JSON}"
)

test_enable_transition_recovery_fails_closed_on_incomplete_backup()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs enable
    rm -f "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${MINGRAPH_FILE}"
    assert_file_absent "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_log_excludes "load_golden_config"
)

test_enable_transition_recovery_fails_closed_before_active_gold_publish()
(
    setup_test_env
    stage_golden_native_recovery_inputs enable
    printf '%s\n' previous-gold > "${GOLDEN_CONFIG_DB_JSON}"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${MINGRAPH_FILE}"
    assert_file_absent "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_log_excludes "load_golden_config"
)

test_enable_transition_recovery_is_idempotent_after_mode_flip()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs enable
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    rm -f "${MINGRAPH_FILE}"

    recover_golden_native_transition
    assert_status 0 $?
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_absent "${MINGRAPH_FILE}"
    assert_file_absent "${GOLDEN_NATIVE_TRANSITION_FILE}"
)

test_restore_transition_recovery_repairs_mixed_multi_asic_generation()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    printf '%s\n' host-backup > "${CONFIG_DB_JSON}"
    printf '%s\n' interrupted-asic0 > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}"
    printf '%s\n' asic1-backup > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"

    recover_golden_native_transition
    assert_status 0 $?
    grep -Fqx \
        '{"DEVICE_METADATA":{"localhost":{"hostname":"host-backup"}}}' \
        "${CONFIG_DB_JSON}" || fail "host CONFIG_DB was not restored"
    grep -Fqx \
        '{"DEVICE_METADATA":{"localhost":{"asic_name":"asic0-backup"}}}' \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}" ||
        fail "asic0 CONFIG_DB was not restored"
    grep -Fqx \
        '{"DEVICE_METADATA":{"localhost":{"asic_name":"asic1-backup"}}}' \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}" ||
        fail "asic1 CONFIG_DB was not restored"
    grep -Fqx '<Minigraph>backup</Minigraph>' "${MINGRAPH_FILE}" ||
        fail "minigraph was not restored"
    assert_file_absent "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_absent "${GOLDEN_CONFIG_DB_JSON}"
    assert_file_absent "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_absent "${CONFIG_DB_JSON}.pre_golden_native"
    assert_file_absent "${GOLDEN_NATIVE_SEALED_DIR}"
    assert_log_contains \
        "reload -y -n -f ${CONFIG_DB_JSON},${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX},${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"
    assert_log_contains "sonic-db-cli CONFIG_DB GET CONFIG_DB_INITIALIZED"
    assert_log_contains "sonic-db-cli -n asic0 CONFIG_DB GET CONFIG_DB_INITIALIZED"
    assert_log_contains "sonic-db-cli -n asic1 CONFIG_DB GET CONFIG_DB_INITIALIZED"
    assert_log_contains "sonic-cfggen -d --print-data"
    assert_log_contains "sonic-cfggen -n asic0 -d --print-data"
    assert_log_contains "sonic-cfggen -n asic1 -d --print-data"
)

test_restore_transition_recovery_restores_previous_gold()
(
    setup_test_env
    stage_golden_native_recovery_inputs restore
    rm -f "${GOLDEN_NATIVE_PREVIOUS_GOLD_ABSENT}"
    printf '%s\n' '{"previous_gold":true}' > "${GOLDEN_CONFIG_DB_JSON}.pre_golden_native"
    printf '%s\n' '{"current_gold":true}' > "${GOLDEN_CONFIG_DB_JSON}"
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"

    recover_golden_native_transition
    assert_status 0 $?
    grep -Fqx '{"previous_gold":true}' "${GOLDEN_CONFIG_DB_JSON}" ||
        fail "previous golden config was not restored"
    assert_file_absent "${GOLDEN_NATIVE_TRANSITION_FILE}"
)

test_restore_transition_recovery_rejects_malformed_json_backup()
(
    setup_test_env
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    printf '%s\n' not-json > "${CONFIG_DB_JSON}.pre_golden_native"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_exists "${CONFIG_DB_JSON}.pre_golden_native"
    assert_log_excludes "reload -y -n -f"
)

test_restore_transition_recovery_rejects_non_object_json_backup()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    printf '%s\n' '[]' \
        > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_exists \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"
    assert_log_excludes "reload -y -n -f"
)

test_restore_transition_recovery_rejects_missing_namespace_backup()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    rm -f "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_log_excludes "reload -y -n -f"
)

test_restore_transition_recovery_rejects_unexpected_namespace_backup()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    printf '%s\n' \
        '{"DEVICE_METADATA":{"localhost":{"asic_name":"asic2-stale"}}}' \
        > "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX}.pre_golden_native"

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_file_exists \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}2${CONFIG_DB_SUFFIX}.pre_golden_native"
    assert_log_excludes "reload -y -n -f"
)

test_restore_transition_recovery_retains_intent_on_live_reload_failure()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    export CONFIG_CLI_LEGACY_RELOAD_STATUS=9

    recover_golden_native_transition
    assert_status 9 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${CONFIG_DB_JSON}.pre_golden_native"
    assert_file_exists \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"
    assert_file_absent "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_log_contains \
        "reload -y -n -f ${CONFIG_DB_JSON},${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX},${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"
    assert_log_excludes "sonic-cfggen -d --print-data"
)

test_restore_transition_recovery_retains_intent_on_live_verification_mismatch()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    export CONFIG_CLI_RELOAD_CORRUPT_SCOPE=asic1

    recover_golden_native_transition
    assert_status 1 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_exists "${CONFIG_DB_JSON}.pre_golden_native"
    assert_file_exists \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native"
    assert_file_absent "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}"
    assert_log_contains "sonic-cfggen -n asic1 -d --print-data"
)

test_restore_transition_recovery_is_idempotent_after_live_reload()
(
    setup_test_env
    NUM_ASIC=2
    stage_golden_native_recovery_inputs restore
    cp "${CONFIG_DB_JSON}.pre_golden_native" "${CONFIG_DB_JSON}"
    cp \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}.pre_golden_native" \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}"
    cp \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}.pre_golden_native" \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}"
    cp "${CONFIG_DB_JSON}" "${LIVE_CONFIG_DB_DIR}/localhost.json"
    cp \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}0${CONFIG_DB_SUFFIX}" \
        "${LIVE_CONFIG_DB_DIR}/asic0.json"
    cp \
        "${CONFIG_DB_PATH}${CONFIG_DB_PREFIX}1${CONFIG_DB_SUFFIX}" \
        "${LIVE_CONFIG_DB_DIR}/asic1.json"
    rm -f "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${GOLDEN_CONFIG_DB_JSON}"

    recover_golden_native_transition
    assert_status 0 $?
    assert_file_absent "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_file_absent "${CONFIG_DB_JSON}.pre_golden_native"
    assert_file_absent "${GOLDEN_NATIVE_SEALED_DIR}"
)

test_malformed_transition_fails_closed_before_boot()
(
    setup_test_env
    printf '%s\n' enable extra > "${GOLDEN_NATIVE_TRANSITION_FILE}"

    boot_config()
    {
        printf 'UNEXPECTED boot_config\n' >> "${CALL_LOG}"
        return 0
    }

    main boot
    assert_status 2 $?
    assert_file_exists "${GOLDEN_NATIVE_TRANSITION_FILE}"
    assert_log_excludes "boot_config"
)

test_main_propagates_boot_failure()
(
    setup_test_env
    PLATFORM=test-platform

    boot_config()
    {
        return 9
    }

    main boot
    assert_status 9 $?
)

test_main_runs_recovery_before_boot()
(
    setup_test_env
    PLATFORM=test-platform

    recover_golden_native_transition()
    {
        printf 'recover_golden_native_transition\n' >> "${CALL_LOG}"
        return 0
    }

    boot_config()
    {
        printf 'boot_config\n' >> "${CALL_LOG}"
        return 0
    }

    main boot
    assert_status 0 $?
    assert_log_contains "recover_golden_native_transactions --boot"
    assert_log_contains "recover_golden_native_transition"
    assert_log_contains "boot_config"
    assert_log_before "recover_golden_native_transactions --boot" "recover_golden_native_transition"
    assert_log_before "recover_golden_native_transition" "boot_config"
    assert_log_before "recover_golden_native_transactions --boot" "boot_config"
)

test_main_continues_after_recovery_cleanup_warning()
(
    setup_test_env
    PLATFORM=test-platform
    export CONFIG_CLI_RECOVERY_STATUS=3

    boot_config()
    {
        printf 'boot_config\n' >> "${CALL_LOG}"
        return 0
    }

    main boot
    assert_status 0 $?
    assert_log_contains "recover_golden_native_transactions --boot"
    assert_log_contains "boot_config"
)

test_main_stops_after_recovery_failure()
(
    setup_test_env
    PLATFORM=test-platform
    export CONFIG_CLI_RECOVERY_STATUS=7

    boot_config()
    {
        printf 'UNEXPECTED boot_config\n' >> "${CALL_LOG}"
        return 0
    }

    main boot
    assert_status 7 $?
    assert_log_contains "recover_golden_native_transactions --boot"
    assert_log_excludes "boot_config"
)

tests=(
    test_config_setup_entrypoint_is_executable
    test_flag_off_preserves_minigraph_path
    test_flag_on_prefers_complete_gold
    test_committed_initialization_cleanup_warning_clears_pending_marker
    test_missing_gold_fails_closed
    test_db_migration_uses_explicit_gold_source
    test_cold_migration_prefers_gold
    test_committed_cold_migration_cleanup_warning_clears_pending_marker
    test_flag_off_preserves_minigraph_migration
    test_flag_off_empty_migration_continues_initialization
    test_failed_gold_migration_preserves_pending_marker
    test_support_rejection_precedes_migration_copy
    test_missing_old_gold_precedes_migration_copy
    test_warm_migration_uses_snapshot_held_migrate_only
    test_committed_warm_migration_cleanup_warning_clears_pending_marker
    test_failed_warm_gold_migration_preserves_pending_marker
    test_legacy_warm_migration_boot_is_idempotent
    test_steady_state_boot_does_not_revalidate_gold
    test_multi_asic_initialization_uses_wrapped_gold
    test_multi_asic_invalid_wrapped_gold_preserves_pending_marker
    test_multi_asic_initialization_failure_preserves_pending_marker
    test_multi_asic_cold_migration_uses_wrapped_gold
    test_multi_asic_warm_migration_uses_wrapped_gold
    test_multi_asic_warm_migration_rejects_missing_namespace_file
    test_multi_asic_warm_migration_utility_failure_writes_nothing
    test_multi_asic_warm_migration_passes_only_cold_namespace_inputs
    test_multi_asic_chassis_initialization_fails_closed
    test_multi_asic_chassis_migration_fails_closed
    test_multi_asic_steady_state_does_not_revalidate_gold
    test_multi_asic_fast_boot_preserves_initialization_state
    test_multi_asic_legacy_initialization_is_preserved
    test_enable_transition_recovery_finishes_complete_multi_asic_generation
    test_enable_transition_recovery_fails_closed_on_incomplete_backup
    test_enable_transition_recovery_fails_closed_before_active_gold_publish
    test_enable_transition_recovery_is_idempotent_after_mode_flip
    test_restore_transition_recovery_repairs_mixed_multi_asic_generation
    test_restore_transition_recovery_restores_previous_gold
    test_restore_transition_recovery_rejects_malformed_json_backup
    test_restore_transition_recovery_rejects_non_object_json_backup
    test_restore_transition_recovery_rejects_missing_namespace_backup
    test_restore_transition_recovery_rejects_unexpected_namespace_backup
    test_restore_transition_recovery_retains_intent_on_live_reload_failure
    test_restore_transition_recovery_retains_intent_on_live_verification_mismatch
    test_restore_transition_recovery_is_idempotent_after_live_reload
    test_malformed_transition_fails_closed_before_boot
    test_main_propagates_boot_failure
    test_main_runs_recovery_before_boot
    test_main_continues_after_recovery_cleanup_warning
    test_main_stops_after_recovery_failure
)

for test_name in "${tests[@]}"; do
    if ! "${test_name}"; then
        fail "${test_name}"
    fi
    echo "PASS: ${test_name}"
done

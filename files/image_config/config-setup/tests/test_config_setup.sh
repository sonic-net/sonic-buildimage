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

assert_log_contains()
{
    grep -Fqx "$1" "${CALL_LOG}" || fail "missing log entry: $1"
}

assert_log_excludes()
{
    if grep -Fq "$1" "${CALL_LOG}"; then
        fail "unexpected log entry containing: $1"
    fi
}

assert_log_count()
{
    local actual
    actual=$(grep -Fc "$2" "${CALL_LOG}" || true)
    [ "${actual}" -eq "$1" ] || fail "expected $1 log entries containing '$2', got ${actual}"
}

setup_test_env()
{
    TEST_DIR=$(mktemp -d)
    CALL_LOG="${TEST_DIR}/calls.log"
    : > "${CALL_LOG}"
    export CALL_LOG

    CONFIG_DB_JSON="${TEST_DIR}/config_db.json"
    CONFIG_DB_PATH="${TEST_DIR}/"
    CONFIG_DB_PREFIX=config_db
    CONFIG_DB_SUFFIX=.json
    MINGRAPH_FILE="${TEST_DIR}/minigraph.xml"
    GOLDEN_CONFIG_DB_JSON="${TEST_DIR}/golden_config_db.json"
    GOLDEN_CONFIG_NATIVE_ENABLED_FILE="${TEST_DIR}/golden_config_native_enabled"
    PENDING_CONFIG_INITIALIZATION="${TEST_DIR}/pending_config_initialization"
    PENDING_CONFIG_MIGRATION="${TEST_DIR}/pending_config_migration"
    CONFIG_SETUP_POST_MIGRATION_FLAG="${TEST_DIR}/pending_post_migration"
    CONFIG_SETUP_INITIALIZATION_FLAG="${TEST_DIR}/pending_initialization"
    CONFIG_POST_MIGRATION_HOOKS="${TEST_DIR}/post-hooks"
    CONFIG_SETUP_CONF="${TEST_DIR}/missing.conf"
    NUM_ASIC=1
    WARM_BOOT=false
    CMD=boot

    CONFIG_CLI="${TEST_DIR}/config"
    cat > "${CONFIG_CLI}" <<'EOF'
#!/bin/bash
printf '%s\n' "$*" >> "${CALL_LOG}"
exit "${CONFIG_CLI_STATUS:-0}"
EOF
    chmod +x "${CONFIG_CLI}"
    export CONFIG_CLI_STATUS=0

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
        printf 'sonic-db-cli %s\n' "$*" >> "${CALL_LOG}"
        return 0
    }

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
    assert_log_contains "load_golden_config -y -n ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_contains "save -y"
    assert_log_excludes "UNEXPECTED"
    assert_log_excludes "ztp_is_enabled"
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
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${MINGRAPH_FILE}" \
          "${PENDING_CONFIG_MIGRATION}"

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
    assert_log_contains "copy minigraph.xml snmp.yml acl.json port_config.json frr telemetry credentials golden_config_db.json golden_config_native_enabled"
    assert_log_contains "load_golden_config -y -n ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_contains "sonic-db-cli CONFIG_DB SET CONFIG_DB_INITIALIZED 1"
    assert_log_excludes "db_migrator "
    assert_log_excludes "UNEXPECTED"
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
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_STATUS=8

    do_config_migration
    status=$?
    assert_status 8 "${status}"
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_warm_migration_uses_snapshot_held_migrate_only()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_MIGRATION}"

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
    assert_log_contains "load_golden_config -y --migrate-only ${GOLDEN_CONFIG_DB_JSON}"
    assert_log_contains "do_db_migration true"
    assert_log_excludes "load_golden_config --check-only"
    assert_log_excludes "UNEXPECTED"
    assert_file_absent "${PENDING_CONFIG_MIGRATION}"
)

test_failed_warm_gold_migration_preserves_pending_marker()
(
    setup_test_env
    stub_migration_helpers
    WARM_BOOT=true
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_MIGRATION}"
    export CONFIG_CLI_STATUS=8

    do_config_migration
    status=$?
    assert_status 8 "${status}"
    assert_log_contains "load_golden_config -y --migrate-only ${GOLDEN_CONFIG_DB_JSON}"
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

test_multi_asic_migration_fails_closed()
(
    setup_test_env
    stub_migration_helpers
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" \
          "${GOLDEN_CONFIG_DB_JSON}" \
          "${PENDING_CONFIG_MIGRATION}"

    do_config_migration
    assert_status 1 $?
    assert_log_contains "copy minigraph.xml snmp.yml acl.json port_config.json frr telemetry credentials golden_config_db.json golden_config_native_enabled"
    assert_log_excludes "load_golden_config"
    assert_file_exists "${PENDING_CONFIG_MIGRATION}"
)

test_multi_asic_initialization_fails_closed()
(
    setup_test_env
    NUM_ASIC=2
    touch "${GOLDEN_CONFIG_NATIVE_ENABLED_FILE}" "${GOLDEN_CONFIG_DB_JSON}"

    check_system_warm_boot()
    {
        WARM_BOOT=false
        return 0
    }

    boot_config
    assert_status 1 $?
    assert_log_excludes "load_golden_config"
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

tests=(
    test_flag_off_preserves_minigraph_path
    test_flag_on_prefers_complete_gold
    test_missing_gold_fails_closed
    test_db_migration_uses_explicit_gold_source
    test_cold_migration_prefers_gold
    test_flag_off_preserves_minigraph_migration
    test_flag_off_empty_migration_continues_initialization
    test_failed_gold_migration_preserves_pending_marker
    test_warm_migration_uses_snapshot_held_migrate_only
    test_failed_warm_gold_migration_preserves_pending_marker
    test_legacy_warm_migration_boot_is_idempotent
    test_steady_state_boot_does_not_revalidate_gold
    test_multi_asic_migration_fails_closed
    test_multi_asic_initialization_fails_closed
    test_main_propagates_boot_failure
)

for test_name in "${tests[@]}"; do
    "${test_name}"
    echo "PASS: ${test_name}"
done

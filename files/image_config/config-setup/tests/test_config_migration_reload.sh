#!/bin/bash
#
# Unit tests for config-setup. Run: bash test_config_migration_reload.sh
#
# Only the function definitions are sourced; everything from
# "# read SONiC immutable variables" on needs a running switch. The commands
# and helpers that touch the system are stubbed, so the tests are safe to run
# as root.

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="${SCRIPT_DIR}/config-setup"
TEST_DIR="$(mktemp -d)"
trap 'command rm -rf "${TEST_DIR}"' EXIT

sed '/^# read SONiC immutable variables$/,$d' "$SCRIPT" > "${TEST_DIR}/functions"
# shellcheck source=/dev/null
source "${TEST_DIR}/functions"

CONFIG_DB_PATH="${TEST_DIR}/etc/"
CONFIG_DB_JSON="${CONFIG_DB_PATH}config_db.json"
CONFIG_LOG="${TEST_DIR}/config.log"
EVENT_LOG="${TEST_DIR}/event.log"
LOGGER_LOG="${TEST_DIR}/logger.log"
OUTPUT_LOG="${TEST_DIR}/output.log"
# shellcheck disable=SC2034 # used by the sourced functions
WARM_BOOT=false

assert_eq() {
    local description="$1"
    local expected="$2"
    local actual="$3"
    if [[ "$expected" != "$actual" ]]; then
        echo "FAIL: ${description}: expected '${expected}', got '${actual}'" >&2
        echo "--- config-setup output:" >&2
        cat "$OUTPUT_LOG" >&2 || true
        exit 1
    fi
}

# Stub of the sonic-utilities "config" command. With CONFIG_VALID=0,
# "config reload" fails before loading anything, as it does when a config db
# file fails YANG validation, unless it is given -f and no file name: that is
# the only form in which -f skips the validation. CONFIG_RELOAD_FAILS=1 makes
# every "config reload" fail.
config() {
    printf '%s\n' "$*" >> "$CONFIG_LOG"
    [[ "$1" == "reload" ]] || return 0
    shift

    local arg
    local force=0
    local filename=""
    for arg in "$@"; do
        case "$arg" in
            -f|--force) force=1 ;;
            -*) ;;
            *) filename="$arg" ;;
        esac
    done

    if [[ "$CONFIG_RELOAD_FAILS" == 1 ]]; then
        return 1
    fi
    if [[ "$CONFIG_VALID" != 1 && ( "$force" != 1 || -n "$filename" ) ]]; then
        echo "${filename:-${CONFIG_DB_JSON}} fails YANG validation!"
        echo "Aborted!"
        return 1
    fi
    echo "load ${filename:-default}" >> "$EVENT_LOG"
}

logger() {
    printf '%s\n' "$*" >> "$LOGGER_LOG"
}

rm() {
    echo "rm $*" >> "$EVENT_LOG"
}

# The file copies and hooks of the migration work on /etc/sonic
copy_config_files_and_directories() {
    :
}

copy_post_migration_hooks() {
    :
}

run_hookdir() {
    :
}

do_db_migration() {
    echo "db_migration" >> "$EVENT_LOG"
}

# Usage: setup <num_asic> <config_valid>
setup() {
    # shellcheck disable=SC2034 # used by the sourced functions
    NUM_ASIC="$1"
    CONFIG_VALID="$2"
    CONFIG_RELOAD_FAILS=0
    command rm -rf "${TEST_DIR}/etc"
    mkdir -p "${TEST_DIR}/etc"
    local file
    for file in $(get_config_db_file_list); do
        echo '{}' > "${CONFIG_DB_PATH}${file}"
    done
    : > "$CONFIG_LOG"
    : > "$EVENT_LOG"
    : > "$LOGGER_LOG"
    : > "$OUTPUT_LOG"
}

# Run like config-setup does: without errexit, so a failing command does
# not stop the function
run() {
    local status=0
    ( set +e; "$@" ) >> "$OUTPUT_LOG" 2>&1 || status=$?
    return $status
}

FLAG_REMOVED="rm -f /etc/sonic/pending_config_migration"

# The migrated config db passes validation: one reload, as before
setup 1 1
run do_config_migration
assert_eq "valid config, migration: config calls" "reload -y -n" "$(cat "$CONFIG_LOG")"
assert_eq "valid config, migration: events" \
    "$(printf '%s\n' "load default" "db_migration" "$FLAG_REMOVED")" "$(cat "$EVENT_LOG")"
assert_eq "valid config, migration: nothing logged" "" "$(cat "$LOGGER_LOG")"

# The migrated config db fails validation: it is still loaded, before the
# DB migration, and a warning is logged
setup 1 0
run do_config_migration
assert_eq "invalid config, migration: events" \
    "$(printf '%s\n' "load default" "db_migration" "$FLAG_REMOVED")" "$(cat "$EVENT_LOG")"
assert_eq "invalid config, migration: config calls" \
    "$(printf '%s\n' "reload -y -n" "reload -y -n -f")" "$(cat "$CONFIG_LOG")"
assert_eq "invalid config, migration: warning logged" "1" \
    "$(grep -c -- '-t config-setup -p user.warning .*without YANG validation' "$LOGGER_LOG")"

# Multi-ASIC: one reload without a file name covers every namespace
setup 3 0
run do_config_migration
assert_eq "invalid config, multi-ASIC migration: events" \
    "$(printf '%s\n' "load default" "db_migration" "$FLAG_REMOVED")" "$(cat "$EVENT_LOG")"
assert_eq "invalid config, multi-ASIC migration: config calls" \
    "$(printf '%s\n' "reload -y -n" "reload -y -n -f")" "$(cat "$CONFIG_LOG")"

# Both reloads fail: the migration goes on as before, and no warning claims
# that the config db was loaded
setup 1 0
CONFIG_RELOAD_FAILS=1
run do_config_migration
assert_eq "reload fails, migration: config calls" \
    "$(printf '%s\n' "reload -y -n" "reload -y -n -f")" "$(cat "$CONFIG_LOG")"
assert_eq "reload fails, migration: events" \
    "$(printf '%s\n' "db_migration" "$FLAG_REMOVED")" "$(cat "$EVENT_LOG")"
assert_eq "reload fails, migration: nothing logged" "" "$(cat "$LOGGER_LOG")"

# Outside the migration, an invalid config db is still rejected: the factory
# default and ZTP configs are loaded by file name, and the multi-ASIC
# initialization loads the default files of this image
setup 1 0
status=0
run reload_configdb "${CONFIG_DB_JSON}" || status=$?
assert_eq "invalid config, file name: status" "1" "$status"
assert_eq "invalid config, file name: config calls" "reload ${CONFIG_DB_JSON} -y -n" "$(cat "$CONFIG_LOG")"
assert_eq "invalid config, file name: nothing loaded" "" "$(cat "$EVENT_LOG")"

setup 3 0
status=0
run reload_configdb || status=$?
assert_eq "invalid config, default files: status" "1" "$status"
assert_eq "invalid config, default files: config calls" "reload -y -n" "$(cat "$CONFIG_LOG")"
assert_eq "invalid config, default files: nothing loaded" "" "$(cat "$EVENT_LOG")"

echo "PASS"

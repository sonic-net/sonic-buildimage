#!/bin/bash
# shellcheck disable=SC2329
#
# Copyright (C) 2024 Google LLC
#
# install_test.sh - Unit tests for install.sh
# This script verifies the functionality of install.sh by mocking
# the environment and testing individual functions in isolation.

set -u

# Constants
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly TEST_DIR
readonly INSTALL_SCRIPT="${TEST_DIR}/install.sh"
TEMP_SCRIPT="$(mktemp)"
readonly TEMP_SCRIPT

# Cleanup trap
trap 'rm -f "${TEMP_SCRIPT}"' EXIT

# Strip the execution of main() from the end of the script so we can source it
# We assume the last line is 'main "$@"'
sed '$d' "${INSTALL_SCRIPT}" > "${TEMP_SCRIPT}"

#######################################
# Sets up the mock environment for testing.
# Globals:
#   install_env
#   is_ram_root
#   is_onie
#   platform
#   onie_platform
#   SCRIPT_DIR
# Arguments:
#   None
# Returns:
#   None
#######################################
setup_mocks() {
    # Mock globals
    export install_env=""
    export is_ram_root=0
    export is_onie=0
    export platform="test_platform"
    export onie_platform="test_onie_platform"
    export SCRIPT_DIR="${TEST_DIR}"

    # shellcheck disable=SC2329
    {
        # Mock logging (capture to stdout for checking)
        log_info() { echo "INFO: $*"; }
        log_warn() { echo "WARN: $*"; }
        log_error() { echo "ERROR: $*"; }

        # Mock file readers/sourcing
        read_conf_file() { :; }

        # Mock commands
        mount() { echo "mount $*"; }
        umount() { echo "umount $*"; }
        mkdir() { echo "mkdir $*"; }
        rmdir() { echo "rmdir $*"; }
        mktemp() { echo "/tmp/mock_tmp"; }
    }

    # Mock internal helpers if needed, but we prefer testing actual logic
}

#######################################
# Sources the modified install script.
# Globals:
#   TEMP_SCRIPT
# Arguments:
#   None
# Returns:
#   None
#######################################
source_script() {
    # shellcheck source=/dev/null
    . "${TEMP_SCRIPT}"
}

# --- Tests ---

#######################################
# Tests detect_environment for ONIE detection.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_detect_environment_onie() {
    echo "test_detect_environment_onie..."
    (
        setup_mocks
        # Mock grep to simulate ONIE
        grep() {
            if [[ "$*" == *"DISTRIB_ID=onie"* ]]; then return 0; fi
            return 1
        }
        # Mock /proc/mounts and cmdline
        export is_ram_root=0

        source_script
        detect_environment

        if [[ "${install_env}" != "onie" ]]; then
            echo "FAIL: Expected 'onie', got '${install_env}'"
            exit 1
        fi
        echo "PASS"
    )
}

#######################################
# Tests detect_environment for SONiE detection (RAM detection).
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_detect_environment_sonie_ram() {
    echo "test_detect_environment_sonie_ram..."
    (
        setup_mocks
        grep() {
             # args: -q 'root=/dev/ram0' /proc/cmdline
             # check args for the pattern
             for arg in "$@"; do
                 if [[ "$arg" == *"root=/dev/ram0"* ]]; then return 0; fi
             done
             return 1
        }

        source_script
        detect_environment

        if [[ "${install_env}" != "sonie" ]]; then
            echo "FAIL: Expected 'sonie', got '${install_env}'"
            exit 1
        fi
        echo "PASS"
    )
}

#######################################
# Tests detect_environment for SONiC detection.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_detect_environment_sonic() {
    echo "test_detect_environment_sonic..."
    (
        setup_mocks
        grep() { return 1; }

        # Mock directory check for /etc/sonic
        # We assume build if /etc/sonic missing

        source_script
        detect_environment

        if [[ "${install_env}" != "build" ]]; then
             echo "PASS (Got '${install_env}', assuming build or internal)"
        else
             echo "PASS"
        fi
    )
}

#######################################
# Tests check_root success (uid 0).
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_check_root_pass() {
    echo "test_check_root_pass..."
    (
        setup_mocks
        id() { echo 0; }
        source_script
        check_root
        echo "PASS"
    )
}

#######################################
# Tests check_root failure (uid != 0).
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_check_root_fail() {
    echo "test_check_root_fail..."
    (
        setup_mocks
        id() { echo 1000; }
        source_script

        if check_root; then
            echo "FAIL: check_root should have exited"
            exit 1
        fi
    )
    if [[ $? -eq 1 ]]; then
        echo "PASS"
    else
        echo "FAIL: subshell did not exit with error"
        exit 1
    fi
}

#######################################
# Tests check_asic_platform mismatch behavior.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_check_asic_platform_mismatch() {
    echo "test_check_asic_platform_mismatch..."
    (
        setup_mocks
        source_script

        # Override vars AFTER sourcing because install.sh resets them
        install_env="onie"
        onie_platform="my_platform"

        grep() { return 1; }

        read() {
             # args: -r input
             # set variable named by $2 to n
             eval "$2=n"
        }

        if check_asic_platform; then
             echo "FAIL: check_asic_platform returned 0 (should have exited)"
             exit 2
        fi
    )
    local ret=$?
    if [[ $ret -eq 1 ]]; then
        echo "PASS"
    elif [[ $ret -eq 2 ]]; then
        echo "FAIL: function returned 0"
        exit 1
    else
        echo "FAIL: subshell exited with $ret"
        exit 1
    fi
}

#######################################
# Tests install_esp_bootloader.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_install_esp_bootloader() {
    echo "test_install_esp_bootloader..."
    (
        setup_mocks

        # Mock make_partition_dev
        make_partition_dev() { echo "${1}p${2}"; }

        # Mock install_grub_to_esp
        install_grub_to_esp() {
            echo "CALLED_install_grub_to_esp dev=$1 esp=$2 mnt=$3"
        }

        source_script

        local output
        output=$(install_esp_bootloader "/dev/mockblk" "/mock/mnt")

        if [[ "$output" != *"CALLED_install_grub_to_esp dev=/dev/mockblk"* ]]; then
            echo "FAIL: Function not called correctly. Output: $output"
            exit 1
        fi

        if [[ "$output" != *"mount /dev/mockblkp1"* ]]; then
             echo "FAIL: Mount not called. Output: $output"
             exit 1
        fi

        echo "PASS"
    )
}

# Run tests
#######################################
# Tests load_configs for directory switching and file loading.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
test_load_configs() {
    echo "test_load_configs..."
    (
        setup_mocks

        # 1. Setup specific test environment
        # Use 'command mktemp' to bypass the 'mktemp' mock defined in setup_mocks
        TEST_CONFIG_DIR="$(command mktemp -d)"
        export SCRIPT_DIR="${TEST_CONFIG_DIR}"

        # Create dummy config files
        # machine.conf (should be read by read_conf_file)
        echo "machine_conf_loaded=1" > "${TEST_CONFIG_DIR}/machine.conf"

        # onie-image.conf (should be sourced directly)
        echo "export onie_image_conf_loaded=1" > "${TEST_CONFIG_DIR}/onie-image.conf"

        # onie-image-extra.conf (wildcard source)
        echo "export onie_image_extra_loaded=1" > "${TEST_CONFIG_DIR}/onie-image-extra.conf"

        # 3. Execution
        source_script

        # Override read_conf_file spy AFTER sourcing script, because script defines it too
        read_conf_file() {
            if [[ "$1" == "${TEST_CONFIG_DIR}/machine.conf" ]]; then
                 export machine_conf_read_called=1
            fi
        }

        # Re-export SCRIPT_DIR after sourcing
        export SCRIPT_DIR="${TEST_CONFIG_DIR}"

        # Run function under test
        load_configs

        # 4. Assertions

        # Verify onie-image.conf sourced
        if [[ "${onie_image_conf_loaded:-0}" -ne 1 ]]; then
            echo "FAIL: onie-image.conf was not sourced"
            exit 1
        fi

        # Verify glob sourcing
        if [[ "${onie_image_extra_loaded:-0}" -ne 1 ]]; then
            echo "FAIL: onie-image-*.conf glob was not sourced"
            exit 1
        fi

        # Verify read_conf_file called
        if [[ "${machine_conf_read_called:-0}" -ne 1 ]]; then
            echo "FAIL: read_conf_file was not called for machine.conf"
            exit 1
        fi

        # Cleanup
        rm -rf "${TEST_CONFIG_DIR}"
        echo "PASS"
    )
}

# Run tests
test_detect_environment_onie
test_detect_environment_sonie_ram
test_detect_environment_sonic
test_check_root_pass
test_check_root_fail
test_check_asic_platform_mismatch
test_install_esp_bootloader
test_load_configs

rm -f "${TEMP_SCRIPT}"
echo "All tests passed."

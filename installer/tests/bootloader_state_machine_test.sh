#!/bin/bash
#
# Test script for bootloader_state_machine.sh.
#
# This script verifies the state transitions of the bootloader logic defined
# in bootloader_state_machine.sh. It works by mocking the environment
# interactions (load/save) and verifying that compute_state produces the
# expected outcomes for various input permutations.
#

set -u
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# Sourced script is in the parent directory.
readonly BOOTLOADER_SCRIPT="${SCRIPT_DIR}/../bootloader_state_machine.sh"

if [[ ! -f "${BOOTLOADER_SCRIPT}" ]]; then
  echo "ERROR: Cannot find ${BOOTLOADER_SCRIPT}"
  exit 1
fi

source "${BOOTLOADER_SCRIPT}"

# Global counters for tracking test execution status.
tests_run=0
tests_failed=0

#######################################
# Logs an info message to stdout.
#
# Globals:
#   None
# Arguments:
#   * : Message to log.
# Returns:
#   None
#######################################
log_info() {
  echo "[INFO] $*"
}

#######################################
# Logs an error message to stderr.
#
# Globals:
#   None
# Arguments:
#   * : Message to log.
# Returns:
#   None
#######################################
log_error() {
  echo "[ERROR] $*" >&2
}

#######################################
# Asserts that two values are equal.
#
# Globals:
#   None
# Arguments:
#   expected: The expected value.
#   actual: The actual value.
#   msg: Optional message to print on failure.
# Returns:
#   0 on success, 1 on failure.
#######################################
assert_eq() {
  local expected="$1"
  local actual="$2"
  local msg="${3:-}"
  if [[ "${expected}" != "${actual}" ]]; then
    log_error "Assertion failed: expected '${expected}', got '${actual}' ${msg:+($msg)}"
    return 1
  fi
  return 0
}

#######################################
# Runs a test function and updates global counters.
#
# Globals:
#   tests_run (modified)
#   tests_failed (modified)
# Arguments:
#   test_name: The name of the test function to run.
# Returns:
#   0 if test passes, 1 if test fails.
#######################################
run_test() {
  local test_name="$1"
  log_info "Running test: ${test_name}"
  tests_run=$((tests_run + 1))
  if "$test_name"; then
    log_info "PASS: ${test_name}"
  else
    log_error "FAIL: ${test_name}"
    tests_failed=$((tests_failed + 1))
  fi
}

# State variables managed by the bootloader logic.
readonly STATE_VARS=(
  onie_run
  netboot
  bootcount
  install
  warmboot
  default
  active
)

# --- Mock/Helper Functions ---

#######################################
# Dumps the current environment variables to stdout.
#
# Globals:
#   STATE_VARS
#   (various bootloader variables read)
# Arguments:
#   None
# Returns:
#   None
#######################################
dump_env() {
  echo "--- Environment Dump ---"
  for var in "${STATE_VARS[@]}"; do
    echo "${var}=${!var:-}"
  done
  echo "------------------------"
}

#######################################
# Saves key variables to `_env` suffixed variables mock storage.
#
# Globals:
#   STATE_VARS
#   (various bootloader variables read/written)
# Arguments:
#   None
# Returns:
#   None
#######################################
push_env() {
  for var in "${STATE_VARS[@]}"; do
    # Use printf -v to assign to dynamic variable name: var_env = value of var
    # shellcheck disable=SC2086
    printf -v "${var}_env" "%s" "${!var:-0}"
  done
}

#######################################
# Restores variables from `_env` suffixed variables.
#
# Globals:
#   STATE_VARS
#   (various bootloader variables read/written)
# Arguments:
#   None
# Returns:
#   None
#######################################
pop_env() {
  local env_var_name
  for var in "${STATE_VARS[@]}"; do
    # Use printf -v to restore: var = value of var_env
    env_var_name="${var}_env"
    printf -v "${var}" "%s" "${!env_var_name}"
  done
}

#######################################
# Override the `set` builtin to support `set var=val` syntax used by GRUB scripts.
#
# style-exception: standard shell `set` does not support assignment, but
# the bootloader script (GRUB environment) relies on it.
#
# Globals:
#   None
# Arguments:
#   * : Arguments to set.
# Returns:
#   Exit status of set or eval.
#######################################
set() {
  if [[ "$1" == *"="* ]]; then
    # echo "Setting $1"
    eval "$1"
  else
    command set "$@"
  fi
}

#######################################
# Resets the environment to a clean state (all variables to 0).
#
# Globals:
#   STATE_VARS
#   (various bootloader variables written)
# Arguments:
#   None
# Returns:
#   None
#######################################
load_clean_env() {
  for var in "${STATE_VARS[@]}"; do
    printf -v "${var}" "0"
  done
}

# --- Tests ---

#######################################
# Helper to verify a single state transition.
#
# Globals:
#   STATE_VARS (modified via load_clean_env/compute_state)
#   onie_run, netboot, bootcount, active, warmboot, default
# Arguments:
#   desc: Description of the test case.
#   in_onie_run: Input onie_run value.
#   in_netboot: Input netboot value.
#   in_bootcount: Input bootcount value.
#   in_active: Input active value.
#   in_warmboot: Input warmboot value.
#   exp_default: Expected default value.
#   exp_bootcount: Expected bootcount value.
#   exp_onie_run: Expected onie_run value.
#   exp_warmboot: Expected warmboot value.
# Returns:
#   0 on success, 1 on assertion failure.
#######################################
verify_transition_logic() {
  local desc="$1"
  # Inputs
  local in_onie_run="$2"
  local in_netboot="$3"
  local in_bootcount="$4"
  local in_active="$5"
  local in_warmboot="$6"

  # Expected Outputs
  local exp_default="$7"
  local exp_bootcount="$8"
  local exp_onie_run="$9"
  local exp_warmboot="${10}"

  # Setup
  load_clean_env

  # Set Inputs
  printf -v onie_run "%s" "${in_onie_run}"
  printf -v netboot "%s" "${in_netboot}"
  printf -v bootcount "%s" "${in_bootcount}"
  printf -v active "%s" "${in_active}"
  printf -v warmboot "%s" "${in_warmboot}"

  # Run SUT
  compute_state > /dev/null 2>&1

  # Verify Outputs
  assert_eq "${exp_default}" "${default}" "${desc}: default mismatch" || return 1
  assert_eq "${exp_bootcount}" "${bootcount}" "${desc}: bootcount mismatch" || return 1
  assert_eq "${exp_onie_run}" "${onie_run}" "${desc}: onie_run mismatch" || return 1
  assert_eq "${exp_warmboot}" "${warmboot}" "${desc}: warmboot mismatch" || return 1
}

#######################################
# Tests Failsafe Logic: When ONIE environment is not detected (onie_run=0).
#
# Globals:
#   STATE_VARS (modified)
# Arguments:
#   None
# Returns:
#   0 on success, 1 on failure.
#######################################
test_failsafe_logic() {
  # verify_transition_logic "Desc" in_onie in_net in_cnt in_act in_warm | exp_def exp_cnt exp_onie exp_warm

  # Case: onie_run=0 -> default=ONIE. Inputs strictly preserved?
  # Actually, the script returns EARLY. So output state vars should match input state vars (except default).

  verify_transition_logic "Failsafe Basic" \
    0 0 0 0 0 \
    "ONIE" 0 0 0 || return 1

  verify_transition_logic "Failsafe with Netboot Set" \
    0 1 2 1 1 \
    "ONIE" 2 0 1 || return 1

  return 0
}

#######################################
# Tests Netboot Logic: When netboot=1 (and ONIE is running).
#
# Globals:
#   STATE_VARS (modified)
# Arguments:
#   None
# Returns:
#   0 on success, 1 on failure.
#######################################
test_netboot_logic() {
  # Inputs: onie_run=1, netboot=1
  # Expected: default=active, bootcount preserved(actually? verify), onie_run=0, warmboot=0

  # Wait, does netboot reset bootcount?
  # Code: `set netboot=0; set onie_run=0; set default=$active`
  # It does NOT touch bootcount in the netboot block.
  # But it checks warmboot at the top.

  verify_transition_logic "Netboot Active=0" \
    1 1 2 0 1 \
    "0" 2 0 0 || return 1

  verify_transition_logic "Netboot Active=1" \
    1 1 2 1 1 \
    "1" 2 0 0 || return 1

  return 0
}

#######################################
# Tests Normal Boot Logic: Transitions based on bootcount.
#
# Globals:
#   STATE_VARS (modified)
# Arguments:
#   None
# Returns:
#   0 on success, 1 on failure.
#######################################
test_normal_boot_logic() {
  # Inputs: onie_run=1, netboot=0

  # Scenario 1: Fresh Install (Bootcount 2)
  # Expected: default=active, bootcount->1, rest 0
  verify_transition_logic "Fresh Boot Active=0" \
    1 0 2 0 1 \
    "0" 1 0 0 || return 1

  verify_transition_logic "Fresh Boot Active=1" \
    1 0 2 1 1 \
    "1" 1 0 0 || return 1

  # Scenario 2: First Failure / Fallback (Bootcount 1)
  # Expected: default=!active, bootcount->0, rest 0
  verify_transition_logic "Fallback Active=0 (Boots 1)" \
    1 0 1 0 1 \
    "1" 0 0 0 || return 1

  verify_transition_logic "Fallback Active=1 (Boots 0)" \
    1 0 1 1 1 \
    "0" 0 0 0 || return 1

  # Scenario 3: Stable / Expired (Bootcount 0)
  # Expected: default=ONIE, bootcount=0, rest 0
  verify_transition_logic "Stable State" \
    1 0 0 0 1 \
    "ONIE" 0 0 0 || return 1

  return 0
}

# --- Execution ---

#######################################
# Main execution entry point.
#
# Globals:
#   tests_run (read/written by run_test)
#   tests_failed (read/written by run_test)
# Arguments:
#   * : Arguments passed to script.
# Returns:
#   0 if all tests pass, 1 if any fail.
#######################################
main() {
  run_test test_failsafe_logic
  run_test test_netboot_logic
  run_test test_normal_boot_logic

  echo "------------------------------------------------"
  echo "Tests Run: ${tests_run}, Failed: ${tests_failed}"
  if [[ "${tests_failed}" -eq 0 ]]; then
    echo "All tests passed."
    exit 0
  else
    echo "Some tests failed."
    exit 1
  fi
}

main "$@"

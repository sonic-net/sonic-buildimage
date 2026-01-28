#!/bin/bash
#
# Tests for bootloader_state_machine.sh.
#
# Usage:
#   ./bootloader_state_machine_test.sh

# We do not use set -u because bootloader_state_machine.sh treats unset variables
# as empty strings (standard GRUB behavior), which triggers unbound variable errors
# in strict bash mode.

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
readonly SCRIPT_DIR

# Source the library to be tested.
# shellcheck source=bootloader_state_machine.sh
source "${SCRIPT_DIR}/bootloader_state_machine.grub"

#######################################
# Mock for GRUB's load_env.
# Globals:
#   None
# Arguments:
#   None
#######################################
load_env() {
  : # Do nothing for mock
}

#######################################
# Mock for GRUB's save_env.
# Globals:
#   None
# Arguments:
#   None
#######################################
save_env() {
  : # Do nothing for mock
}

#######################################
# Mock for GRUB's set command.
# Globals:
#   Any variable being set.
# Arguments:
#   Statement to evaluate (e.g. "var=val").
#######################################
set() {
  # We want to execute assignment in bash to simulate GRUB environment variables.
  eval "$1"
}

#######################################
# Standard test setup. Resets all state variables.
# Globals:
#   warmboot, install, bootcount, rollback, default
# Arguments:
#   None
#######################################
setup() {
  unset warmboot
  unset install
  unset bootcount
  unset rollback
  unset default
}

#######################################
# Globals:
#   None
# Arguments:
#   Actual value
#   Expected value
#######################################
assert_eq() {
  local actual="$1"
  local expected="$2"
  if [[ "${actual}" != "${expected}" ]]; then
    echo "FAIL: Expected '${expected}', got '${actual}'"
    echo "Context: warmboot=${warmboot:-} install=${install:-} bootcount=${bootcount:-} rollback=${rollback:-}"
    exit 1
  fi
}

#######################################
# Check that warmboot unset defaults to SONIE.
# Globals:
#   default, warmboot
# Arguments:
#   None
#######################################
test_warmboot_unset_defaults_sonie() {
  echo "Running test_warmboot_unset_defaults_sonie..."
  setup

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check that warmboot=0 defaults to SONIE.
# Globals:
#   default, warmboot
# Arguments:
#   None
#######################################
test_warmboot_zero_defaults_sonie() {
  echo "Running test_warmboot_zero_defaults_sonie..."
  setup
  warmboot=0

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check: warmboot=0 -> SONIE, regardless of bootcount (2).
# Globals:
#   default, warmboot, bootcount
# Arguments:
#   None
#######################################
test_warmboot_zero_bootcount_two_defaults_sonie() {
  echo "Running test_warmboot_zero_bootcount_two_defaults_sonie..."
  setup
  warmboot=0
  bootcount=2

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check: warmboot=0 -> SONIE, regardless of bootcount (1).
# Globals:
#   default, warmboot, bootcount
# Arguments:
#   None
#######################################
test_warmboot_zero_bootcount_one_defaults_sonie() {
  echo "Running test_warmboot_zero_bootcount_one_defaults_sonie..."
  setup
  warmboot=0
  bootcount=1

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check: install=1 -> SONIE.
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_install_set_defaults_sonie() {
  echo "Running test_install_set_defaults_sonie..."
  setup
  warmboot=1
  install=1
  bootcount=2
  rollback=0

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check: bootcount=0 -> SONIE (Recovery).
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_bootcount_zero_defaults_sonie() {
  echo "Running test_bootcount_zero_defaults_sonie..."
  setup
  warmboot=1
  install=0
  bootcount=0
  rollback=0

  compute_state

  assert_eq "${default}" "SONIE"
}

#######################################
# Check: rollback=0 (SONIC preference), bootcount=2 -> try SONIC_A.
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_rollback_zero_bootcount_two_boots_sonic_a_decrements() {
  echo "Running test_rollback_zero_bootcount_two_boots_sonic_a_decrements..."
  setup
  warmboot=1
  install=0
  bootcount=2
  rollback=0

  compute_state

  assert_eq "${default}" "SONIC_A"
  assert_eq "${bootcount}" "1"
}

#######################################
# Check: rollback=0 (SONIC preference), bootcount=1 -> try SONIC_B.
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_rollback_zero_bootcount_one_boots_sonic_b_decrements() {
  echo "Running test_rollback_zero_bootcount_one_boots_sonic_b_decrements..."
  setup
  warmboot=1
  install=0
  bootcount=1
  rollback=0

  compute_state

  assert_eq "${default}" "SONIC_B"
  assert_eq "${bootcount}" "0"
}

#######################################
# Check: rollback unset defaults to 0 (SONIC preference).
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_rollback_unset_defaults_zero_boots_sonic_a() {
  echo "Running test_rollback_unset_defaults_zero_boots_sonic_a..."
  setup
  warmboot=1
  install=0
  bootcount=2
  unset rollback

  compute_state

  assert_eq "${default}" "SONIC_A"
  assert_eq "${bootcount}" "1"
}

#######################################
# Check: rollback=1 (SONIC_B preference), bootcount=2 -> try SONIC_B.
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_rollback_one_bootcount_two_boots_sonic_b_decrements() {
  echo "Running test_rollback_one_bootcount_two_boots_sonic_b_decrements..."
  setup
  warmboot=1
  install=0
  bootcount=2
  rollback=1

  compute_state

  assert_eq "${default}" "SONIC_B"
  assert_eq "${bootcount}" "1"
}

#######################################
# Check: rollback=1 (SONIC_B preference), bootcount=1 -> try SONIC_A.
# Globals:
#   default, warmboot, install, bootcount, rollback
# Arguments:
#   None
#######################################
test_rollback_one_bootcount_one_boots_sonic_a_decrements() {
  echo "Running test_rollback_one_bootcount_one_boots_sonic_a_decrements..."
  setup
  warmboot=1
  install=0
  bootcount=1
  rollback=1

  compute_state

  assert_eq "${default}" "SONIC_A"
  assert_eq "${bootcount}" "0"
}

#######################################
# Main test suite entry point.
# Globals:
#   None
# Arguments:
#   None
#######################################
main() {
  test_warmboot_unset_defaults_sonie
  test_warmboot_zero_defaults_sonie
  test_warmboot_zero_bootcount_two_defaults_sonie
  test_warmboot_zero_bootcount_one_defaults_sonie
  test_install_set_defaults_sonie
  test_bootcount_zero_defaults_sonie
  test_rollback_zero_bootcount_two_boots_sonic_a_decrements
  test_rollback_zero_bootcount_one_boots_sonic_b_decrements
  test_rollback_unset_defaults_zero_boots_sonic_a
  test_rollback_one_bootcount_two_boots_sonic_b_decrements
  test_rollback_one_bootcount_one_boots_sonic_a_decrements
  echo "All tests passed!"
}

main "$@"

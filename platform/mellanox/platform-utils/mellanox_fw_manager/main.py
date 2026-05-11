#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Command Line Interface for Mellanox Firmware Manager

Provides a command-line interface for firmware upgrade operations.
"""

import sys
import logging
import logging.handlers
import fcntl
import subprocess
from contextlib import contextmanager
import click
from .fw_manager import create_firmware_manager, EXIT_SUCCESS, EXIT_FAILURE, FW_UPGRADE_IS_REQUIRED, FirmwareManagerError, FirmwareUpgradeError, FirmwareUpgradePartialError
from .firmware_coordinator import FirmwareCoordinator
from .platform_utils import run_command

logger = None


def setup_logging(verbose: bool = False, nosyslog: bool = False):
    """Setup global logging configuration with syslog."""
    global logger

    level = logging.DEBUG if verbose else logging.INFO

    logger = logging.getLogger()
    logger.setLevel(level)

    logger.handlers.clear()

    if not nosyslog:
        syslog_handler = logging.handlers.SysLogHandler(address='/dev/log')
        syslog_handler.setLevel(level)

        formatter = logging.Formatter('mellanox-fw-manager[%(process)d]: %(levelname)s - %(message)s')
        syslog_handler.setFormatter(formatter)

        logger.addHandler(syslog_handler)

    if verbose or nosyslog:
        console_handler = logging.StreamHandler()
        console_handler.setLevel(level)
        console_formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        console_handler.setFormatter(console_formatter)
        logger.addHandler(console_handler)


def _exit_if_qemu() -> None:
    """
    Exit if running on QEMU/SimX platform (matches shell script ExitIfQEMU).

    Raises:
        SystemExit: If QEMU platform is detected
    """
    try:
        cmd = ['lspci', '-vvv']
        result = run_command(cmd, logger=logger, capture_output=True, text=True)
        if result.returncode == 0 and 'SimX' in result.stdout:
            print("No FW upgrade for SimX platform")
            sys.exit(EXIT_SUCCESS)
    except Exception as e:
        logger.warning(f"Failed to check for QEMU platform: {e}")


@contextmanager
def _lock_state_change():
    """
    Context manager for file locking (matches shell script LockStateChange/UnlockStateChange).
    """
    lock_file = "/tmp/mlxfwmanager-lock"
    lock_fd = None

    try:
        logger.info(f"Locking {lock_file} from CLI")
        lock_fd = open(lock_file, 'w')
        fcntl.flock(lock_fd.fileno(), fcntl.LOCK_EX)
        logger.info(f"Locked {lock_file} from CLI")
        yield lock_fd
    except Exception as e:
        logger.error(f"Failed to acquire lock: {e}")
        raise FirmwareManagerError(f"Failed to acquire lock: {e}")
    finally:
        if lock_fd:
            try:
                logger.info(f"Unlocking {lock_file} from CLI")
                fcntl.flock(lock_fd.fileno(), fcntl.LOCK_UN)
                lock_fd.close()
            except Exception as e:
                logger.warning(f"Failed to unlock: {e}")


def handle_status(asic_id_arg: str, verbose: bool, upgrade: bool) -> int:
    """
    Handle firmware status check - reports if firmware version matches.

    Args:
        asic_id_arg: ASIC ID, "all", or "__flag__" (when used as flag)
        verbose: Enable verbose logging
        upgrade: Use firmware from next SONiC image

    Returns:
        Exit code
    """
    try:
        fw_coordinator = FirmwareCoordinator(verbose=verbose, from_image=upgrade)
        num_asics = fw_coordinator.get_asic_count()

        # Handle flag mode for single-ASIC systems
        if asic_id_arg == '__flag__':
            if num_asics == 1:
                asic_id_arg = '0'
            else:
                print(f"Error: Multi-ASIC system detected ({num_asics} ASICs).")
                print("Usage: mlnx-fw-manager --status <ASIC_ID|all>")
                print("Example: mlnx-fw-manager --status all")
                return EXIT_FAILURE

        # Determine which ASICs to check
        if asic_id_arg.lower() == 'all':
            managers_to_check = fw_coordinator.managers
            print(f"Firmware Status Report ({num_asics} ASIC(s)):")
        else:
            try:
                asic_id = int(asic_id_arg)
                if asic_id >= num_asics:
                    print(f"Error: ASIC {asic_id} not found. System has {num_asics} ASIC(s).")
                    return EXIT_FAILURE
                managers_to_check = [fw_coordinator.managers[asic_id]]
                print(f"Firmware Status Report (ASIC {asic_id}):")
            except ValueError:
                print(f"Error: Invalid ASIC ID '{asic_id_arg}'. Use a number or 'all'.")
                return EXIT_FAILURE

        upgrade_needed_asics = []
        up_to_date_asics = []
        error_asics = []

        for manager in managers_to_check:
            try:
                is_needed = manager.is_upgrade_required()
                if is_needed:
                    upgrade_needed_asics.append(manager.asic_index)
                    print(f"ASIC {manager.asic_index}: Upgrade needed")
                else:
                    up_to_date_asics.append(manager.asic_index)
                    print(f"ASIC {manager.asic_index}: Up to date")
            except Exception as e:
                error_asics.append(manager.asic_index)
                print(f"ASIC {manager.asic_index}: Error checking status - {e}")

        print(f"Summary: {len(up_to_date_asics)} up to date, "
              f"{len(upgrade_needed_asics)} need upgrade, "
              f"{len(error_asics)} errors")

        if upgrade_needed_asics or error_asics:
            return FW_UPGRADE_IS_REQUIRED
        return EXIT_SUCCESS

    except Exception as e:
        print(f"Status check failed: {e}")
        return EXIT_FAILURE


def handle_reset(verbose: bool) -> int:
    """
    Handle firmware configuration reset.

    Args:
        verbose: Enable verbose logging

    Returns:
        Exit code
    """
    try:
        fw_coordinator = FirmwareCoordinator(verbose=verbose)
        fw_coordinator.reset_firmware_config()
        print("Firmware configuration reset completed successfully.")
        return EXIT_SUCCESS
    except Exception as e:
        print(f"Firmware reset failed: {e}")
        return EXIT_FAILURE


def handle_dry_run(verbose: bool, upgrade: bool) -> int:
    """
    Handle dry-run (check if firmware upgrade is required).

    Args:
        verbose: Enable verbose logging
        upgrade: Use firmware from next SONiC image

    Returns:
        Exit code
    """
    try:
        fw_coordinator = FirmwareCoordinator(verbose=verbose, from_image=upgrade)
        if fw_coordinator.check_upgrade_required():
            print("Firmware upgrade is required.")
            logger.info(f"Dry-run check: upgrade required, exiting with {FW_UPGRADE_IS_REQUIRED}")
            return FW_UPGRADE_IS_REQUIRED
        else:
            print("Firmware is up to date.")
            logger.info(f"Dry-run check: up to date, exiting with {EXIT_SUCCESS}")
            return EXIT_SUCCESS
    except Exception as e:
        print(f"Firmware check failed: {e}")
        logger.error(f"Dry-run check failed, exiting with {EXIT_FAILURE}")
        return EXIT_FAILURE


def handle_upgrade(verbose: bool, upgrade: bool, clear_semaphore: bool) -> int:
    """
    Handle firmware upgrade operation.

    Args:
        verbose: Enable verbose logging
        upgrade: Use firmware from next SONiC image
        clear_semaphore: Clear hardware semaphore before upgrade

    Returns:
        Exit code
    """
    try:
        fw_coordinator = FirmwareCoordinator(
            verbose=verbose,
            from_image=upgrade,
            clear_semaphore=clear_semaphore
        )

        if not fw_coordinator.check_upgrade_required():
            print("Firmware is up to date.")
            logger.info(f"Upgrade check: up to date, exiting with {EXIT_SUCCESS}")
            return EXIT_SUCCESS
        else:
            fw_coordinator.upgrade_firmware()
            print("Firmware upgrade completed successfully.")
            logger.info(f"Upgrade completed successfully, exiting with {EXIT_SUCCESS}")
            return EXIT_SUCCESS

    except FirmwareUpgradeError as e:
        print(f"Firmware upgrade failed: {e}")
        return EXIT_FAILURE
    except FirmwareUpgradePartialError as e:
        print(f"Firmware upgrade partially failed: {e}")
        return EXIT_SUCCESS
    except Exception as e:
        print(f"Firmware upgrade failed: {e}")
        return EXIT_FAILURE


@click.command(context_settings={'help_option_names': ['-h', '--help'], 'max_content_width': 120})
@click.option('-u', '--upgrade', is_flag=True,
              help='Upgrade ASIC firmware using next boot image (useful after SONiC-To-SONiC update)')
@click.option('-v', '--verbose', is_flag=True,
              help='Verbose mode (enabled when -u|--upgrade)')
@click.option('-d', '--dry-run', is_flag=True,
              help='Compare the FW versions without installation. Return code "0" means the FW is up-to-date, return code "10" means an upgrade is required, otherwise an error is detected.')
@click.option('-c', '--clear-semaphore', is_flag=True,
              help='Clear hw semaphore before firmware upgrade')
@click.option('-r', '--reset', is_flag=True,
              help='Reset firmware configuration (NVIDIA BlueField platform only)')
@click.option('--nosyslog', is_flag=True,
              help='Disable syslog and log to console only')
@click.option('--status', 'status', type=str, default=None, is_flag=False, flag_value='__flag__', metavar='[ASIC_ID|all]',
              help='Show firmware version status. Single-ASIC: use as flag. Multi-ASIC: specify ASIC ID or "all".')
def main(upgrade, verbose, dry_run, clear_semaphore, reset, nosyslog, status):
    """
    Mellanox Firmware Manager

    Usage: ./mlnx-fw-upgrade [OPTIONS]

    OPTIONS:
      -u, --upgrade  Upgrade ASIC firmware using next boot image (useful after SONiC-To-SONiC update)
      -v, --verbose  Verbose mode (enabled when -u|--upgrade)
      -d, --dry-run  Compare the FW versions without installation. Return code "0" means the FW is up-to-date, return code "10" means an upgrade is required, otherwise an error is detected.
      -c, --clear-semaphore   Clear hw semaphore before firmware upgrade
      -r, --reset    Reset firmware configuration (NVIDIA BlueField platform only)
      --status [ASIC_ID|all]  Show firmware version status
                              Single-ASIC: use as flag (no argument)
                              Multi-ASIC: specify ASIC ID or "all"
      --nosyslog     Disable syslog and log to console only
      -h, --help     Print help

    Examples:
      ./mlnx-fw-upgrade --verbose
      ./mlnx-fw-upgrade --upgrade
      ./mlnx-fw-upgrade --reset
      ./mlnx-fw-upgrade --clear-semaphore --upgrade

      # Single-ASIC system:
      ./mlnx-fw-upgrade --status              # Check firmware status

      # Multi-ASIC system:
      ./mlnx-fw-upgrade --status all          # Check all ASICs
      ./mlnx-fw-upgrade --status 0            # Check ASIC 0
      ./mlnx-fw-upgrade --status 0 --upgrade  # Check ASIC 0 against next image

      ./mlnx-fw-upgrade --help
    """
    setup_logging(verbose, nosyslog)

    logger.info("Mellanox Firmware Manager started")

    _exit_if_qemu()

    if status is not None:
        exit_code = handle_status(status, verbose, upgrade)
        logger.info(f"Mellanox Firmware Manager finished with exit code {exit_code}")
        sys.exit(exit_code)

    with _lock_state_change():
        if reset:
            exit_code = handle_reset(verbose)
        elif dry_run:
            exit_code = handle_dry_run(verbose, upgrade)
        else:
            exit_code = handle_upgrade(verbose, upgrade, clear_semaphore)

    logger.info(f"Mellanox Firmware Manager finished with exit code {exit_code}")
    sys.exit(exit_code)


if __name__ == '__main__':
    main()

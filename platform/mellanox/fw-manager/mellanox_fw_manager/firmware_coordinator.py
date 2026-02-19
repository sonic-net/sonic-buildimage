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
Firmware coordinator for managing multiple ASIC firmware operations.

Handles coordination of firmware upgrades across multiple ASICs,
including process management, error handling, and SONiC image integration.
"""
import os
import logging
import subprocess
from typing import List
from multiprocessing import Queue

from .firmware_base import (
    FirmwareManagerError, FirmwareUpgradeError, FirmwareUpgradePartialError,
    UpgradeStatus, UpgradeStatusType
)
from .platform_utils import _detect_platform, run_command
from .asic_manager import AsicManager
from .bluefield_manager import BluefieldFirmwareManager


class FirmwareCoordinator:
    """Main coordinator class that manages multiple ASIC firmware processes."""

    def __init__(self, verbose: bool = False, from_image: bool = False, clear_semaphore: bool = False):
        """
        Initialize the firmware coordinator.

        Args:
            verbose: Enable verbose logging
            from_image: If True, upgrade from next SONiC image (matches shell script --upgrade)
            clear_semaphore: Clear hardware semaphore before upgrade
        """
        self.verbose = verbose
        self.from_image = from_image
        self.clear_semaphore = clear_semaphore
        self.logger = logging.getLogger()

        try:
            platform = _detect_platform()
        except Exception as e:
            self.logger.error(f"Failed to detect platform: {e}")
            raise

        self.asic_manager = AsicManager(platform)

        from .fw_manager import create_firmware_manager
        num_asics = self.get_asic_count()
        pci_ids = self.get_asic_pci_ids()
        asic_type = self.asic_manager.get_asic_type()
        self.managers = []

        if self.from_image:
            try:
                self.fw_bin_path = self._get_image_firmware_path()
            except Exception as e:
                self.logger.error(f"Failed to get firmware path from image: {e}")
                raise
        else:
            self.fw_bin_path = "/etc/mlnx"

        for asic_index in range(num_asics):
            pci_id = pci_ids[asic_index] if asic_index < len(pci_ids) else None

            try:
                manager = create_firmware_manager(
                    asic_index=asic_index,
                    pci_id=pci_id,
                    asic_type=asic_type,
                    fw_bin_path=self.fw_bin_path,
                    verbose=self.verbose,
                    clear_semaphore=self.clear_semaphore
                )

                self.managers.append(manager)

            except FirmwareManagerError as e:
                self.logger.error(f"Failed to initialize ASIC {asic_index}: {e}")
                raise

        self.logger.info(f"Initialized firmware coordinator with {len(self.managers)} ASIC(s) and image from {self.fw_bin_path}")

    def upgrade_firmware(self) -> None:
        """
        Upgrade firmware for all ASICs using separate processes.

        Raises:
            FirmwareUpgradeError: If all ASIC upgrades fail
            FirmwareUpgradePartialError: If some ASIC upgrades fail
        """
        num_asics = len(self.managers)

        self.logger.info(f"Starting firmware upgrade for {num_asics} ASIC(s) from {self.fw_bin_path}")

        processes = self.managers.copy()

        status_queue = Queue()

        for manager in processes:
            manager.status_queue = status_queue
            manager.start()

        results = {}
        success_count = 0
        failure_count = 0
        timeout_failures = set()

        # Wait for all processes with timeout (10 minutes per ASIC)
        # Firmware upgrades typically take 1-3 minutes, so 10 minutes provides a safe margin
        timeout_per_asic = 600  # seconds
        for process in processes:
            process.join(timeout=timeout_per_asic)
            if process.is_alive():
                self.logger.error(f"ASIC {process.asic_index} firmware upgrade timed out after {timeout_per_asic} seconds")
                process.terminate()
                process.join(timeout=10)  # Give it 10 seconds to terminate
                if process.is_alive():
                    self.logger.error(f"Force killing stuck process for ASIC {process.asic_index}")
                    process.kill()
                    process.join()
                timeout_failures.add(process.asic_index)
                failure_count += 1

        while not status_queue.empty():
            try:
                status_data = status_queue.get_nowait()
                status = UpgradeStatus.from_dict(status_data)
                results[status.asic_index] = status

                if status.status == UpgradeStatusType.SUCCESS:
                    success_count += 1
                elif status.status == UpgradeStatusType.FAILED or status.status == UpgradeStatusType.ERROR:
                    failure_count += 1
            except Exception as e:
                self.logger.error(f"Error processing status queue: {e}")
                break

        # If a process completed but didn't report status (not in results and not timeout), treat as failure
        for process in processes:
            if process.asic_index not in results and process.asic_index not in timeout_failures:
                self.logger.error(f"ASIC {process.asic_index} completed but did not report status, treating as failure")
                failure_count += 1

        total_failures = failure_count
        total_asics = num_asics

        self.logger.info(f"Upgrade results: {success_count} successful, {total_failures} failed")

        if total_failures == total_asics:
            self.logger.error("All ASIC upgrades failed")
            raise FirmwareUpgradeError("All ASIC upgrades failed")
        elif total_failures > 0:
            self.logger.warning(f"Some ASIC upgrades failed ({total_failures}/{total_asics})")
            raise FirmwareUpgradePartialError(f"Some ASIC upgrades failed ({total_failures}/{total_asics})")
        else:
            self.logger.info("All ASIC upgrades completed successfully")

    def check_upgrade_required(self) -> bool:
        """
        Check if firmware upgrade is required for any ASIC (dry-run functionality).

        Returns:
            True if upgrade is required, False otherwise
        """
        self.logger.info(f"Checking if firmware upgrade is required from {self.fw_bin_path}")

        upgrade_needed = False
        for manager in self.managers:
            try:
                if manager.is_upgrade_required():
                    self.logger.info(f"Firmware upgrade is required for ASIC {manager.asic_index}")
                    upgrade_needed = True
            except Exception as e:
                self.logger.error(f"Error checking upgrade status for ASIC {manager.asic_index}: {e}")
                upgrade_needed = True

        if upgrade_needed:
            self.logger.info("Firmware upgrade is required")
        else:
            self.logger.info("Firmware is up to date")

        return upgrade_needed

    def _get_image_firmware_path(self) -> str:
        """
        Get firmware path from next SONiC image (matches shell script UpgradeFWFromImage).

        Returns:
            Path to firmware binaries from the next SONiC image
        """
        cmd = ['sonic-installer', 'list']
        result = run_command(cmd, logger=self.logger, capture_output=True, text=True)
        if result.returncode != 0:
            error_msg = f"Failed to get SONiC image list: {result.stderr if result.stderr else 'Unknown error'}"
            self.logger.error(error_msg)
            raise FirmwareUpgradeError(error_msg)

        next_image = None
        current_image = None
        for line in result.stdout.split('\n'):
            if line.startswith('Next: '):
                next_image = line.split('Next: ')[1].strip()
            elif line.startswith('Current: '):
                current_image = line.split('Current: ')[1].strip()

        if not next_image:
            raise FirmwareUpgradeError("No next SONiC image found")

        platform_fw_path = f"/host/image-{next_image.replace('SONiC-OS-', '')}/platform/fw/asic/"

        if os.path.exists(platform_fw_path):
            self.logger.info(f"Using FW binaries from {platform_fw_path}")
            return platform_fw_path
        else:
            fs_path = f"/host/image-{next_image.replace('SONiC-OS-', '')}/fs.squashfs"
            fs_mountpoint = f"/tmp/image-{next_image.replace('SONiC-OS-', '')}-fs"
            fw_bin_path = f"{fs_mountpoint}/etc/mlnx/"

            self.logger.info(f"Using FW binaries from {fw_bin_path}")

            try:
                os.makedirs(fs_mountpoint, exist_ok=True)
            except Exception as e:
                error_msg = f"Failed to create mount point directory {fs_mountpoint}: {e}"
                self.logger.error(error_msg)
                raise FirmwareUpgradeError(error_msg)

            cmd = ['mount', '-t', 'squashfs', fs_path, fs_mountpoint]
            mount_result = run_command(cmd, logger=self.logger)
            if mount_result.returncode != 0:
                error_msg = f"Failed to mount {fs_path}: {mount_result.stderr if mount_result.stderr else 'Unknown error'}"
                self.logger.error(error_msg)
                raise FirmwareUpgradeError(error_msg)

            return fw_bin_path

    def get_asic_count(self) -> int:
        """Get the number of ASICs in the system."""
        return self.asic_manager.get_asic_count()

    def get_asic_pci_ids(self) -> List[str]:
        """Get PCI IDs for all ASICs."""
        return self.asic_manager.get_asic_pci_ids()

    def reset_firmware_config(self) -> None:
        """
        Reset firmware configuration for all BlueField ASICs.

        Raises:
            FirmwareManagerError: If reset fails or no BlueField ASICs found
        """
        bluefield_managers = [m for m in self.managers if isinstance(m, BluefieldFirmwareManager)]

        if not bluefield_managers:
            raise FirmwareManagerError("No BlueField ASICs found for reset operation")

        self.logger.info(f"Resetting firmware configuration for {len(bluefield_managers)} BlueField ASIC(s)")

        success_count = 0
        failure_count = 0

        for manager in bluefield_managers:
            try:
                if manager.reset_firmware_config():
                    success_count += 1
                    self.logger.info(f"Reset successful for ASIC {manager.asic_index}")
                else:
                    failure_count += 1
                    self.logger.error(f"Reset failed for ASIC {manager.asic_index}")
            except Exception as e:
                failure_count += 1
                self.logger.error(f"Reset failed for ASIC {manager.asic_index}: {e}")

        if failure_count == len(bluefield_managers):
            raise FirmwareManagerError("All BlueField ASIC resets failed")
        elif failure_count > 0:
            self.logger.warning(f"Some BlueField ASIC resets failed ({failure_count}/{len(bluefield_managers)})")

        self.logger.info(f"Reset completed: {success_count} successful, {failure_count} failed")

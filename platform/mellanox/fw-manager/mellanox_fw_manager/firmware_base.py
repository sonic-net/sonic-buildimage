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
Base classes, exceptions, and enums for firmware management.

Contains the abstract base class and common data structures used across
all firmware manager implementations.
"""
from __future__ import annotations

import os
import logging
import subprocess
import time
import xml.etree.ElementTree as ET
from abc import ABC, abstractmethod
from enum import Enum
from typing import List, Dict, Optional, Tuple, Any
from multiprocessing import Process, Queue
from .platform_utils import run_command

EXIT_SUCCESS = 0
EXIT_FAILURE = 1
FW_ALREADY_UPDATED_FAILURE = 2
FW_UPGRADE_IS_REQUIRED = 10


class FirmwareManagerError(Exception):
    """Base exception for firmware manager errors."""
    pass


class FirmwareUpgradeError(Exception):
    """Exception raised when firmware upgrade fails."""
    pass


class FirmwareUpgradePartialError(Exception):
    """Exception raised when some ASIC upgrades fail."""
    pass


class UpgradeStatusType(Enum):
    """Enum for firmware upgrade status types."""
    SUCCESS = "success"
    FAILED = "failed"
    ERROR = "error"


class UpgradeStatus:
    """Status information for firmware upgrade operations."""

    def __init__(self, asic_index: int, status: UpgradeStatusType, message: str = "",
                 current_version: str = "", available_version: str = "", pci_id: str = None):
        self.asic_index = asic_index
        self.status = status
        self.message = message
        self.current_version = current_version
        self.available_version = available_version
        self.pci_id = pci_id
        self.timestamp = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for inter-process communication."""
        return {
            'asic_index': self.asic_index,
            'status': self.status.value,
            'message': self.message,
            'current_version': self.current_version,
            'available_version': self.available_version,
            'pci_id': self.pci_id,
            'timestamp': self.timestamp
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'UpgradeStatus':
        """Create from dictionary for inter-process communication."""
        status = cls(
            asic_index=data['asic_index'],
            status=UpgradeStatusType(data['status']),
            message=data['message'],
            current_version=data['current_version'],
            available_version=data['available_version'],
            pci_id=data.get('pci_id')
        )
        status.timestamp = data.get('timestamp')
        return status


class FirmwareManagerBase(Process):
    """Base firmware manager class for a single ASIC, running in its own process."""

    def __init__(self, asic_index: int, pci_id: str,
                 fw_bin_path: str = None, verbose: bool = False, clear_semaphore: bool = False,
                 asic_type: str = None, status_queue: Queue = None):
        """
        Initialize the firmware manager for a single ASIC.

        Args:
            asic_index: Index of the ASIC this manager handles
            pci_id: PCI ID of the ASIC
            fw_bin_path: Path to firmware binaries
            verbose: Enable verbose logging
            clear_semaphore: Clear hardware semaphore before upgrade
            asic_type: Type of ASIC (e.g., 'spc1', 'bf3')
            status_queue: Queue for reporting status to parent process
        """
        super().__init__()
        self.asic_index = asic_index
        self.pci_id = pci_id
        self.fw_bin_path = fw_bin_path
        self.verbose = verbose
        self.should_clear_semaphore = clear_semaphore
        self.status_queue = status_queue

        self.asic_type = asic_type
        self.fw_file = None
        self.current_version = None
        self.available_version = None

        self.logger = logging.getLogger()

        self._initialize_asic()

    def _initialize_asic(self) -> None:
        """Initialize ASIC-specific information during object creation."""
        try:
            if not self.asic_type:
                raise FirmwareManagerError(f"ASIC type not provided for ASIC {self.asic_index}")

            # Validate ASIC type is supported by this manager
            if self.asic_type not in self.get_asic_type_map().values():
                raise FirmwareManagerError(f"Unsupported ASIC type '{self.asic_type}' for ASIC {self.asic_index}")

            self.fw_file = self._get_firmware_file_path(self.asic_type, self.fw_bin_path)
            if not self.fw_file or not os.path.exists(self.fw_file):
                raise FirmwareManagerError(f"Firmware file not found: {self.fw_file}")

            self.current_version, self.available_version = self._get_firmware_versions()
            if not self.current_version or not self.available_version:
                raise FirmwareManagerError(f"Could not retrieve firmware versions for ASIC {self.asic_index}")

            self.logger.info(f"ASIC {self.asic_index} initialized: {self.asic_type}, "
                             f"current: {self.current_version}, available: {self.available_version}")

        except Exception as e:
            self.logger.error(f"Initialization failed: {str(e)}")
            raise

    def run(self):
        """Main process entry point."""
        try:
            if not self.is_upgrade_required():
                self._report_status(UpgradeStatusType.SUCCESS, f"ASIC {self.asic_index} firmware is up to date")
                return

            if self.should_clear_semaphore:
                self.logger.info(f"Clearing semaphore before upgrade for ASIC {self.asic_index}")
                if not self.clear_semaphore():
                    self.logger.warning(f"Failed to clear semaphore for ASIC {self.asic_index}, continuing with upgrade")

            if self.run_firmware_update():
                self._report_status(UpgradeStatusType.SUCCESS, f"ASIC {self.asic_index} upgrade completed")
            else:
                self._report_status(UpgradeStatusType.FAILED, f"ASIC {self.asic_index} upgrade failed")

        except Exception as e:
            self._report_status(UpgradeStatusType.ERROR, f"Unexpected error: {str(e)}")

    def is_upgrade_required(self) -> bool:
        """
        Check if firmware upgrade is required for this ASIC.

        Returns:
            True if upgrade is required, False otherwise
        """
        return self.current_version != self.available_version

    def _report_status(self, status: UpgradeStatusType, message: str = ""):
        """Report status to parent process."""
        if self.status_queue:
            upgrade_status = UpgradeStatus(
                asic_index=self.asic_index,
                status=status,
                message=message,
                current_version=self.current_version or "",
                available_version=self.available_version or "",
                pci_id=self.pci_id
            )
            self.status_queue.put(upgrade_status.to_dict())

    def get_asic_type(self) -> Optional[str]:
        """Get the ASIC type for this platform."""
        return self.asic_type

    @classmethod
    @abstractmethod
    def get_asic_type_map(cls) -> Dict[str, str]:
        """
        Get the PCI ID to ASIC type mapping for this manager.

        Returns:
            Dictionary mapping PCI IDs (format 'vendor:device') to ASIC types
        """
        pass

    def get_firmware_filename(self) -> Optional[str]:
        """
        Get the firmware filename for this manager's ASIC type.

        Returns:
            Firmware filename or None if not found
        """
        if not self.asic_type:
            return None
        return f'fw-{self.asic_type}.mfa'

    @abstractmethod
    def _get_mst_device_type(self) -> str:
        """Get MST device type based on ASIC type (matches shell script)."""
        pass

    @abstractmethod
    def run_firmware_update(self) -> bool:
        """Run the actual firmware update command."""
        pass

    def _get_firmware_file_path(self, asic_type: str, fw_bin_path: str = None) -> Optional[str]:
        """Get the firmware file path for the given ASIC type."""
        fw_filename = self.get_firmware_filename()
        if not fw_filename:
            return None

        if fw_bin_path:
            return os.path.join(fw_bin_path, fw_filename)
        else:
            return os.path.join('/etc/mlnx', fw_filename)

    def _query_firmware_versions(self) -> Tuple[str, str]:
        """
        Query firmware versions without retry logic.

        Returns:
            Tuple of (current_version, available_version)

        Raises:
            FirmwareManagerError: If query fails or versions cannot be retrieved
        """
        cmd = ['mlxfwmanager', '--query-format', 'XML', '-d', self.pci_id]
        result = self._run_command(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise FirmwareManagerError("Query returned non-zero exit code")

        root = ET.fromstring(result.stdout)
        current_version = root.find('.//Device/Versions/FW').get('current')
        psid = root.find('.//Device').get('psid')

        if not current_version or not psid:
            raise FirmwareManagerError("Version or PSID not found in response")

        available_version = self._get_available_firmware_version(psid)
        return current_version, available_version

    def _get_firmware_versions(self) -> Tuple[Optional[str], Optional[str]]:
        """Get current and available firmware versions with retry logic."""
        query_retry_count = 0
        query_retry_count_max = 10

        while query_retry_count < query_retry_count_max:
            try:
                return self._query_firmware_versions()
            except Exception as e:
                if query_retry_count >= query_retry_count_max - 1:
                    self.logger.error(f"Failed to get firmware versions after {query_retry_count_max} attempts: {str(e)}")
                    return None, None
                self.logger.info(f"Unable to get firmware versions (attempt {query_retry_count + 1}/{query_retry_count_max}): {str(e)}, retrying...")

            query_retry_count += 1
            time.sleep(1)

        return None, None

    @abstractmethod
    def _get_available_firmware_version(self, psid: str) -> Optional[str]:
        """Get available firmware version using platform-specific method."""
        pass

    def _get_env(self) -> dict:
        """
        Get environment variables including MFT diagnosis flags if verbose mode is enabled.

        Returns:
            Dictionary of environment variables
        """
        env = os.environ.copy()
        if self.verbose:
            env['FLASH_ACCESS_DEBUG'] = '1'
            env['FW_COMPS_DEBUG'] = '1'
        return env

    def _run_command(self, cmd: List[str], **kwargs) -> subprocess.CompletedProcess:
        """
        Execute a subprocess command with automatic logging.

        Args:
            cmd: Command and arguments as a list
            **kwargs: Additional arguments to pass to subprocess.run

        Returns:
            CompletedProcess instance from subprocess.run
        """
        return run_command(cmd, logger=self.logger, **kwargs)

    def clear_semaphore(self) -> bool:
        """
        Clear hardware semaphore for this ASIC.

        Returns:
            True if successful, False otherwise
        """
        try:
            self.logger.info(f"Clearing semaphore for device {self.pci_id}")
            cmd = ['/usr/bin/flint', '-d', self.pci_id, '--clear_semaphore']
            result = self._run_command(cmd, capture_output=True, text=True, check=True)
            self.logger.info(f"Successfully cleared semaphore for {self.pci_id}")
            return True
        except subprocess.CalledProcessError as e:
            self.logger.error(f"Failed to clear semaphore for {self.pci_id}: {e.stderr}")
            return False
        except Exception as e:
            self.logger.error(f"Failed to clear semaphore for ASIC {self.asic_index}: {e}")
            return False

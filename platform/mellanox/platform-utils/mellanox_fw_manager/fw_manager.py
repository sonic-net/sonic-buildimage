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
Firmware Manager

Main entry point and factory functions for Mellanox ASIC firmware management.
"""

from typing import Optional
from .firmware_base import FirmwareManagerBase, FirmwareManagerError
from .spectrum_manager import SpectrumFirmwareManager
from .bluefield_manager import BluefieldFirmwareManager

from .firmware_base import (
    EXIT_SUCCESS, EXIT_FAILURE, FW_ALREADY_UPDATED_FAILURE, FW_UPGRADE_IS_REQUIRED,
    FirmwareManagerError, FirmwareUpgradeError, FirmwareUpgradePartialError,
    UpgradeStatusType, UpgradeStatus
)
from .firmware_coordinator import FirmwareCoordinator
from .platform_utils import _detect_platform


def create_firmware_manager(asic_index: int, pci_id: Optional[str], asic_type: str,
                            fw_bin_path: str = None, verbose: bool = False, clear_semaphore: bool = False) -> FirmwareManagerBase:
    """
    Factory function to create the appropriate firmware manager.

    Args:
        asic_index: Index of the ASIC
        pci_id: PCI ID of the ASIC
        fw_bin_path: Path to firmware binaries
        verbose: Enable verbose logging
        clear_semaphore: Clear hardware semaphore before upgrade

    Returns:
        Appropriate firmware manager instance
    """
    if asic_type == 'BF3':
        manager = BluefieldFirmwareManager(
            asic_index=asic_index,
            pci_id=pci_id,
            fw_bin_path=fw_bin_path,
            verbose=verbose,
            clear_semaphore=clear_semaphore,
            asic_type=asic_type
        )
    else:
        manager = SpectrumFirmwareManager(
            asic_index=asic_index,
            pci_id=pci_id,
            fw_bin_path=fw_bin_path,
            verbose=verbose,
            clear_semaphore=clear_semaphore,
            asic_type=asic_type
        )

    return manager

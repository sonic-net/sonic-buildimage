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
Spectrum ASIC firmware manager implementation.

Handles firmware operations specific to Spectrum ASICs using mlxfwmanager.
"""

import subprocess
from typing import Dict, Optional
from .firmware_base import FirmwareManagerBase, FW_ALREADY_UPDATED_FAILURE


class SpectrumFirmwareManager(FirmwareManagerBase):
    """Firmware manager for Spectrum ASICs."""

    # PCI ID to ASIC type mapping for Spectrum devices
    ASIC_TYPE_MAP = {
        '15b3:cb84': 'SPC',
        '15b3:cf6c': 'SPC2',
        '15b3:cf70': 'SPC3',
        '15b3:cf80': 'SPC4',
        '15b3:cf82': 'SPC5',
    }

    @classmethod
    def get_asic_type_map(cls) -> Dict[str, str]:
        """
        Get the PCI ID to ASIC type mapping for Spectrum devices.

        Returns:
            Dictionary mapping PCI IDs to ASIC types
        """
        return cls.ASIC_TYPE_MAP

    def _get_mst_device_type(self) -> str:
        """Get MST device type for Spectrum ASICs."""
        return "Spectrum"

    def _get_available_firmware_version(self, psid: str) -> Optional[str]:
        """Get available firmware version for Spectrum ASICs using mlxfwmanager."""
        try:
            cmd = ['mlxfwmanager', '--list-content', '-i', self.fw_file, '-d', self.pci_id]
            result = self._run_command(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                self.logger.error(f"Failed to get available firmware version for Spectrum ASICs using mlxfwmanager: {result.returncode}")
                return None

            lines = result.stdout.strip().split('\n')
            for line in lines:
                if psid in line:
                    parts = line.split()
                    if len(parts) >= 4:
                        return parts[3]

            self.logger.error(f"No firmware version found for PSID {psid} in mlxfwmanager output")
            return None
        except Exception as e:
            self.logger.error(f"Failed to get available firmware version for Spectrum ASICs using mlxfwmanager: {e}")
            return None

    def run_firmware_update(self) -> bool:
        """Run the actual firmware update command for Spectrum ASICs."""
        try:
            cmd = ['mlxfwmanager', '-u', '-f', '-y', '-d', self.pci_id, '-i', self.fw_file]
            env = self._get_env()

            result = self._run_command(cmd, env=env, capture_output=True, text=True)

            if result.returncode == FW_ALREADY_UPDATED_FAILURE:
                self.logger.info("FW reactivation is required. Reactivating and updating FW ...")
                reactivate_cmd = ['flint', '-d', self.pci_id, 'ir']
                reactivate_result = self._run_command(reactivate_cmd, capture_output=True, text=True)
                if reactivate_result.returncode != 0:
                    self.logger.warning(f"FW reactivation failed with return code {reactivate_result.returncode}: {reactivate_result.stderr}")

                result = self._run_command(cmd, env=env, capture_output=True, text=True)

            if result.returncode != 0:
                self.logger.error(f"Failed to update firmware for Spectrum ASICs with return code {result.returncode}: {result.stderr}")
                return False

            return True
        except Exception as e:
            self.logger.error(f"Failed to run firmware update for Spectrum ASICs: {e}")
            return False

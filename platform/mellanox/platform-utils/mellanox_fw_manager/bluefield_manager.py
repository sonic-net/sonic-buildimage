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
BlueField ASIC firmware manager implementation.

Handles firmware operations specific to BlueField ASICs using flint and mlxconfig.
"""

import subprocess
from typing import Dict, Optional
from .firmware_base import FirmwareManagerBase


class BluefieldFirmwareManager(FirmwareManagerBase):
    """Firmware manager for BlueField ASICs."""

    ASIC_TYPE_MAP = {
        '15b3:a2dc': 'BF3'
    }

    @classmethod
    def get_asic_type_map(cls) -> Dict[str, str]:
        """
        Get the PCI ID to ASIC type mapping for BlueField devices.

        Returns:
            Dictionary mapping PCI IDs to ASIC types
        """
        return cls.ASIC_TYPE_MAP

    def _get_mst_device_type(self) -> str:
        """Get MST device type for BlueField ASICs"""
        return "BlueField3"

    def _get_available_firmware_version(self, psid: str) -> Optional[str]:
        """Get available firmware version for BlueField ASICs using flint."""
        try:
            cmd = ['flint', '-i', self.fw_file, '--psid', psid, 'query']
            result = self._run_command(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                self.logger.error(f"Failed to get available firmware version for BlueField ASICs using flint: {result.returncode}")
                return None

            lines = result.stdout.strip().split('\n')
            for line in lines:
                if 'FW Version:' in line:
                    return line.split('FW Version:')[1].strip()

            self.logger.error(f"No FW Version found in flint output for PSID {psid}")
            return None
        except Exception as e:
            self.logger.error(f"Failed to get available firmware version for BlueField ASICs using flint: {e}")
            return None

    def run_firmware_update(self) -> bool:
        """Run the actual firmware update command for BlueField ASICs."""
        try:
            env = self._get_env()

            reactivate_cmd = ['flint', '-d', self.pci_id, 'ir']
            result = self._run_command(reactivate_cmd, capture_output=True, text=True)
            if result.returncode != 0:
                self.logger.warning(f"FW reactivation failed with return code {result.returncode}: {result.stderr}")

            burn_cmd = ['flint', '-d', self.pci_id, '-i', self.fw_file, 'burn']
            result = self._run_command(burn_cmd, env=env, capture_output=True, text=True)
            if result.returncode != 0:
                self.logger.error(f"Failed to burn firmware for BlueField ASICs using flint with return code {result.returncode}: {result.stderr}")
                return False

            return self.reset_firmware_config()
        except Exception as e:
            self.logger.error(f"Failed to run firmware update for BlueField ASICs: {e}")
            return False

    def reset_firmware_config(self) -> bool:
        """Reset firmware configuration (BlueField specific)."""
        try:
            cmd = ['mlxconfig', '-d', self.pci_id, '-y', 'r']
            result = self._run_command(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                self.logger.error(f"Failed to reset firmware config with return code {result.returncode}: {result.stderr}")
                return False
            return True
        except Exception as e:
            self.logger.error(f"Reset firmware config failed: {e}")
            return False

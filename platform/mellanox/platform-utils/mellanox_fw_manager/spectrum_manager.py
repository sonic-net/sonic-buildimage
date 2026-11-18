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

import re
import subprocess
from typing import Dict, Optional
from . import vmod
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
        '15b3:cf84': 'SPC6',
    }

    _ANSI_ESCAPE_RE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]|[\x08\r]')
    _MLXFW_ERR_RE = re.compile(
        r'^\s*Fail\s*:|^\s*-[EW]-|MCC\s+(?:ERROR|FAIL)|\bFAIL(?:ED|URE|S)?\b|\bERROR\b',
        re.IGNORECASE,
    )
    _MLXFW_NOISE_RE = re.compile(
        r'^\s*(user/.*\.(?:cpp|c|h):\d+|\[FLASH_ACCESS_DEBUG\]|-D-|controlFsm\s*:)',
        re.IGNORECASE,
    )

    # OE firmware versions, read on CPO platforms only.
    vmod_supported = False
    vmod_versions_read = False
    hw_vmod_versions = None
    mfa_vmod_version = None
    vmod_skip_reason = "ASIC firmware check failed"

    def _initialize_asic(self) -> None:
        super()._initialize_asic()
        self.vmod_supported = vmod.is_cpo_platform()

    def _read_vmod_versions(self) -> None:
        # Read on first use rather than at init: when the ASIC firmware already
        # differs a burn is needed anyway, and the MFA extract costs ~5 s.
        if self.vmod_versions_read:
            return
        try:
            self.hw_vmod_versions = vmod.get_hw_versions(self.pci_id)
            self.mfa_vmod_version = vmod.get_mfa_version(self.fw_file, self.pci_id)
        except Exception as e:
            # Left unknown, so it is reported and does not request a burn.
            self.logger.error(f"ASIC {self.asic_index}: failed to read the OE firmware version: {e}")
        self.vmod_versions_read = True
        self.logger.info(f"ASIC {self.asic_index} OE firmware: {self._vmod_versions_text()}")

    def _vmod_versions_text(self) -> str:
        hw = self.hw_vmod_versions
        if not hw:
            current = 'N/A'
        elif hw.boot == hw.running:
            current = hw.running
        else:
            current = f"{hw.running}, {hw.boot} after the next ASIC reset"
        available = vmod.to_device_version(self.mfa_vmod_version or '') or 'N/A'
        return f"current {current}, available {available}"

    def is_upgrade_required(self) -> bool:
        """
        ASIC firmware differs, or (on CPO platforms) the OE firmware differs.

        The OE firmware is a component of the same MFA, so either way the
        one MFA burn upgrades both.
        """
        # Why the OE firmware was not read, for the status report.
        if super().is_upgrade_required():
            self.vmod_skip_reason = "ASIC firmware upgrade needed"
            return True
        return self.is_vmod_upgrade_required()

    def is_vmod_upgrade_required(self) -> bool:
        if not self.vmod_supported:
            return False
        self._read_vmod_versions()
        if not self.hw_vmod_versions or not vmod.to_device_version(self.mfa_vmod_version or ''):
            # Unknown is not a reason to burn: that would re-burn on every boot.
            self.logger.error(f"ASIC {self.asic_index}: unable to read the OE firmware version "
                              f"({self._vmod_versions_text()})")
            return False
        # The boot bank decides: a burned image waiting for the ASIC reset is
        # up to date, an old image left in the other bank is not.
        if vmod.versions_match(self.hw_vmod_versions.boot, self.mfa_vmod_version):
            return False
        self.logger.info(f"ASIC {self.asic_index} OE firmware differs ({self._vmod_versions_text()}); "
                         f"firmware upgrade is required")
        return True

    def get_vmod_status(self) -> Optional[str]:
        """The OE part of the status report; call after is_upgrade_required()."""
        if not self.vmod_supported:
            return None
        if not self.vmod_versions_read:
            # The ASIC decided first: the OE firmware was not read.
            return f"not checked ({self.vmod_skip_reason})"
        hw = self.hw_vmod_versions
        available = vmod.to_device_version(self.mfa_vmod_version or '')
        if not hw or not available:
            return f"unknown, unable to read the version ({self._vmod_versions_text()})"
        if vmod.versions_match(hw.boot, self.mfa_vmod_version):
            if vmod.versions_match(hw.running, self.mfa_vmod_version):
                return f"up to date ({available})"
            return f"up to date ({available}, active after the next ASIC reset)"
        return f"upgrade needed ({self._vmod_versions_text()})"

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

    @classmethod
    def _mlxfwmanager_detail(cls, result: subprocess.CompletedProcess, max_len: int = 1024) -> str:
        """Extract a syslog-safe error string from an mlxfwmanager result."""
        stderr = cls._ANSI_ESCAPE_RE.sub('', (result.stderr or "")).strip()
        if stderr:
            return stderr

        stdout = cls._ANSI_ESCAPE_RE.sub('', (result.stdout or "")).strip()
        if not stdout:
            return ""

        seen, picked = set(), []
        for line in stdout.splitlines():
            line = line.strip()
            if not line or line in seen or cls._MLXFW_NOISE_RE.search(line):
                continue
            if cls._MLXFW_ERR_RE.search(line):
                seen.add(line)
                picked.append(line)

        if picked:
            detail = " | ".join(picked)
            return detail if len(detail) <= max_len else detail[:max_len].rstrip() + " ...[truncated]"
        return stdout if len(stdout) <= max_len else "...[truncated]" + stdout[-max_len:]

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
                self.logger.error(f"Failed to update firmware for Spectrum ASICs with return code {result.returncode}: {self._mlxfwmanager_detail(result)}")
                return False

            return True
        except Exception as e:
            self.logger.error(f"Failed to run firmware update for Spectrum ASICs: {e}")
            return False

#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
ASIC Manager

Handles ASIC detection and configuration parsing for both single and multi-ASIC systems.
"""

import os
import logging
from typing import List, Optional

class AsicManager:
    """Manages ASIC detection and configuration."""

    def __init__(self, platform: str):
        """
        Initialize the ASIC manager.

        Args:
            platform: SONiC platform name
        """
        self.platform = platform
        self.logger = logging.getLogger()

        self.asic_conf_file = f"/usr/share/sonic/device/{platform}/asic.conf"

        self._load_asic_data()

    def _load_asic_data(self):
        """Load and cache ASIC configuration data from file."""
        self.logger.info(f"Loading ASIC configuration from {self.asic_conf_file}")

        if not os.path.exists(self.asic_conf_file):
            self.logger.info(f"No asic.conf file found, assuming single ASIC system ({self.asic_conf_file})")
            self._asic_count = 1
            self._pci_ids = []
            return

        try:
            asic_count = 1
            pci_ids = []

            with open(self.asic_conf_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line.startswith('NUM_ASIC='):
                        asic_count = int(line.split('=')[1].strip())
                    elif line.startswith('DEV_ID_ASIC_'):
                        parts = line.split('=')
                        if len(parts) == 2:
                            asic_index = int(parts[0].split('_')[-1])
                            pci_id = parts[1].strip()
                            pci_ids.append((asic_index, pci_id))

            pci_ids.sort(key=lambda x: x[0])
            result = [pci_id for _, pci_id in pci_ids]

            while len(result) < asic_count:
                result.append("unknown")

            self._asic_count = asic_count
            self._pci_ids = result[:asic_count]

            self.logger.info(f"Loaded ASIC data: {asic_count} ASICs, PCI IDs: {self._pci_ids}")

        except (ValueError, FileNotFoundError, IOError) as e:
            self.logger.warning(f"Could not parse ASIC configuration from {self.asic_conf_file}: {e}")
            self._asic_count = 1
            self._pci_ids = []

    def get_asic_count(self) -> int:
        """
        Get the number of ASICs in the system.

        Returns:
            Number of ASICs (1 for single ASIC, >1 for multi-ASIC)
        """
        return self._asic_count

    def get_asic_pci_ids(self) -> List[str]:
        """
        Get PCI IDs for all ASICs.

        Returns:
            List of PCI IDs for each ASIC
        """
        return self._pci_ids.copy()

    def get_asic_pci_id_by_index(self, asic_index: int) -> str:
        """
        Get PCI ID for a specific ASIC index.

        Args:
            asic_index: Index of the ASIC

        Returns:
            PCI ID for the ASIC
        """
        if asic_index < len(self._pci_ids):
            return self._pci_ids[asic_index]
        return "unknown"

    def is_multi_asic(self) -> bool:
        """
        Check if this is a multi-ASIC system.

        Returns:
            True if multi-ASIC, False if single ASIC
        """
        return self._asic_count > 1

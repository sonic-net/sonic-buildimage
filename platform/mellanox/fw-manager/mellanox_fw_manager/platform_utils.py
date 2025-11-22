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
Platform detection utilities for firmware management.

Contains functions to detect SONiC platform and ASIC types.
"""

import os
import logging
import subprocess
import time
from typing import Optional, List

def run_command(cmd: List[str], logger: logging.Logger = None, **kwargs) -> subprocess.CompletedProcess:
    """
    Execute a subprocess command with automatic logging.

    Args:
        cmd: Command and arguments as a list
        logger: Logger instance to use for logging. If None, uses root logger
        **kwargs: Additional arguments to pass to subprocess.run

    Returns:
        CompletedProcess instance from subprocess.run
    """
    if logger is None:
        logger = logging.getLogger()

    try:
        logger.info(f"Executing: {' '.join(cmd)}")
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        logger.error(f"Failed to execute command {' '.join(cmd)}: {e}")
        raise

def _detect_platform() -> Optional[str]:
    """Detect SONiC platform name from /host/machine.conf onie_platform variable."""
    try:
        conf_file = "/host/machine.conf"
        if not os.path.exists(conf_file):
            logging.error(f"Platform configuration file not found: {conf_file}")
            return None

        with open(conf_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('onie_platform='):
                    onie_platform = line.split('=')[1].strip()
                    logging.info(f"Detected platform: {onie_platform}")
                    return onie_platform

        logging.error(f"Could not find onie_platform variable in {conf_file}")
        return None

    except Exception as e:
        logging.error(f"Platform detection failed: {e}")
        return None

def _detect_platform_from_asic_conf(platform: str) -> Optional[str]:
    """Detect SONiC platform name from asic.conf file."""
    try:
        asic_conf_file = f"/usr/share/sonic/device/{platform}/asic.conf"
        if not os.path.exists(asic_conf_file):
            logging.error(f"ASIC configuration file not found: {asic_conf_file}")
            return None

        return asic_conf_file
    except Exception as e:
        logging.error(f"Failed to detect platform from asic.conf: {e}")
        return None

def _is_multi_asic(asic_conf_file: str) -> bool:
    """Check if the platform is a multi-ASIC platform."""
    try:
        with open(asic_conf_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('NUM_ASIC='):
                    return int(line.split('=')[1].strip()) > 1
        logging.warning(f"NUM_ASIC not found in {asic_conf_file}, assuming single ASIC")
        return False
    except Exception as e:
        logging.error(f"Multi-ASIC detection failed: {e}")
        return False

def _detect_asic_type() -> Optional[str]:
    """Detect ASIC type using lspci command with retry logic."""
    # Import here to avoid circular dependencies at module load time
    from .spectrum_manager import SpectrumFirmwareManager
    from .bluefield_manager import BluefieldFirmwareManager

    try:
        query_retry_count = 0
        query_retry_count_max = 10

        while query_retry_count < query_retry_count_max:
            cmd = ['lspci', '-n']
            if query_retry_count == 0:
                logging.info(f"Executing: {' '.join(cmd)}")
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode == 0:
                break

            time.sleep(1)
            query_retry_count += 1

        if result.returncode != 0:
            logging.error(f"Failed to execute lspci command after {query_retry_count_max} retries")
            return None

        pci_output = result.stdout

        # Check Spectrum ASICs first
        for vendor_product, asic_type in SpectrumFirmwareManager.get_asic_type_map().items():
            if vendor_product in pci_output:
                return asic_type

        # Check BlueField ASICs last (Smart Switch requirement)
        for vendor_product, asic_type in BluefieldFirmwareManager.get_asic_type_map().items():
            if vendor_product in pci_output:
                return asic_type

        logging.warning("No known Mellanox ASIC type found in lspci output")
        return None
    except Exception as e:
        logging.error(f"ASIC type detection failed: {e}")
        return None

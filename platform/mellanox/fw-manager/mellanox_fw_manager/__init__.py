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
Mellanox Firmware Manager Package

This package provides firmware management capabilities for Mellanox ASICs,
supporting both single and multi-ASIC systems with unified error handling
and parallel upgrade capabilities.
"""
__version__ = "1.0.0"
__author__ = "SONiC Team"

from .fw_manager import (
    create_firmware_manager,
    FirmwareManagerError,
    FirmwareUpgradeError,
    FirmwareUpgradePartialError
)
from .firmware_coordinator import FirmwareCoordinator
from .firmware_base import FirmwareManagerBase
from .spectrum_manager import SpectrumFirmwareManager
from .bluefield_manager import BluefieldFirmwareManager
from .asic_manager import AsicManager
__all__ = [
    'FirmwareManagerBase',
    'SpectrumFirmwareManager',
    'BluefieldFirmwareManager',
    'create_firmware_manager',
    'FirmwareManagerError',
    'FirmwareUpgradeError',
    'FirmwareUpgradePartialError',
    'FirmwareCoordinator',
    'AsicManager'
]

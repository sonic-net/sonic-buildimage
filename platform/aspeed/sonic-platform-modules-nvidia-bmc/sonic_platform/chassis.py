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
# chassis.py
#
# Chassis implementation for NVIDIA AST2700 BMC
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common.device_info import get_platform_json_data
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Chassis(ChassisBase):
    """
    Platform-specific Chassis class for NVIDIA AST2700 BMC
    """

    def __init__(self):
        """
        Initialize NVIDIA chassis with hardware-specific configuration
        """
        super().__init__()

        self._platform_data = get_platform_json_data() or {}

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a reboot cause
            string from ChassisBase and the second is an optional description.
        """
        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_name(self):
        """
        Retrieves the name of the chassis

        Returns:
            String containing the name of the chassis
        """
        return self._platform_data.get('chassis', {}).get('name', 'N/A')

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis

        Returns:
            String containing the model number of the chassis
        """
        return self._platform_data.get('chassis', {}).get('model', 'N/A')

    def get_serial(self):
        """
        Retrieves the serial number of the chassis

        Returns:
            String containing the serial number of the chassis
        """
        return "N/A"

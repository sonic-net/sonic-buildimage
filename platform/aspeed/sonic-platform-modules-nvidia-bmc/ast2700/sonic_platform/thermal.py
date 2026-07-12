#
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

import os

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

from sonic_platform.utils import hw_mgmt_path, read_sysfs_float


class ThermalBMC(ThermalBase):
    """
    Thermal sensor class for the NVIDIA AST2700 BMC platform.

    Reads the BMC ambient temperature exposed by hw-management under
    `/var/run/hw-management/thermal/`.
    """

    HW_MGMT_ROOT = hw_mgmt_path("thermal")
    TEMP_SCALE = 1000.0

    def __init__(self):
        ThermalBase.__init__(self)

        self.name = "BMC Ambient"
        self.input_path = os.path.join(self.HW_MGMT_ROOT, "bmc_temp_input")
        self.max_path = os.path.join(self.HW_MGMT_ROOT, "bmc_temp")

    def _read_scaled(self, path):
        value = read_sysfs_float(path)
        if value is None:
            return None
        return value / self.TEMP_SCALE

    def get_name(self):
        """
        Retrieve the name of the thermal sensor.

        Returns:
            str: The name of the thermal sensor.
        """
        return self.name

    def get_presence(self):
        """
        Retrieve the presence of the thermal sensor.

        Returns:
            bool: Always True.
        """
        return True

    def get_model(self):
        """
        Retrieve the model of the thermal sensor.

        Returns:
            str: Model identifier for the sensor, or None if not available.
        """
        return None

    def get_serial(self):
        """
        Retrieve the serial number of the thermal sensor.

        Returns:
            str: Serial number of the sensor, or None if not available.
        """
        return None

    def get_status(self):
        """
        Retrieve the operational status of the thermal sensor.

        Returns:
            bool: True if the sensor is present and reporting a valid temperature.
        """
        return self.get_temperature() is not None

    def get_temperature(self):
        """
        Retrieve the current temperature reading from the thermal sensor.

        Returns:
            The current temperature in Celsius to the nearest thousandth of a degree (e.g. 30.125),
            or None if unavailable.
        """
        return self._read_scaled(self.input_path)

    def get_high_threshold(self):
        """
        Retrieve the high threshold temperature of the thermal sensor.

        Returns:
            The high threshold temperature in Celsius, or None if unavailable.
        """
        return self._read_scaled(self.max_path)

    def get_low_threshold(self):
        """
        Retrieve the low threshold temperature of the thermal sensor.

        Returns:
            None -- the BMC ambient sensor has no low threshold.
        """
        return None

    def get_high_critical_threshold(self):
        """
        Retrieve the high critical threshold temperature of the thermal sensor.

        Returns:
            None -- the BMC ambient sensor has no critical threshold.
        """
        return None

    def get_low_critical_threshold(self):
        """
        Retrieve the low critical threshold temperature of the thermal sensor.

        Returns:
            None -- the BMC ambient sensor has no critical threshold.
        """
        return None

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

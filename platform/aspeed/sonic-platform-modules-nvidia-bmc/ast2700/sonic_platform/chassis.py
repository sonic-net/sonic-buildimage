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

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.thermal import ThermalBMC
    from sonic_platform.switch_host_module import SwitchHostModule
    from sonic_platform.eeprom import EepromBMC
    from sonic_platform.reboot_cause import RebootCause
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Chassis(ChassisBase):
    """
    Platform-specific Chassis class for the NVIDIA AST2700 BMC.
    """

    def __init__(self):
        super().__init__()

        self._thermal_list = [ThermalBMC()]
        self._switch_host_module = SwitchHostModule()
        self._module_list = [self._switch_host_module]
        self._liquid_cooling = None
        self._eeprom = EepromBMC()
        self._reboot_cause = RebootCause()

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a reboot cause
            string from ChassisBase and the second is an optional description.
        """
        return self._reboot_cause.get_reboot_cause()

    def get_eeprom(self):
        """
        Retrieves the system eeprom device on this chassis

        Returns:
            An object representing the hardware eeprom device.
        """
        return self._eeprom

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._eeprom.get_product_name()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_part_number()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial_number()

    def get_switch_host_serial(self):
        """
        Retrieves the hardware serial number for the switch host module

        Returns:
            A string containing the hardware serial number for the switch host module.
        """
        return self._switch_host_module.get_serial()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_system_eeprom_info()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.get_revision()

    def is_bmc(self):
        """
        Retrieves whether this chassis is a BMC

        Returns:
            bool: True if this chassis is a BMC, False otherwise
        """
        return True

    def is_liquid_cooled(self):
        """
        Retrieves whether this chassis is liquid cooled

        Returns:
            bool: True if this chassis is liquid cooled, False otherwise
        """
        platform_data = device_info.get_platform_json_data() or {}
        return bool(platform_data.get("chassis", {}).get("liquid_cooled", False))

    def get_liquid_cooling(self):
        """
        Lazily construct and return the platform's LiquidCooling object.

        Returns:
            LiquidCooling: the (cached) liquid cooling object for this chassis.
        """
        if self._liquid_cooling is None:
            from sonic_platform.liquid_cooling import LiquidCooling
            self._liquid_cooling = LiquidCooling()
        return self._liquid_cooling

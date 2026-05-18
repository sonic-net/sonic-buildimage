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

from unittest.mock import MagicMock, patch

import pytest

from sonic_platform_base.chassis_base import ChassisBase


# `EepromBMC` / `EepromSystem` run an `ipmi-fru` subprocess from `__init__` via
# `super().__init__`. We patch them at the point they're imported into the chassis
# and switch_host_module modules so that tests never touch the real hardware.
PATCH_EEPROM_BMC = "sonic_platform.chassis.EepromBMC"
PATCH_THERMAL_BMC = "sonic_platform.chassis.ThermalBMC"
PATCH_SWITCH_HOST = "sonic_platform.chassis.SwitchHostModule"
PATCH_REBOOT_CAUSE = "sonic_platform.chassis.RebootCause"
PATCH_PLATFORM_JSON = "sonic_platform.chassis.device_info.get_platform_json_data"


@pytest.fixture
def chassis():
    """Build a `Chassis` instance with all collaborators mocked out."""
    with patch(PATCH_EEPROM_BMC) as eeprom_cls, \
         patch(PATCH_THERMAL_BMC) as thermal_cls, \
         patch(PATCH_SWITCH_HOST) as module_cls, \
         patch(PATCH_REBOOT_CAUSE) as reboot_cls:
        eeprom_cls.return_value = MagicMock(name="EepromBMC")
        thermal_cls.return_value = MagicMock(name="ThermalBMC")
        module_cls.return_value = MagicMock(name="SwitchHostModule")
        reboot_cls.return_value = MagicMock(name="RebootCause")

        from sonic_platform.chassis import Chassis
        yield Chassis()


class TestChassis:
    """Mock-based coverage for `sonic_platform.chassis.Chassis`."""

    def test_is_chassis_base_subclass(self, chassis):
        assert isinstance(chassis, ChassisBase)

    def test_is_bmc_true(self, chassis):
        assert chassis.is_bmc() is True

    def test_is_liquid_cooled_true_from_platform_json(self, chassis):
        with patch(PATCH_PLATFORM_JSON, return_value={"chassis": {"liquid_cooled": True}}):
            assert chassis.is_liquid_cooled() is True

    def test_is_liquid_cooled_false_from_platform_json(self, chassis):
        with patch(PATCH_PLATFORM_JSON, return_value={"chassis": {"liquid_cooled": False}}):
            assert chassis.is_liquid_cooled() is False

    def test_is_liquid_cooled_defaults_false_when_field_missing(self, chassis):
        with patch(PATCH_PLATFORM_JSON, return_value={"chassis": {}}):
            assert chassis.is_liquid_cooled() is False

    def test_is_liquid_cooled_defaults_false_when_chassis_missing(self, chassis):
        with patch(PATCH_PLATFORM_JSON, return_value={}):
            assert chassis.is_liquid_cooled() is False

    def test_is_liquid_cooled_false_when_platform_json_unavailable(self, chassis):
        with patch(PATCH_PLATFORM_JSON, return_value=None):
            assert chassis.is_liquid_cooled() is False

    def test_default_collaborator_lists(self, chassis):
        assert len(chassis._thermal_list) == 1
        assert len(chassis._module_list) == 1
        assert chassis._liquid_cooling is None

    def test_get_eeprom_returns_internal_eeprom(self, chassis):
        assert chassis.get_eeprom() is chassis._eeprom

    def test_get_name_delegates_to_eeprom(self, chassis):
        chassis._eeprom.get_product_name.return_value = "Nvidia-BMC-AST2700"
        assert chassis.get_name() == "Nvidia-BMC-AST2700"
        chassis._eeprom.get_product_name.assert_called_once_with()

    def test_get_model_delegates_to_eeprom(self, chassis):
        chassis._eeprom.get_part_number.return_value = "MBF-AST2700-001"
        assert chassis.get_model() == "MBF-AST2700-001"
        chassis._eeprom.get_part_number.assert_called_once_with()

    def test_get_base_mac_delegates_to_eeprom(self, chassis):
        chassis._eeprom.get_base_mac.return_value = "aa:bb:cc:dd:ee:ff"
        assert chassis.get_base_mac() == "aa:bb:cc:dd:ee:ff"

    def test_get_serial_delegates_to_eeprom(self, chassis):
        chassis._eeprom.get_serial_number.return_value = "SN1234567"
        assert chassis.get_serial() == "SN1234567"

    def test_get_system_eeprom_info_delegates(self, chassis):
        info = {hex(0x21): "product", hex(0x23): "SN0"}
        chassis._eeprom.get_system_eeprom_info.return_value = info
        assert chassis.get_system_eeprom_info() == info

    def test_get_revision_delegates_to_eeprom(self, chassis):
        chassis._eeprom.get_revision.return_value = "A0"
        assert chassis.get_revision() == "A0"

    def test_get_reboot_cause_delegates(self, chassis):
        expected = (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, "")
        chassis._reboot_cause.get_reboot_cause.return_value = expected
        assert chassis.get_reboot_cause() == expected
        chassis._reboot_cause.get_reboot_cause.assert_called_once_with()

    def test_get_switch_host_serial_delegates(self, chassis):
        expected = "SWHOST-SN-0001"
        chassis._switch_host_module.get_serial.return_value = expected
        assert chassis.get_switch_host_serial() == expected
        chassis._switch_host_module.get_serial.assert_called_once_with()

    def test_get_liquid_cooling_lazily_initializes(self, chassis):
        assert chassis._liquid_cooling is None
        with patch("sonic_platform.liquid_cooling.LiquidCooling") as lc_cls:
            lc_instance = MagicMock(name="LiquidCooling")
            lc_cls.return_value = lc_instance
            lc = chassis.get_liquid_cooling()
        lc_cls.assert_called_once_with()
        assert lc is lc_instance
        assert chassis._liquid_cooling is lc_instance

    def test_get_liquid_cooling_is_cached(self, chassis):
        with patch("sonic_platform.liquid_cooling.LiquidCooling") as lc_cls:
            lc_instance = MagicMock(name="LiquidCooling")
            lc_cls.return_value = lc_instance
            first = chassis.get_liquid_cooling()
            second = chassis.get_liquid_cooling()
        lc_cls.assert_called_once_with()
        assert first is lc_instance
        assert second is lc_instance

    def test_thermal_and_module_lists_accessible_via_base_api(self, chassis):
        assert chassis.get_all_thermals() == chassis._thermal_list
        assert chassis.get_all_modules() == chassis._module_list

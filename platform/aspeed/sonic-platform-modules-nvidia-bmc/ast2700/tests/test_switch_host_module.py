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

from types import SimpleNamespace
from unittest.mock import MagicMock, mock_open, patch

import pytest

from sonic_platform_base.module_base import ModuleBase
from sonic_platform import switch_host_module as shm_mod
from sonic_platform.switch_host_module import (
    PWR_DOWN_SYSFS,
    SwitchHostModule,
)


@pytest.fixture
def module():
    """Return a `SwitchHostModule` with a mocked `EepromSystem` backend."""
    with patch("sonic_platform.switch_host_module.EepromSystem") as eeprom_cls:
        eeprom_cls.return_value = MagicMock(name="EepromSystem")
        yield SwitchHostModule()


class TestSwitchHostModuleBasics:

    def test_inherits_from_module_base(self, module):
        assert isinstance(module, ModuleBase)

    def test_static_identity(self, module):
        assert module.get_name() == SwitchHostModule.NAME == "SWITCH-HOST"
        assert module.get_description() == SwitchHostModule.DESCRIPTION
        assert module.get_type() == SwitchHostModule.MODULE_TYPE_SWITCH_HOST
        assert module.get_slot() == 0
        assert module.is_replaceable() is False

    def test_custom_slot(self):
        with patch("sonic_platform.switch_host_module.EepromSystem"):
            mod = SwitchHostModule(slot=3)
        assert mod.get_slot() == 3

    def test_get_serial_delegates_to_eeprom(self, module):
        module._eeprom.get_serial_number.return_value = "SN-SH-1234"
        assert module.get_serial() == "SN-SH-1234"
        module._eeprom.get_serial_number.assert_called_once_with()


class TestSwitchHostOperStatus:

    def test_online_when_pwr_down_zero(self, module):
        with patch("builtins.open", mock_open(read_data="0\n")):
            assert module.get_oper_status() == ModuleBase.MODULE_STATUS_ONLINE

    def test_offline_when_pwr_down_one(self, module):
        with patch("builtins.open", mock_open(read_data="1\n")):
            assert module.get_oper_status() == ModuleBase.MODULE_STATUS_OFFLINE

    def test_empty_when_pwr_down_unreadable(self, module):
        with patch("builtins.open", side_effect=OSError("missing")):
            assert module.get_oper_status() == ModuleBase.MODULE_STATUS_EMPTY

    def test_empty_on_bad_content(self, module):
        with patch("builtins.open", mock_open(read_data="not-a-number")):
            assert module.get_oper_status() == ModuleBase.MODULE_STATUS_EMPTY

    def test_online_when_pwr_down_hex_zero(self, module):
        # The parser uses `int(raw, 0)` so hex literals must work too.
        with patch("builtins.open", mock_open(read_data="0x0")):
            assert module.get_oper_status() == ModuleBase.MODULE_STATUS_ONLINE


class TestSwitchHostPowerControl:

    def _mock_run(self, returncode=0, stderr=b""):
        return SimpleNamespace(returncode=returncode, stdout=b"", stderr=stderr)

    def test_set_admin_state_up_invokes_power_on(self, module):
        with patch("sonic_platform.switch_host_module.subprocess.run", return_value=self._mock_run()) as run:
            assert module.set_admin_state(True) is True
        args = run.call_args[0][0]
        assert args[0] == shm_mod.HW_MGMT_POWERCTRL
        assert args[1] == "power_on"

    def test_set_admin_state_down_invokes_power_off(self, module):
        with patch("sonic_platform.switch_host_module.subprocess.run", return_value=self._mock_run()) as run:
            assert module.set_admin_state(False) is True
        assert run.call_args[0][0][1] == "power_off"

    def test_do_power_cycle_invokes_reset(self, module):
        with patch("sonic_platform.switch_host_module.subprocess.run", return_value=self._mock_run()) as run:
            assert module.do_power_cycle() is True
        args = run.call_args[0][0]
        assert args[0] == shm_mod.HW_MGMT_POWERCTRL
        assert args[1] == "reset"

    def test_do_power_cycle_returns_false_on_non_zero_exit(self, module):
        fake = self._mock_run(returncode=7, stderr=b"boom")
        with patch("sonic_platform.switch_host_module.subprocess.run", return_value=fake):
            assert module.do_power_cycle() is False

    def test_do_power_cycle_returns_false_on_oserror(self, module):
        with patch("sonic_platform.switch_host_module.subprocess.run", side_effect=OSError("no such binary")):
            assert module.do_power_cycle() is False

    def test_reboot_delegates_to_do_power_cycle(self, module):
        with patch.object(module, "do_power_cycle", return_value=True) as do_cycle:
            assert module.reboot() is True
        do_cycle.assert_called_once_with()

    def test_reboot_unsupported_type_returns_false_without_running(self, module):
        with patch.object(module, "do_power_cycle") as do_cycle:
            assert module.reboot(ModuleBase.MODULE_REBOOT_CPU_COMPLEX) is False
        do_cycle.assert_not_called()

    def test_powerctrl_returns_false_on_non_zero_exit(self, module):
        fake = self._mock_run(returncode=7, stderr=b"boom")
        with patch("sonic_platform.switch_host_module.subprocess.run", return_value=fake):
            assert module.set_admin_state(True) is False

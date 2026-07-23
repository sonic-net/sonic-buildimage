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
import subprocess

try:
    from sonic_platform_base.module_base import ModuleBase
    from sonic_platform.eeprom import EepromSystem
    from sonic_platform.utils import hw_mgmt_path, read_sysfs_int
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()

# hw-management helper script used for Switch-Host power actions.
HW_MGMT_POWERCTRL = "/usr/bin/hw-management-bmc-powerctrl.sh"

# Maximum time (in seconds) to wait for `hw-management-bmc-powerctrl.sh` to complete.
HW_MGMT_POWERCTRL_TIMEOUT = 10

# `pwr_down` sysfs values:
#   0 -> Switch-Host powered on
#   1 -> Switch-Host powered off
PWR_DOWN_SYSFS = hw_mgmt_path("system", "pwr_down")

# hw-management-bmc-powerctrl.sh actions.
_POWERCTRL_POWER_ON = "power_on"
_POWERCTRL_POWER_OFF = "power_off"
_POWERCTRL_RESET = "reset"


class SwitchHostModule(ModuleBase):
    """
    Module object representing the Switch-Host on the NVIDIA AST2700 BMC.
    """

    NAME = "SWITCH-HOST"
    DESCRIPTION = "NVIDIA Switch-Host"

    def __init__(self, slot=0):
        super().__init__()
        self._slot = slot
        self._eeprom = EepromSystem()

    @staticmethod
    def _run_powerctrl(action):
        """
        Invoke `hw-management-bmc-powerctrl.sh <action>`.

        A non-zero exit or spawn failure is logged as ERROR together with the subprocess stderr.

        Args:
            action: one of `_POWERCTRL_POWER_ON`, `_POWERCTRL_POWER_OFF`, `_POWERCTRL_RESET`.

        Returns:
            bool: True on a zero exit, False otherwise.
        """
        try:
            result = subprocess.run(
                [HW_MGMT_POWERCTRL, action],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=HW_MGMT_POWERCTRL_TIMEOUT,
            )
        except subprocess.TimeoutExpired as exc:
            logger.log_error(f"{HW_MGMT_POWERCTRL} {action} timed out: {exc}")
            return False
        except OSError as exc:
            logger.log_error(f"{HW_MGMT_POWERCTRL} {action} failed to execute: {exc}")
            return False

        if result.returncode != 0:
            stderr = (result.stderr or b"").decode(errors="replace").strip()
            logger.log_error(f"{HW_MGMT_POWERCTRL} {action} exited with {result.returncode}: {stderr}")
            return False
        return True

    def get_name(self):
        return self.NAME

    def get_description(self):
        return self.DESCRIPTION

    def get_type(self):
        return self.MODULE_TYPE_SWITCH_HOST

    def get_slot(self):
        return self._slot

    def get_presence(self):
        return True

    def is_replaceable(self):
        return False

    def set_admin_state(self, up):
        action = _POWERCTRL_POWER_ON if up else _POWERCTRL_POWER_OFF
        return self._run_powerctrl(action)

    def do_power_cycle(self):
        return self._run_powerctrl(_POWERCTRL_RESET)

    def reboot(self, reboot_type=ModuleBase.MODULE_REBOOT_DEFAULT):
        if reboot_type != ModuleBase.MODULE_REBOOT_DEFAULT:
            logger.log_error(
                f"Switch-Host reboot type {reboot_type!r} is not supported; "
                "only MODULE_REBOOT_DEFAULT is supported"
            )
            return False
        return self.do_power_cycle()

    def get_oper_status(self):
        value = read_sysfs_int(PWR_DOWN_SYSFS)
        if value == 0:
            return ModuleBase.MODULE_STATUS_ONLINE
        if value == 1:
            return ModuleBase.MODULE_STATUS_OFFLINE
        return ModuleBase.MODULE_STATUS_EMPTY

    def get_serial(self):
        return self._eeprom.get_serial_number()

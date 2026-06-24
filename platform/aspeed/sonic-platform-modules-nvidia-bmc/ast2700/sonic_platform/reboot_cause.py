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

from sonic_platform.utils import hwmgmt_flag_is_set
from sonic_py_common.logger import Logger

try:
    from sonic_platform_base.chassis_base import ChassisBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()


class RebootCause:
    """
    Reboot-cause provider for the NVIDIA AST2700 BMC.

    Reads hw-management reset attribute files under `/var/run/hw-management/bmc/`.
    """

    _BMC_RESET_DIR = "bmc"

    def __init__(self):
        self._reboot_major_cause_dict = {
            'reset_security_watchdog2': ChassisBase.REBOOT_CAUSE_WATCHDOG,
            'reset_cpu': ChassisBase.REBOOT_CAUSE_HARDWARE_CPU,
            'reset_other': ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER,
            'reset_power_on': ChassisBase.REBOOT_CAUSE_POWER_LOSS,
        }
        self._watchdog_file = 'reset_watchdog'
        self._power_on_file = 'reset_power_on'

    def get_reboot_cause(self):
        """
        Returns:
            tuple(str, str): `(cause, description)`. `cause` is one of the
            `ChassisBase.REBOOT_CAUSE_*` constants.
        """
        if (hwmgmt_flag_is_set(self._BMC_RESET_DIR, self._watchdog_file)
                and hwmgmt_flag_is_set(self._BMC_RESET_DIR, self._power_on_file)):
            logger.log_info(
                f"Reboot cause: {self._watchdog_file} and {self._power_on_file} (software watchdog)"
            )
            return ChassisBase.REBOOT_CAUSE_NON_HARDWARE, ''

        for reset_file, reset_cause in self._reboot_major_cause_dict.items():
            if hwmgmt_flag_is_set(self._BMC_RESET_DIR, reset_file):
                logger.log_info(f"Hardware reboot cause: {reset_file}")
                return reset_cause, ''

        logger.log_info("No hardware reboot cause found")
        return ChassisBase.REBOOT_CAUSE_NON_HARDWARE, ''

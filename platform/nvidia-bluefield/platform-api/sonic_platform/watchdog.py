#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
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
    from sonic_platform_base.watchdog_base import WatchdogBase
    from sonic_py_common.syslogger import SysLogger
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = SysLogger(log_identifier='Watchdog')

WD_COMMON_ERROR = -1


class Watchdog(WatchdogBase):
    """Placeholder for watchdog implementation"""

    def __init__(self):
        self.MLXBF_DRIVER = "mlxbf-bootctl"

    def exec_cmd(self, cmd, raise_exception=True):
        """Execute commands related to watchdog api"""
        try:
            return subprocess.check_output(cmd).decode().strip()
        except Exception as err:
            if raise_exception:
                raise err
            else:
                logger.log_info(f"Failed to run cmd {' '.join(cmd)}")

    def get_conf_time_and_mode(self):
        """Obtain the mode in which the watchdog is configured and the time
        Returns - A Tuple (Arm status, configured timeout)"""
        status_cmd = {self.MLXBF_DRIVER}
        ret_status = "disabled"
        ret_time = 0
        try:
            stat_output_list = self.exec_cmd(status_cmd).split('\n')
            bootctl_stat_dict = {item.split(': ')[0]: item.split(': ')[1] for item in stat_output_list}
            ret_status = bootctl_stat_dict.get('boot watchdog mode', 'disabled')
            ret_time = int(bootctl_stat_dict.get('boot watchdog interval', 0))
        except Exception as err:
            logger.log_error(f"Could not obtain status usind mlxbf-bootctl :{err}")
        return ret_status, ret_time

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        arm_time = WD_COMMON_ERROR
        if seconds < 45 or seconds > 4095:
            logger.log_error(f"Arm time provided {seconds} is outside the valid range 45-4095")
            return arm_time
        arm_command = [self.MLXBF_DRIVER, "--watchdog-boot-mode", "standard",
                       "--watchdog-boot-interval", str(seconds)]
        try:
            if self.is_armed_for_time(seconds):
                # Already armed for the time specified
                return seconds
            self.exec_cmd(arm_command)
            arm_time = seconds
        except Exception as err:
            # On an error return code check_output raises exception, return Fault
            logger.log_error(f"Could not arm watchdog :{err}")
        return arm_time

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        disarm_command = [self.MLXBF_DRIVER, "--watchdog-boot-mode", "disabled"]
        try:
            self.exec_cmd(disarm_command)
        except Exception as err:
            logger.log_error(f"Could not disarm watchdog :{err}")
            # On an error return code check_output raises exception, return Fault
            return False
        return True

    def is_armed_for_time(self, time_check=None):
        """
        Retrieves the armed state of the hardware watchdog
        And it also checks if the time configured
        If the time_check parameter is not provided, we check
        if watchdog is just armed or not
        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        conf_mode, conf_time = self.get_conf_time_and_mode()
        armed = conf_mode == 'standard'
        if not time_check:
            return armed
        return armed and (time_check == conf_time)

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        return self.is_armed_for_time()

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        timeleft = WD_COMMON_ERROR
        if self.is_armed():
            _, timeleft = self.get_conf_time_and_mode()
        return timeleft

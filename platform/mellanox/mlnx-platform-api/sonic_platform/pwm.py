#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of PWM API
#
#############################################################################

try:
    from sonic_platform_base.pwm_base import PwmBase
    from . import utils
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


PWM_FILE_PATH = "/var/run/hw-management/thermal/pwm"
PWM_NAME = "pwm"
PWM_MIN_THRESHOLD = 0
PWM_MAX_THRESHOLD = 255


class PWM(PwmBase):
    """
    PWM encapsulates PWM device functionality.
    Reads PWM value from hw-management thermal subsystem.
    """

    def __init__(self):
        super().__init__()

    def get_name(self):
        """
        Retrieves the name of the PWM device

        Returns:
            string: The name of the PWM device
        """
        return PWM_NAME

    def get_presence(self):
        """
        Retrieves the presence of the PWM device

        Returns:
            bool: True if PWM device is present, False if not
        """
        import os
        return os.path.exists(PWM_FILE_PATH)

    def get_model(self):
        """
        Retrieves the model number of the PWM device

        Returns:
            string: Model number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the PWM device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_revision(self):
        """
        Retrieves the hardware revision of the PWM device

        Returns:
            string: Revision value of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the PWM device

        Returns:
            bool: True if PWM device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.

        Returns:
            int: The 1-based relative physical position in parent device
        """
        return 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.

        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_pwm_value(self):
        """
        Retrieves the current PWM value

        Returns:
            int: The current PWM value (0-255)
        """
        return utils.read_int_from_file(PWM_FILE_PATH, default=0)

    def get_pwm_max_threshold(self):
        """
        Retrieves the maximum PWM threshold value

        Returns:
            int: The maximum PWM threshold value (255)
        """
        return PWM_MAX_THRESHOLD

    def get_pwm_min_threshold(self):
        """
        Retrieves the minimum PWM threshold value

        Returns:
            int: The minimum PWM threshold value (0)
        """
        return PWM_MIN_THRESHOLD

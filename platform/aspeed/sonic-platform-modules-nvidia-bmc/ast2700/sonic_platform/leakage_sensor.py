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
from typing import List, Optional, Tuple

try:
    from sonic_platform_base.liquid_cooling_base import LeakageSensorBase
    from sonic_platform.leak_sensor_profile import LeakSensorProfile
    from sonic_platform_base.liquid_cooling_base import LeakSeverity
    from sonic_platform.utils import hw_mgmt_path, read_sysfs_float, read_sysfs_text
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()


def _check_value_in_range(scaled, bound_a, bound_b):
    low, high = (bound_a, bound_b) if bound_a <= bound_b else (bound_b, bound_a)
    return low <= scaled <= high


class LeakageSensor(LeakageSensorBase):
    """
    NVIDIA BMC leakage sensor.
    """

    LEAKAGE_CHANNEL_DIR = hw_mgmt_path("leakage", "{a2d_index}", "{channel_index}")

    def __init__(self, a2d_index, channel_index):
        self._a2d_index = a2d_index
        self._channel_index = channel_index
        self._channel_dir = self.LEAKAGE_CHANNEL_DIR.format(a2d_index=a2d_index, channel_index=channel_index)
        name, sensor_type, location = self._get_leak_sensor_identity_from_hwmgmt()
        super().__init__(name, type=sensor_type, location=location)
        self._init_thresholds()
        self._profile = LeakSensorProfile(self.get_leak_sensor_type())

    def _get_leak_sensor_identity_from_hwmgmt(self) -> Tuple[str, Optional[str], Optional[str]]:
        channel_raw = read_sysfs_text(os.path.join(self._channel_dir, "channel_name"))
        sensor_type = read_sysfs_text(os.path.join(self._channel_dir, "type"))
        default_name = f"leakage_{self._a2d_index}_{self._channel_index}"
        name = channel_raw.strip() if channel_raw else default_name
        location = name.split("_", 1)[0] if name else None
        return name, sensor_type, location

    def _init_thresholds(self):
        self._scale = self._read_channel_sysfs("scale")
        self._min_threshold = self._read_channel_sysfs("min")
        self._max_threshold = self._read_channel_sysfs("max")
        self._lwarn_threshold = self._read_channel_sysfs("lwarn")
        self._warn_threshold = self._read_channel_sysfs("warn")
        self._lcrit_threshold = self._read_channel_sysfs("lcrit")
        self._crit_threshold = self._read_channel_sysfs("crit")
        self._hwmgmt_thresholds_valid = self._validate_hwmgmt_thresholds()

    def _read_channel_sysfs(self, path):
        return read_sysfs_float(os.path.join(self._channel_dir, path))

    def _verify_hwmgmt_data(self) -> Tuple[List[str], Optional[str]]:
        required = (
            ("scale", self._scale),
            ("min", self._min_threshold),
            ("max", self._max_threshold),
            ("lwarn", self._lwarn_threshold),
            ("warn", self._warn_threshold),
            ("lcrit", self._lcrit_threshold),
            ("crit", self._crit_threshold),
        )
        missing = [n for n, v in required if v is None]
        if missing:
            return missing, None

        order = ("lcrit", "crit", "lwarn", "warn", "min", "max")
        values = (
            self._lcrit_threshold,
            self._crit_threshold,
            self._lwarn_threshold,
            self._warn_threshold,
            self._min_threshold,
            self._max_threshold,
        )
        for lo, hi, lo_n, hi_n in zip(values, values[1:], order, order[1:]):
            if not lo < hi:
                msg = (
                    "thresholds must be strictly ordered as "
                    "lcrit < crit < lwarn < warn < min < max; "
                    f"failed {lo_n} ({lo!r}) < {hi_n} ({hi!r})"
                )
                return [], msg
        return [], None

    def _log_prefix(self) -> str:
        return f"leak sensor {self.name} ({self._channel_dir})"

    def _validate_hwmgmt_thresholds(self):
        missing, ordering_error = self._verify_hwmgmt_data()
        if missing:
            missing_list = ", ".join(sorted(missing))
            logger.log_error(f"{self._log_prefix()}: missing required sysfs values: {missing_list}")
            return False
        if ordering_error:
            logger.log_error(f"{self._log_prefix()}: {ordering_error}")
            return False
        return True

    def _check_channel_value(self) -> Tuple[bool, bool, Optional[str]]:
        """
        Evaluate the sensor and return a `(leaking, sensor_ok, severity)` tuple.
        """
        READING_ERROR = False, False, None

        if not self._hwmgmt_thresholds_valid:
            return READING_ERROR

        input_value = self._read_channel_sysfs("input")
        if input_value is None:
            logger.log_error(f"{self._log_prefix()}: missing sysfs value: input")
            return READING_ERROR

        scaled = input_value * self._scale

        if scaled > self._max_threshold or scaled < self._lcrit_threshold:
            return READING_ERROR

        if _check_value_in_range(scaled, self._min_threshold, self._max_threshold):
            return False, True, None
        if _check_value_in_range(scaled, self._lcrit_threshold, self._crit_threshold):
            return True, True, LeakSeverity.CRITICAL
        if _check_value_in_range(scaled, self._lwarn_threshold, self._warn_threshold):
            return True, True, LeakSeverity.MINOR

        return False, False, None

    def is_leak(self):
        leaking, _, _ = self._check_channel_value()
        return leaking

    def is_leak_sensor_ok(self):
        _, sensor_ok, _ = self._check_channel_value()
        return sensor_ok

    def get_leak_severity(self):
        _, _, severity = self._check_channel_value()
        return severity

    def get_leak_profile(self):
        return self._profile

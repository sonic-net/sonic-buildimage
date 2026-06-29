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
from unittest.mock import patch

import pytest

from sonic_platform_base.liquid_cooling_base import LeakageSensorBase, LeakSeverity
from sonic_platform import leakage_sensor as leakage_mod
from sonic_platform.leakage_sensor import (
    LeakageSensor,
    _check_value_in_range,
)
from sonic_platform.leak_sensor_profile import LeakSensorProfile


# Strictly ordered thresholds per the hwmgmt convention used by this driver:
# lcrit < crit < lwarn < warn < min < max
GOOD_THRESHOLDS = {
    "scale": 1.0,
    "lcrit": 0.0,
    "crit": 10.0,
    "lwarn": 20.0,
    "warn": 30.0,
    "min": 40.0,
    "max": 100.0,
}
GOOD_TEXT = {
    "channel_name": "Mngm_ADC0_Ch0_Embedded_0",
    "type": "rop",
}


def _make_sensor(text=None, floats=None, a2d=1, channel=2):
    """Build a LeakageSensor with controlled hwmgmt reads, no real sysfs."""
    text_map = dict(GOOD_TEXT)
    if text is not None:
        text_map.update(text)
    float_map = dict(GOOD_THRESHOLDS)
    if floats is not None:
        float_map.update(floats)

    def fake_text(path, default=None):
        return text_map.get(os.path.basename(path), default)

    def fake_float(path, default=None):
        return float_map.get(os.path.basename(path), default)

    with patch.object(leakage_mod, "read_sysfs_text", side_effect=fake_text), \
         patch.object(leakage_mod, "read_sysfs_float", side_effect=fake_float):
        return LeakageSensor(a2d, channel)


class TestCheckValueInRange:

    def test_inclusive_bounds_in_order(self):
        assert _check_value_in_range(5, 0, 10) is True
        assert _check_value_in_range(0, 0, 10) is True
        assert _check_value_in_range(10, 0, 10) is True

    def test_swapped_bounds_ok(self):
        assert _check_value_in_range(5, 10, 0) is True

    def test_outside(self):
        assert _check_value_in_range(11, 0, 10) is False
        assert _check_value_in_range(-1, 0, 10) is False


class TestLeakageSensorConstruction:

    def test_inherits_from_base(self):
        sensor = _make_sensor()
        assert isinstance(sensor, LeakageSensorBase)

    def test_identity_from_hwmgmt(self):
        sensor = _make_sensor()
        assert sensor.get_name() == "Mngm_ADC0_Ch0_Embedded_0"
        assert sensor.get_leak_sensor_type() == "rop"
        assert sensor.get_leak_sensor_location() == "Mngm"

    def test_fallback_name_when_channel_name_missing(self):
        sensor = _make_sensor(text={"channel_name": None})
        # Falls back to `leakage_{a2d_index}_{channel_index}`.
        assert sensor.get_name() == "leakage_1_2"
        assert sensor.get_leak_sensor_location() == "leakage"

    def test_get_leak_profile_is_a_profile(self):
        sensor = _make_sensor()
        profile = sensor.get_leak_profile()
        assert isinstance(profile, LeakSensorProfile)
        assert profile.get_type() == "rop"


class TestThresholdValidation:

    def test_valid_thresholds(self):
        sensor = _make_sensor()
        assert sensor._hwmgmt_thresholds_valid is True

    def test_missing_threshold_invalidates(self):
        sensor = _make_sensor(floats={"lcrit": None})
        assert sensor._hwmgmt_thresholds_valid is False

    def test_misordered_thresholds_invalidates(self):
        # `crit` must be < `lwarn`; make `crit` larger than `lwarn` (20.0).
        bad = dict(GOOD_THRESHOLDS)
        bad["crit"] = 25.0
        sensor = _make_sensor(floats=bad)
        assert sensor._hwmgmt_thresholds_valid is False


class TestReadingContext:

    def test_full_context_includes_input_scaled_and_thresholds(self):
        sensor = _make_sensor()
        ctx = sensor._reading_context(input_value=12.5, scaled=25.0)
        assert ctx == (
            "input=12.5, scaled=25.0, scale=1.0, "
            "lcrit=0.0, crit=10.0, lwarn=20.0, warn=30.0, min=40.0, max=100.0"
        )

    def test_thresholds_only_when_no_reading_values(self):
        sensor = _make_sensor()
        ctx = sensor._reading_context()
        assert ctx == (
            "scale=1.0, lcrit=0.0, crit=10.0, lwarn=20.0, warn=30.0, min=40.0, max=100.0"
        )

    def test_scaled_only(self):
        sensor = _make_sensor()
        ctx = sensor._reading_context(scaled=25.0)
        assert ctx == (
            "scaled=25.0, scale=1.0, "
            "lcrit=0.0, crit=10.0, lwarn=20.0, warn=30.0, min=40.0, max=100.0"
        )

    def test_input_only(self):
        sensor = _make_sensor()
        ctx = sensor._reading_context(input_value=12.5)
        assert ctx == (
            "input=12.5, scale=1.0, "
            "lcrit=0.0, crit=10.0, lwarn=20.0, warn=30.0, min=40.0, max=100.0"
        )


class TestCheckChannelValue:
    """
    Threshold layout used by the driver (strictly ordered):

        lcrit=0 < crit=10 < lwarn=20 < warn=30 < min=40 < max=100

    Bands:
        [min, max]    = [40, 100]   -> normal
        [lwarn, warn] = [20, 30]    -> MINOR leak
        [lcrit, crit] = [0, 10]     -> CRITICAL leak
        (crit, lwarn) and (warn, min) -> sensor fault (no leak, not OK)
        scaled > max or < lcrit       -> reading error
    """

    @staticmethod
    def _eval(sensor, input_value):
        side_effect = lambda key: input_value if key == "input" else None
        with patch.object(sensor, "_read_channel_sysfs", side_effect=side_effect):
            return sensor._check_channel_value()

    def test_normal_band(self):
        sensor = _make_sensor()
        assert self._eval(sensor, 50) == (False, True, None)

    def test_minor_leak_band(self):
        sensor = _make_sensor()
        assert self._eval(sensor, 25) == (True, True, LeakSeverity.MINOR)

    def test_critical_leak_band(self):
        sensor = _make_sensor()
        assert self._eval(sensor, 5) == (True, True, LeakSeverity.CRITICAL)

    def test_sensor_fault_between_critical_and_minor(self):
        sensor = _make_sensor()
        # 15 is in (crit=10, lwarn=20) -- between the critical and warning bands.
        assert self._eval(sensor, 15) == (False, False, None)

    def test_sensor_fault_between_minor_and_normal(self):
        sensor = _make_sensor()
        # 35 is in (warn=30, min=40) -- between the warning and normal bands.
        assert self._eval(sensor, 35) == (False, False, None)

    def test_above_max_is_reading_error(self):
        sensor = _make_sensor()
        assert self._eval(sensor, 200) == (False, False, None)

    def test_below_lcrit_is_reading_error(self):
        sensor = _make_sensor()
        assert self._eval(sensor, -5) == (False, False, None)

    def test_invalid_thresholds_short_circuit(self):
        sensor = _make_sensor(floats={"lcrit": None})
        # When thresholds are invalid, `_read_channel_sysfs` should not even be called.
        with patch.object(sensor, "_read_channel_sysfs") as read:
            assert sensor._check_channel_value() == (False, False, None)
        read.assert_not_called()

    def test_missing_input_is_reading_error(self):
        sensor = _make_sensor()
        assert self._eval(sensor, None) == (False, False, None)

    def test_scale_is_applied(self):
        # Same input value, different scale -> different band.
        sensor = _make_sensor(floats={"scale": 2.0})
        # 12.5 * 2.0 = 25.0 -> MINOR
        assert self._eval(sensor, 12.5) == (True, True, LeakSeverity.MINOR)


class TestPublicAccessors:

    def test_is_leak_reflects_check(self):
        sensor = _make_sensor()
        with patch.object(sensor, "_check_channel_value", return_value=(True, True, LeakSeverity.MINOR)):
            assert sensor.is_leak() is True

    def test_is_leak_sensor_ok_reflects_check(self):
        sensor = _make_sensor()
        with patch.object(sensor, "_check_channel_value", return_value=(False, True, None)):
            assert sensor.is_leak_sensor_ok() is True
        with patch.object(sensor, "_check_channel_value", return_value=(False, False, None)):
            assert sensor.is_leak_sensor_ok() is False

    def test_get_leak_severity_returns_enum_or_none(self):
        sensor = _make_sensor()
        with patch.object(sensor, "_check_channel_value", return_value=(True, True, LeakSeverity.CRITICAL)):
            assert sensor.get_leak_severity() is LeakSeverity.CRITICAL
        with patch.object(sensor, "_check_channel_value", return_value=(False, True, None)):
            assert sensor.get_leak_severity() is None

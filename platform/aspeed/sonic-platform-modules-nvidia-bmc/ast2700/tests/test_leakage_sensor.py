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
from pathlib import Path
import signal
import time
from unittest.mock import patch

import pytest

from sonic_platform_base.liquid_cooling_base import LeakageSensorBase, LeakSeverity
from sonic_platform import leakage_sensor as leakage_mod
from sonic_platform import liquid_cooling as lc_mod
from sonic_platform.leakage_sensor import (
    LeakageSensor,
    _check_value_in_range,
)
from sonic_platform.leak_sensor_profile import LeakSensorProfile
from sonic_py_common.logger import Logger


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
        # get_leak_severity() reports the reading of the last is_leak().
        sensor = _make_sensor()
        with patch.object(sensor, "_check_channel_value", return_value=(True, True, LeakSeverity.CRITICAL)):
            assert sensor.is_leak() is True
            assert sensor.get_leak_severity() is LeakSeverity.CRITICAL
        with patch.object(sensor, "_check_channel_value", return_value=(False, True, None)):
            assert sensor.is_leak() is False
            assert sensor.get_leak_severity() is None


class TestHardwareWithoutInjection:
    @pytest.mark.parametrize("reading,expected", [
        (50, (True, False, None, False)),
        (25, (True, True, LeakSeverity.MINOR, False)),
        (5, (True, True, LeakSeverity.CRITICAL, False)),
        (15, (False, False, None, False)),
    ])
    def test_accessors_preserve_hardware_state(self, leak_tree, reading, expected):
        # Without an injection, every method reports the hardware reading.
        leak_tree.input_file().write_text(str(reading))
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == expected
        assert not leak_tree.store.exists()


class TestPhysicalLeakPrecedence:
    @pytest.mark.parametrize("injected", [LeakSeverity.MINOR, LeakSeverity.CRITICAL])
    @pytest.mark.parametrize("reading,physical", [(25, LeakSeverity.MINOR), (5, LeakSeverity.CRITICAL)])
    def test_hardware_wins_then_injection_returns(self, leak_tree, injected, reading, physical):
        interface = leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        assert interface.set_test_leak("supply", True, severity=injected) is True
        leak_tree.input_file().write_text(str(reading))
        assert leak_tree.poll(sensor) == (True, True, physical, False)
        assert interface.is_test_leak_enabled("supply") is True
        leak_tree.input_file().write_text("50")
        assert leak_tree.poll(sensor) == (True, True, injected, True)

    def test_other_sensor_keeps_its_physical_leak(self, leak_tree):
        interface = leak_tree.interface()
        other = leak_tree.cooling.get_all_leak_sensors()[1]
        leak_tree.input_file(1, 2).write_text("5")
        assert leak_tree.poll(other) == (True, True, LeakSeverity.CRITICAL, False)
        for enable in (True, False):
            assert interface.set_test_leak("supply", enable) is True
            assert leak_tree.poll(other) == (True, True, LeakSeverity.CRITICAL, False)


class TestInvalidStoreAndRequests:
    @pytest.mark.parametrize("content", [b"", b"unknown", b"\xff\x00"])
    def test_invalid_marker_means_no_injection(self, leak_tree, content):
        interface = leak_tree.interface()
        leak_tree.store.mkdir()
        (leak_tree.store / "1_1.test_leak").write_bytes(content)
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, False, None, False)
        assert interface.is_test_leak_enabled("supply") is False

    def test_whitespace_around_severity_is_ignored(self, leak_tree):
        interface = leak_tree.interface()
        leak_tree.store.mkdir()
        (leak_tree.store / "1_1.test_leak").write_text(" \nCRITICAL\t")
        assert interface.is_test_leak_enabled("supply") is True
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, True, LeakSeverity.CRITICAL, True)

    @pytest.mark.parametrize("previous", [None, LeakSeverity.MINOR])
    @pytest.mark.parametrize("fault", ["replace", "create", "write"])
    def test_failed_write_preserves_marker_and_cleans_temporary(self, leak_tree, previous, fault):
        interface = leak_tree.interface()
        store = leak_tree.store
        if previous:
            assert interface.set_test_leak("supply", True, severity=previous) is True
        before = {p.name: p.read_bytes() for p in store.iterdir()} if store.exists() else {}
        real_open, real_os_open, real_fdopen = open, os.open, os.fdopen

        class FailingFile:
            # The temporary file exists; writing to it fails.
            def __init__(self, file):
                self.file = file

            def __enter__(self):
                return self

            def __exit__(self, *exc):
                self.file.close()
                return False

            def write(self, data):
                # The fault comes after the temporary file exists in the store.
                assert any(p.name not in before for p in store.iterdir())
                raise OSError("marker write denied")

        def failing_fdopen(fd, *args, **kwargs):
            return FailingFile(real_fdopen(fd, *args, **kwargs))

        def denied_open(path, mode="r", *args, **kwargs):
            if not isinstance(path, int) and Path(path).parent == store and any(c in mode for c in "wax+"):
                raise OSError("marker write denied")
            return real_open(path, mode, *args, **kwargs)

        def denied_os_open(path, flags, *args, **kwargs):
            if Path(path).parent == store and flags & (os.O_WRONLY | os.O_RDWR):
                raise OSError("marker write denied")
            return real_os_open(path, flags, *args, **kwargs)

        with patch.object(Logger, "log_notice") as notice:
            if fault == "replace":
                denied = OSError("marker replacement denied")
                with patch("os.replace", side_effect=denied), patch("os.rename", side_effect=denied):
                    assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is False
            elif fault == "create":
                with patch("builtins.open", side_effect=denied_open), patch("os.open", side_effect=denied_os_open):
                    assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is False
            else:
                with patch("os.fdopen", side_effect=failing_fdopen):
                    assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is False
        after = {p.name: p.read_bytes() for p in store.iterdir()} if store.exists() else {}
        assert after == before
        assert len(notice.call_args_list) == 1
        message = str(notice.call_args_list[0]).lower()
        assert "leak test:" in message
        assert "supply" in message and any(word in message for word in ("fail", "denied", "error", "false"))
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (
            True, bool(previous), previous, bool(previous)
        )

    def test_unreadable_marker_falls_back_to_hardware(self, leak_tree):
        interface = leak_tree.interface()
        leak_tree.store.mkdir()
        (leak_tree.store / "1_1.test_leak").mkdir()
        assert interface.is_test_leak_enabled("supply") is False
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, False, None, False)

    def test_unwritable_store_returns_false(self, leak_tree):
        interface = leak_tree.interface()
        # A file in place of the store directory makes the write fail, even as root.
        leak_tree.store.write_text("not a directory")
        assert interface.set_test_leak("supply", True) is False
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, False, None, False)

    @pytest.mark.parametrize("name,severity", [(None, LeakSeverity.MINOR), (7, LeakSeverity.MINOR),
                                               ("supply", None), ("supply", []), ("supply", 7)])
    def test_wrong_types_return_false(self, leak_tree, name, severity):
        interface = leak_tree.interface()
        assert interface.set_test_leak(name, True, severity=severity) is False
        if not isinstance(name, str):
            assert interface.is_test_leak_enabled(name) is False
        assert not leak_tree.store.exists() or not list(leak_tree.store.iterdir())


class TestInjectorLifetime:
    def test_exited_injector_leaves_state_for_another_process(self, leak_tree):
        leak_tree.interface()
        assert leak_tree.child_result(leak_tree.child("set")) is True
        assert leak_tree.child_result(leak_tree.child("poll"))[0] == [True, True, "CRITICAL", True]
        assert leak_tree.child_result(leak_tree.child("set", enable=False)) is True
        assert leak_tree.child_result(leak_tree.child("poll"))[0] == [True, False, None, False]

    @pytest.mark.parametrize("previous", [None, LeakSeverity.MINOR])
    def test_killed_writer_leaves_old_marker_and_clear_removes_orphan(self, leak_tree, previous):
        interface = leak_tree.interface()
        if previous:
            assert interface.set_test_leak("supply", True, severity=previous) is True
        child = leak_tree.child("set", pause=True)
        try:
            temporary = leak_tree.wait_for_replace(child)
            os.kill(child.pid, signal.SIGKILL)
            child.communicate(timeout=10)
            assert child.returncode == -signal.SIGKILL
            assert temporary.exists()
            assert leak_tree.child_result(leak_tree.child("poll"))[0] == [
                True, bool(previous), previous.value if previous else None, bool(previous)
            ]
            assert leak_tree.child_result(leak_tree.child("clear")) is True
            assert list(leak_tree.store.iterdir()) == []
        finally:
            if child.poll() is None:
                child.kill()
                child.communicate(timeout=10)


class TestConsistentPollReading:
    @pytest.mark.parametrize("change", ["withdraw", "missing", "physical", "inject"])
    @pytest.mark.parametrize("boundary", ["health", "leak", "severity"])
    def test_change_between_accessors(self, leak_tree, change, boundary):
        interface = leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        initial = change != "inject"
        if initial:
            assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        old = (True, LeakSeverity.CRITICAL, True) if initial else (False, None, False)
        new = {"withdraw": (False, None, False), "missing": (False, None, False),
               "physical": (True, LeakSeverity.MINOR, False),
               "inject": (True, LeakSeverity.CRITICAL, True)}[change]

        def change_state():
            if change in ("withdraw", "inject"):
                assert leak_tree.child_result(leak_tree.child("set", enable=change == "inject")) is True
            elif change == "missing":
                leak_tree.input_file().unlink()
            else:
                leak_tree.input_file().write_text("25")
            # The calls of one poll can be any time apart.
            time.sleep(0.15)

        assert sensor.is_leak_sensor_ok() is True
        if boundary == "health":
            change_state()
        leaking = sensor.is_leak()
        if boundary == "leak":
            change_state()
        severity = sensor.get_leak_severity()
        if boundary == "severity":
            change_state()
        provenance = sensor.is_test_leak()
        assert (leaking, severity, provenance) == (new if boundary == "health" else old)
        assert leak_tree.poll(sensor) == (change != "missing", *new)

    @pytest.mark.parametrize("marker,reading,change,expected,next_poll", [
        # A real leak that starts after is_leak() shows at the next poll. It never gets the test flag.
        (LeakSeverity.MINOR, "50", "5", (True, LeakSeverity.MINOR, True), (True, True, LeakSeverity.CRITICAL, False)),
        # A real leak that is_leak() saw keeps its severity and stays a real leak,
        # also when the sensor then dries over a marker, dries, or faults.
        (LeakSeverity.MINOR, "5", "50", (True, LeakSeverity.CRITICAL, False), (True, True, LeakSeverity.MINOR, True)),
        (None, "5", "50", (True, LeakSeverity.CRITICAL, False), (True, False, None, False)),
        (None, "5", "15", (True, LeakSeverity.CRITICAL, False), (False, False, None, False)),
        (None, "25", "5", (True, LeakSeverity.MINOR, False), (True, True, LeakSeverity.CRITICAL, False)),
        # No leak at is_leak(): no severity, whatever the hardware reads afterwards.
        (None, "50", "5", (False, None, False), (True, True, LeakSeverity.CRITICAL, False)),
        (None, "15", "5", (False, None, False), (True, True, LeakSeverity.CRITICAL, False)),
    ])
    def test_hardware_change_after_is_leak(self, leak_tree, marker, reading, change, expected, next_poll):
        interface = leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        if marker:
            assert interface.set_test_leak("supply", True, severity=marker) is True
        leak_tree.input_file().write_text(reading)
        leaking = sensor.is_leak()
        leak_tree.input_file().write_text(change)
        assert (leaking, sensor.get_leak_severity(), sensor.is_test_leak()) == expected
        assert leak_tree.poll(sensor) == next_poll

    @pytest.mark.parametrize("reading,marker,record", [
        ("50", None, (False, True, None, False)),
        ("50", LeakSeverity.MINOR, (True, True, LeakSeverity.MINOR, True)),
        ("5", LeakSeverity.MINOR, (True, True, LeakSeverity.CRITICAL, False)),
        ("15", LeakSeverity.MINOR, (False, False, None, False)),
    ])
    def test_is_leak_records_the_reading_for_the_base_class(self, leak_tree, reading, marker, record):
        interface = leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        if marker:
            assert interface.set_test_leak("supply", True, severity=marker) is True
        leak_tree.input_file().write_text(reading)
        assert sensor.is_leak() is record[0]
        assert (sensor.leaking, sensor.leak_sensor_ok, sensor.leak_severity, sensor.test_leak) == record

    def test_no_reading_before_the_first_is_leak(self, leak_tree):
        interface = leak_tree.interface()
        physical, injected = leak_tree.cooling.get_all_leak_sensors()[:2]
        leak_tree.input_file().write_text("5")
        assert interface.set_test_leak("return", True, severity=LeakSeverity.CRITICAL) is True
        for sensor in (physical, injected):
            assert (sensor.get_leak_severity(), sensor.is_test_leak()) == (None, False)
        assert (physical.is_leak(), physical.get_leak_severity(), physical.is_test_leak()) == (
            True, LeakSeverity.CRITICAL, False)
        assert (injected.is_leak(), injected.get_leak_severity(), injected.is_test_leak()) == (
            True, LeakSeverity.CRITICAL, True)

    @pytest.mark.parametrize("withdrawal", ["interface", "other_process", "clear_all"])
    @pytest.mark.parametrize("method", ["get_leak_severity", "is_test_leak"])
    def test_accessors_report_the_last_is_leak(self, leak_tree, withdrawal, method):
        interface = leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        assert sensor.is_leak() is True
        if withdrawal == "interface":
            assert interface.set_test_leak("supply", False) is True
        elif withdrawal == "other_process":
            assert leak_tree.child_result(leak_tree.child("set", enable=False)) is True
        else:
            assert interface.clear_test_leaks() is True
        # A request does not touch the sensor objects: the record stays until the next is_leak().
        recorded = LeakSeverity.CRITICAL if method == "get_leak_severity" else True
        assert getattr(sensor, method)() == recorded
        assert getattr(sensor, method)() == recorded
        assert sensor.is_leak() is False
        assert getattr(sensor, method)() is (None if method == "get_leak_severity" else False)

    def test_is_leak_sensor_ok_reads_now_and_keeps_the_record(self, leak_tree):
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        leak_tree.input_file().write_text("5")
        assert sensor.is_leak() is True
        leak_tree.input_file().write_text("15")
        assert sensor.is_leak_sensor_ok() is False
        assert sensor.get_leak_severity() is LeakSeverity.CRITICAL

    def test_record_belongs_to_the_sensor_object(self, leak_tree):
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        first = lc_mod.LiquidCooling().get_all_leak_sensors()[0]
        second = lc_mod.LiquidCooling().get_all_leak_sensors()[0]
        assert first.is_leak() is True
        assert (first.get_leak_severity(), first.is_test_leak()) == (LeakSeverity.CRITICAL, True)
        assert (second.get_leak_severity(), second.is_test_leak()) == (None, False)

    def test_requests_do_not_read_hardware(self, leak_tree):
        interface = leak_tree.interface()
        real_open = open
        reads = []

        def observe_open(path, *args, **kwargs):
            if not isinstance(path, int) and Path(path) == leak_tree.input_file():
                reads.append(str(path))
                raise OSError("hardware unavailable")
            return real_open(path, *args, **kwargs)
        with patch("builtins.open", side_effect=observe_open):
            assert interface.set_test_leak("supply", True) is True
            assert interface.is_test_leak_enabled("supply") is True
            assert interface.set_test_leak("supply", False) is True
            assert interface.clear_test_leaks() is True
        assert reads == []

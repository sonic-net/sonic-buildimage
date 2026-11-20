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
import sys
import types
from unittest.mock import MagicMock, NonCallableMock, patch

import pytest

from sonic_platform_base.liquid_cooling_base import LeakSeverity, LiquidCoolingBase
from sonic_platform import leakage_sensor as leakage_mod
from sonic_platform import liquid_cooling as lc_mod
from sonic_platform.liquid_cooling import LiquidCooling
from sonic_py_common.logger import Logger


def _fake_sensor(profile_type):
    """Build a stand-in for `LeakageSensor` with the given profile type."""
    sensor = MagicMock()
    profile = MagicMock()
    profile.get_type.return_value = profile_type
    sensor.get_leak_profile.return_value = profile
    return sensor, profile


class TestSortedNumericSubdirs:

    def test_returns_empty_for_missing_dir(self):
        with patch("os.path.isdir", return_value=False):
            assert lc_mod._sorted_numeric_subdirs("/nope") == []

    def test_returns_sorted_ints_skipping_non_numeric(self):
        names = ["2", "10", "1", "skip", ".hidden", "3"]
        is_dir_for = {"/p/" + n for n in names if n.isdigit()}
        with patch("os.path.isdir", side_effect=lambda p: p == "/p" or p in is_dir_for), \
             patch("os.listdir", return_value=names):
            assert lc_mod._sorted_numeric_subdirs("/p") == [1, 2, 3, 10]

    def test_skips_files_named_as_integers(self):
        # A regular file whose name is a digit must not be picked up.
        with patch("os.path.isdir", side_effect=lambda p: p == "/p"), \
             patch("os.listdir", return_value=["1", "2"]):
            assert lc_mod._sorted_numeric_subdirs("/p") == []


class TestDiscoverLeakageHwIndices:

    def test_empty_when_root_missing(self):
        with patch.object(lc_mod, "_sorted_numeric_subdirs", return_value=[]):
            assert lc_mod._discover_leakage_hw_indices() == []

    def test_enumerates_a2d_and_channels(self):
        def fake_subdirs(parent):
            # Layout: two a2d directories at the root; the first has two channels,
            # the second has one.
            if parent.endswith("/leakage"):
                return [0, 1]
            if parent.endswith("/leakage/0"):
                return [0, 1]
            if parent.endswith("/leakage/1"):
                return [2]
            return []

        with patch.object(lc_mod, "_sorted_numeric_subdirs", side_effect=fake_subdirs):
            pairs = lc_mod._discover_leakage_hw_indices()

        assert pairs == [(0, 0), (0, 1), (1, 2)]


class TestLiquidCoolingConstruction:

    def test_no_sensors_when_discovery_empty(self):
        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[]):
            lc = LiquidCooling()
        assert isinstance(lc, LiquidCoolingBase)
        assert lc.get_num_leak_sensors() == 0
        assert lc.get_all_profiles() == []

    def test_registers_one_profile_per_distinct_type(self):
        s1, p_cond_a = _fake_sensor("conductive")
        s2, p_cond_b = _fake_sensor("conductive")
        s3, p_opt = _fake_sensor("optical")

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (0, 1), (1, 0)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s1, s2, s3]):
            lc = LiquidCooling()

        assert lc.get_num_leak_sensors() == 3
        profiles = lc.get_all_profiles()
        # One profile per distinct type; the first sensor of that type wins.
        types = {p.get_type() for p in profiles}
        assert types == {"conductive", "optical"}
        assert p_cond_a in profiles
        assert p_opt in profiles
        assert p_cond_b not in profiles

    def test_skips_sensors_returning_no_profile(self):
        s_with, profile = _fake_sensor("conductive")
        s_without = MagicMock()
        s_without.get_leak_profile.return_value = None

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (0, 1)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s_with, s_without]):
            lc = LiquidCooling()

        # Both sensors are kept, but only one profile is registered.
        assert lc.get_num_leak_sensors() == 2
        assert lc.get_all_profiles() == [profile]

    def test_sensors_are_passed_to_base(self):
        s1, _ = _fake_sensor("conductive")
        s2, _ = _fake_sensor("conductive")

        with patch.object(lc_mod, "_discover_leakage_hw_indices", return_value=[(0, 0), (1, 0)]), \
             patch.object(lc_mod, "LeakageSensor", side_effect=[s1, s2]):
            lc = LiquidCooling()

        assert lc.get_all_leak_sensors() == [s1, s2]


class TestSharedLeakInterface:
    def test_interface_runs_the_base_initializer(self):
        # The base class initializer must run.
        with patch.object(lc_mod.LeakageSensorTestBase, "__init__", return_value=None) as base_init:
            interface = lc_mod.LeakageSensorTest([])
        base_init.assert_called_once_with()
        assert interface.is_test_leak_enabled("unknown") is False

    def test_injection_idempotence_unknown_names_and_clear_all(self, leak_tree):
        interface = leak_tree.interface()
        sensors = leak_tree.cooling.get_all_leak_sensors()
        # The sensors exist before the child process injects.
        assert leak_tree.child_result(leak_tree.child("set", severity="MINOR")) is True
        assert leak_tree.poll(sensors[0]) == (True, True, LeakSeverity.MINOR, True)
        assert interface.is_test_leak_enabled("supply") is True
        assert interface.set_test_leak("supply", True) is True
        assert interface.set_test_leak("supply", False) is True
        assert interface.set_test_leak("supply", False) is True
        assert interface.set_test_leak("unknown", True) is False
        assert interface.is_test_leak_enabled("unknown") is False
        for sensor in sensors:
            assert interface.set_test_leak(sensor.get_name(), True) is True
            assert leak_tree.poll(sensor) == (True, True, LeakSeverity.MINOR, True)
        (leak_tree.store / "99_99.test_leak").write_text("CRITICAL")
        (leak_tree.store / ".abandoned-write").write_text("MINOR")
        assert interface.clear_test_leaks() is True
        assert interface.clear_test_leaks() is True
        assert list(leak_tree.store.iterdir()) == []
        for sensor in sensors:
            assert leak_tree.poll(sensor) == (True, False, None, False)
            assert interface.is_test_leak_enabled(sensor.get_name()) is False

    def test_clear_all_without_store_returns_true_and_creates_nothing(self, leak_tree):
        interface = leak_tree.interface()
        with patch.object(Logger, "log_notice") as notice:
            assert interface.clear_test_leaks() is True
        assert not leak_tree.store.exists()
        assert len(notice.call_args_list) == 1
        assert "leak test:" in str(notice.call_args_list[0])

    def test_severity_is_keyword_only_and_ignored_on_withdrawal(self, leak_tree):
        interface = leak_tree.interface()
        with pytest.raises(TypeError):
            interface.set_test_leak("supply", True, LeakSeverity.CRITICAL)
        assert not leak_tree.store.exists()
        assert interface.set_test_leak("supply", True) is True
        assert interface.set_test_leak("supply", False, severity="not a severity") is True
        assert interface.is_test_leak_enabled("supply") is False

    @pytest.mark.parametrize("reading,severity", [(25, LeakSeverity.MINOR), (5, LeakSeverity.CRITICAL)])
    def test_withdrawal_reports_current_hardware(self, leak_tree, reading, severity):
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True) is True
        leak_tree.input_file().write_text(str(reading))
        assert interface.set_test_leak("supply", False) is True
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, True, severity, False)


class TestFaultsAndAmbiguousNames:
    @pytest.mark.parametrize("fault", ["outside", "missing"])
    def test_fault_masks_injection_until_hardware_recovers(self, leak_tree, fault):
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        if fault == "outside":
            leak_tree.input_file().write_text("200")
        else:
            leak_tree.input_file().unlink()
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (False, False, None, False)
        assert interface.is_test_leak_enabled("supply") is True
        leak_tree.input_file().write_text("50")
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, True, LeakSeverity.CRITICAL, True)

    def test_duplicate_name_is_refused_other_names_still_work(self, leak_tree):
        (leak_tree.input_file(1, 2).parent / "channel_name").write_text("supply")
        cooling = LiquidCooling()
        sensors = cooling.get_all_leak_sensors()
        assert [s.get_name() for s in sensors] == ["supply", "supply", "tray"]
        interface = leak_tree.interface(cooling)
        assert interface.set_test_leak("supply", True) is False
        assert interface.is_test_leak_enabled("supply") is False
        for sensor in sensors[:2]:
            assert leak_tree.poll(sensor) == (True, False, None, False)
        assert interface.set_test_leak("tray", True) is True
        assert leak_tree.poll(sensors[2]) == (True, True, LeakSeverity.MINOR, True)
        (leak_tree.store / "1_1.test_leak").write_text("CRITICAL")
        assert interface.clear_test_leaks() is True
        assert list(leak_tree.store.iterdir()) == []
        for sensor in sensors:
            assert leak_tree.poll(sensor) == (True, False, None, False)


class TestDefaultStateAndNotices:
    def test_discovery_and_reads_do_not_create_store(self, leak_tree):
        for sensor in leak_tree.cooling.get_all_leak_sensors():
            assert leak_tree.poll(sensor) == (True, False, None, False)
        assert not leak_tree.store.exists()
        assert leak_tree.child_result(leak_tree.child("poll")) == [[True, False, None, False]] * 3
        assert not leak_tree.store.exists()

    def test_failed_store_request_is_not_visible_to_other_process(self, leak_tree):
        interface = leak_tree.interface()
        leak_tree.store.write_text("not a directory")
        assert interface.set_test_leak("supply", True) is False
        assert leak_tree.child_result(leak_tree.child("poll")) == [[True, False, None, False]] * 3

    def test_only_the_interface_injects(self, leak_tree):
        # Only the interface may store a test leak: no method of a sensor or of the
        # liquid cooling object does so, whatever severity it is given.
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        for platform_object in (sensor, leak_tree.cooling):
            for name in dir(platform_object):
                method = getattr(platform_object, name)
                if name.startswith("__") or not callable(method):
                    continue
                for arguments in ((LeakSeverity.CRITICAL,), ()):
                    try:
                        method(*arguments)
                    except Exception:
                        pass
        assert not leak_tree.store.exists()
        assert leak_tree.poll(sensor) == (True, False, None, False)

    def test_each_request_emits_one_notice_after_change(self, leak_tree):
        interface = leak_tree.interface()
        for name, enable, severity, expected in [
            ("supply", True, LeakSeverity.CRITICAL, True),
            ("supply", False, LeakSeverity.MINOR, True),
            ("unknown", True, LeakSeverity.MINOR, False),
        ]:
            states = []

            def observe_notice(*args, **kwargs):
                marker = leak_tree.store / "1_1.test_leak"
                states.append(marker.read_text().strip() if marker.exists() else None)
            with patch.object(Logger, "log_notice", side_effect=observe_notice) as notice:
                assert interface.set_test_leak(name, enable, severity=severity) is expected
            assert len(notice.call_args_list) == 1
            message = str(notice.call_args_list[0])
            assert "leak test:" in message
            assert name in message
            if enable:
                assert severity.value in message
            assert states == ["CRITICAL" if name == "supply" and enable else None]


class TestStorePermissionsAndWriters:
    def test_store_is_the_directory_shared_with_the_monitor_container(self, production_leak_store):
        # /run/bmc is shared by the BMC host and the monitor container.
        assert production_leak_store == "/run/bmc/leak_sim"

    @pytest.mark.parametrize("mask", [0o000, 0o022, 0o077])
    def test_first_injection_sets_fixed_modes(self, leak_tree, mask):
        interface = leak_tree.interface()
        old_mask = os.umask(mask)
        try:
            assert interface.set_test_leak("supply", True) is True
        finally:
            os.umask(old_mask)
        assert leak_tree.store.stat().st_mode & 0o777 == 0o755
        assert (leak_tree.store / "1_1.test_leak").stat().st_mode & 0o777 == 0o600

    def test_store_is_never_writable_by_other_users(self, leak_tree):
        interface = leak_tree.interface()
        modes = []
        real_mkdir = os.mkdir

        def observed_mkdir(path, *args, **kwargs):
            real_mkdir(path, *args, **kwargs)
            modes.append(os.stat(path).st_mode & 0o777)

        old_mask = os.umask(0o000)
        try:
            with patch("os.mkdir", side_effect=observed_mkdir):
                assert interface.set_test_leak("supply", True) is True
        finally:
            os.umask(old_mask)
        # No mode seen at any step is group or world writable.
        assert modes and all(mode & 0o022 == 0 for mode in modes)
        assert leak_tree.store.stat().st_mode & 0o777 == 0o755

    def test_withdrawal_and_clear_with_unusable_store(self, leak_tree):
        interface = leak_tree.interface()
        # A store that is not a directory makes both requests fail. They do not raise.
        leak_tree.store.write_text("not a directory")
        assert interface.set_test_leak("supply", False) is False
        assert interface.clear_test_leaks() is False
        assert leak_tree.store.read_text() == "not a directory"

    def test_absent_parent_is_not_created(self, leak_tree, monkeypatch):
        missing_parent = leak_tree.store.parent / "absent"
        monkeypatch.setattr(leakage_mod, "LEAK_TEST_DIR", str(missing_parent / "leak_sim"))
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True) is False
        assert not missing_parent.exists()
        assert list(leak_tree.store.parent.iterdir()) == []

    def test_last_completed_writer_and_independent_sensors(self, leak_tree):
        leak_tree.interface()
        assert leak_tree.child_result(leak_tree.child("set", severity="MINOR")) is True
        assert leak_tree.child_result(leak_tree.child("set")) is True
        assert leak_tree.child_result(leak_tree.child("set", name="return", severity="MINOR")) is True
        assert leak_tree.child_result(leak_tree.child("poll")) == [
            [True, True, "CRITICAL", True], [True, True, "MINOR", True], [True, False, None, False]]
        assert leak_tree.child_result(leak_tree.child("set", enable=False)) is True
        assert leak_tree.child_result(leak_tree.child("poll")) == [
            [True, False, None, False], [True, True, "MINOR", True], [True, False, None, False]]

    def test_pulse_between_polls_is_not_reported(self, leak_tree):
        leak_tree.interface()
        sensor = leak_tree.cooling.get_all_leak_sensors()[0]
        assert leak_tree.poll(sensor) == (True, False, None, False)
        assert leak_tree.child_result(leak_tree.child("set")) is True
        assert leak_tree.child_result(leak_tree.child("set", enable=False)) is True
        assert leak_tree.poll(sensor) == (True, False, None, False)

    @pytest.mark.parametrize("release_before_clear", [False, True])
    def test_clear_interleaved_with_writer_leaves_complete_state(self, leak_tree, release_before_clear):
        interface = leak_tree.interface()
        child = leak_tree.child("set", pause=True)
        try:
            leak_tree.wait_for_replace(child)
            gate = Path(str(leak_tree.store) + ".go")
            if release_before_clear:
                gate.write_text("go")
                assert leak_tree.child_result(child) is True
            cleared = interface.clear_test_leaks()
            # True means the store is empty when the call returns.
            if cleared:
                assert list(leak_tree.store.iterdir()) == []
            if not release_before_clear:
                gate.write_text("go")
                result = leak_tree.child_result(child)
                assert isinstance(result, bool)
            files = list(leak_tree.store.iterdir())
            assert len(files) <= 1
            if files:
                assert files[0].name == "1_1.test_leak"
                assert files[0].read_text().strip() == "CRITICAL"
            expected = [True, bool(files), "CRITICAL" if files else None, bool(files)]
            assert leak_tree.child_result(leak_tree.child("poll"))[0] == expected
        finally:
            if child.poll() is None:
                child.kill()
                child.communicate(timeout=10)

    def test_clear_leaves_foreign_directory_and_returns_false(self, leak_tree):
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True) is True
        foreign = leak_tree.store / "foreign"
        foreign.mkdir()
        (foreign / "keep").write_text("keep")
        assert interface.clear_test_leaks() is False
        assert list(leak_tree.store.iterdir()) == [foreign]
        assert (foreign / "keep").read_text() == "keep"
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, False, None, False)


class TestStoreWithoutDatabase:
    def test_requests_and_sensor_reads_never_use_database(self, leak_tree, monkeypatch):
        # Make every database use fail: the interface must not need the database.
        accesses = []

        class ForbiddenDatabase(types.ModuleType):
            def __getattr__(self, name):
                accesses.append(name)
                raise AssertionError("database access forbidden")

            def __call__(self, *args, **kwargs):
                accesses.append("call")
                raise AssertionError("database access forbidden")
        forbidden = ForbiddenDatabase("swsscommon")

        def from_client(value):
            # A database client object, or a mock from a stubbed client.
            origin = value.__name__ if isinstance(value, types.ModuleType) else getattr(value, "__module__", "")
            return isinstance(value, NonCallableMock) or str(origin).split(".")[0] == "swsscommon"

        for name in [name for name in sys.modules if name.split(".")[0] == "swsscommon"]:
            monkeypatch.setitem(sys.modules, name, forbidden)
        monkeypatch.setitem(sys.modules, "swsscommon.swsscommon", forbidden)
        # Also trap client names bound at import time: in the platform and common
        # modules, and in the classes of the two leak modules.
        holders = [module for name, module in list(sys.modules.items())
                   if name.split(".")[0] in ("sonic_platform", "sonic_py_common") and module is not None]
        holders += [value for module_name in ("sonic_platform.leakage_sensor", "sonic_platform.liquid_cooling")
                    for value in vars(sys.modules[module_name]).values()
                    if isinstance(value, type) and value.__module__ == module_name]
        for holder in holders:
            for name, value in list(vars(holder).items()):
                if not name.startswith("__") and from_client(value):
                    monkeypatch.setattr(holder, name, forbidden)
        # Build new objects, so that a client made in a constructor is trapped too.
        cooling = LiquidCooling()
        interface = leak_tree.interface(cooling)
        sensor = cooling.get_all_leak_sensors()[0]
        assert interface.is_leak_test_supported() is True
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        assert interface.is_test_leak_enabled("supply") is True
        assert leak_tree.poll(sensor) == (True, True, LeakSeverity.CRITICAL, True)
        assert interface.set_test_leak("supply", False) is True
        assert interface.is_test_leak_enabled("supply") is False
        assert interface.clear_test_leaks() is True
        assert leak_tree.poll(sensor) == (True, False, None, False)

        assert accesses == []

    def test_repeated_injection_preserves_marker_and_emits_one_notice(self, leak_tree):
        interface = leak_tree.interface()
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        marker = leak_tree.store / "1_1.test_leak"
        # A fixed timestamp shows a rewrite, even with a coarse clock.
        os.utime(marker, ns=(1000000000, 1000000000))
        before = marker.stat()
        content = marker.read_bytes()
        with patch.object(Logger, "log_notice") as notice:
            assert interface.set_test_leak("supply", True, severity=LeakSeverity.CRITICAL) is True
        after = marker.stat()
        assert (after.st_ino, after.st_mtime_ns, after.st_size) == (before.st_ino, before.st_mtime_ns, before.st_size)
        assert marker.read_bytes() == content
        assert len(notice.call_args_list) == 1
        assert "supply" in str(notice.call_args_list[0])
        assert "CRITICAL" in str(notice.call_args_list[0])
        assert interface.set_test_leak("supply", True, severity=LeakSeverity.MINOR) is True
        assert marker.read_text().strip() == "MINOR"
        assert leak_tree.poll(leak_tree.cooling.get_all_leak_sensors()[0]) == (True, True, LeakSeverity.MINOR, True)

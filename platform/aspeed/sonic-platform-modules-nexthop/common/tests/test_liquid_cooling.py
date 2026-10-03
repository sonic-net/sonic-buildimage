#
# test_liquid_cooling.py
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
"""Unit tests for the Nexthop BMC (Aspeed) leak test API implementation in
liquid_cooling.py.

These run on any host: the injection markers are redirected to a tmp directory
and syslog is stubbed, so no BMC hardware or pmon container is required.

The leak test API contract is defined by sonic-platform-common
(sonic_platform_base/leakage_sensor_test_base.py). The vendor tests here
mirror the expectations of its base test suite
(tests/leakage_sensor_test_base_test.py) against the Nexthop implementation, plus
the marker-file behaviour specific to this platform.
"""

import importlib.util
import os
import sys
import types

import pytest

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
COMMON_DIR = os.path.dirname(TESTS_DIR)
NEXTHOP_DIR = os.path.dirname(COMMON_DIR)
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(NEXTHOP_DIR)))
PLATFORM_COMMON_ROOT = os.path.join(REPO_ROOT, "src", "sonic-platform-common")
PLATFORM_COMMON_DIR = os.path.join(PLATFORM_COMMON_ROOT, "sonic_platform_base")

# Bind sonic_platform_base to the real submodule directory without executing its
# __init__.py, which pulls in the whole transceiver stack.
_base = sys.modules.get("sonic_platform_base")
if _base is None:
    _base = types.ModuleType("sonic_platform_base")
    sys.modules["sonic_platform_base"] = _base
_base.__path__ = [PLATFORM_COMMON_DIR]

# sonic_py_common is not available in the unit-test environment; stub the only
# piece liquid_cooling.py needs.
if "sonic_py_common" not in sys.modules:
    _py_common = types.ModuleType("sonic_py_common")
    _syslogger = types.ModuleType("sonic_py_common.syslogger")

    class SysLogger:
        def __init__(self, *args, **kwargs):
            pass

        def log_notice(self, msg):
            pass

        def log_warning(self, msg):
            pass

        def log_error(self, msg):
            pass

    _syslogger.SysLogger = SysLogger
    _py_common.syslogger = _syslogger
    sys.modules["sonic_py_common"] = _py_common
    sys.modules["sonic_py_common.syslogger"] = _syslogger

from sonic_platform_base.leakage_sensor_test_base import LeakageSensorTestBase  # noqa: E402
from sonic_platform_base.liquid_cooling_base import (  # noqa: E402
    LeakSensorProfileBase,
    LeakSeverity,
)

# sonic_platform is not installed in the unit-test environment; load the module
# under test by path.
_spec = importlib.util.spec_from_file_location(
    "nexthop_liquid_cooling",
    os.path.join(COMMON_DIR, "sonic_platform", "liquid_cooling.py"))
liquid_cooling = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(liquid_cooling)

SENSOR_NAMES = ["leakage1", "leakage2"]


def marker(tmp_path, sensor_name):
    return tmp_path / "{}.{}".format(sensor_name, liquid_cooling.TEST_LEAK_SUFFIX)


@pytest.fixture
def lc(tmp_path):
    return liquid_cooling.LiquidCooling(leak_test_dir=str(tmp_path))


@pytest.fixture
def leak_test(lc):
    return lc.get_leak_sensor_test()


class TestSensors:
    """Sensor inventory and hardware state (no hardware read path yet, so never
    leaking)."""

    def test_sensors_and_profile(self, lc):
        assert lc.get_num_leak_sensors() == 2
        names = [s.get_name() for s in lc.get_all_leak_sensors()]
        assert names == SENSOR_NAMES

        sensor = lc.get_leak_sensor(0)
        assert sensor.get_leak_sensor_type() == liquid_cooling.SENSOR_TYPE
        assert sensor.get_leak_sensor_location() == "Intake"
        assert sensor.is_leak_sensor_ok() == True

        profile = sensor.get_leak_profile()
        assert isinstance(profile, LeakSensorProfileBase)
        assert profile.get_type() == liquid_cooling.SENSOR_TYPE
        assert profile.get_leak_max_minor_duration_sec() == \
            liquid_cooling.DEFAULT_MAX_MINOR_DURATION_SEC
        assert lc.get_all_profiles() == [profile]
        assert lc.get_profile(liquid_cooling.SENSOR_TYPE) is profile

    def test_no_leak_without_injection(self, lc):
        assert lc.get_leak_sensor_status() == []
        for sensor in lc.get_all_leak_sensors():
            assert sensor.is_leak() == False
            assert sensor.is_test_leak() == False
            assert sensor.get_leak_severity() is None
            assert sensor.is_leak_sensor_ok() == True


class TestLeakTestApi:
    """Nexthop BMC implementation of the LeakageSensorTestBase contract."""

    def test_leak_test_interface(self, lc, leak_test):
        assert isinstance(leak_test, LeakageSensorTestBase)
        assert leak_test.is_leak_test_supported() == True
        # The same object is handed out on every call.
        assert lc.get_leak_sensor_test() is leak_test

    def test_severity_is_keyword_only(self, leak_test):
        with pytest.raises(TypeError):
            leak_test.set_test_leak("leakage1", True, LeakSeverity.CRITICAL)

    def test_default_severity_is_minor(self, lc, leak_test, tmp_path):
        assert leak_test.set_test_leak("leakage1", True) == True
        assert marker(tmp_path, "leakage1").read_text() == LeakSeverity.MINOR.value

        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == True
        assert sensor.get_leak_severity() is LeakSeverity.MINOR

    def test_set_test_leak_is_idempotent(self, leak_test):
        assert leak_test.set_test_leak("leakage1", True) == True
        assert leak_test.set_test_leak("leakage1", True) == True
        assert leak_test.is_test_leak_enabled("leakage1") == True
        assert leak_test.set_test_leak("leakage1", False) == True
        assert leak_test.set_test_leak("leakage1", False) == True
        assert leak_test.is_test_leak_enabled("leakage1") == False
        assert leak_test.clear_test_leaks() == True

    def test_reinjection_updates_severity(self, lc, leak_test):
        leak_test.set_test_leak("leakage1", True, severity=LeakSeverity.MINOR)
        leak_test.set_test_leak("leakage1", True, severity=LeakSeverity.CRITICAL)

        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == True
        assert sensor.get_leak_severity() is LeakSeverity.CRITICAL

    def test_unknown_sensor_returns_false(self, leak_test, tmp_path):
        assert leak_test.set_test_leak("no_such_sensor", True) == False
        assert leak_test.is_test_leak_enabled("no_such_sensor") == False
        assert leak_test.set_test_leak("no_such_sensor", False) == False
        assert not marker(tmp_path, "no_such_sensor").exists()

    def test_clear_with_nothing_injected(self, leak_test):
        assert leak_test.clear_test_leaks() == True

    def test_injected_leak_is_flagged(self, lc, leak_test):
        assert leak_test.set_test_leak("leakage1", True,
                                       severity=LeakSeverity.CRITICAL) == True
        assert leak_test.is_test_leak_enabled("leakage1") == True
        assert leak_test.is_test_leak_enabled("leakage2") == False

        leaking = lc.get_leak_sensor_status()
        assert len(leaking) == 1
        assert leaking[0].get_name() == "leakage1"
        assert leaking[0].is_test_leak() == True
        assert leaking[0].get_leak_severity() is LeakSeverity.CRITICAL
        assert leaking[0].is_leak_sensor_ok() == True

        assert leak_test.clear_test_leaks() == True
        assert lc.get_leak_sensor_status() == []
        assert leaking[0].is_test_leak() == False
        assert leaking[0].get_leak_severity() is None

    def test_clear_test_leaks_withdraws_every_sensor(self, lc, leak_test, tmp_path):
        for name in SENSOR_NAMES:
            assert leak_test.set_test_leak(name, True) == True
        assert len(lc.get_leak_sensor_status()) == 2

        assert leak_test.clear_test_leaks() == True
        for name in SENSOR_NAMES:
            assert leak_test.is_test_leak_enabled(name) == False
            assert not marker(tmp_path, name).exists()
        assert lc.get_leak_sensor_status() == []

    def test_withdrawal_only_touches_injected_sensors(self, lc, leak_test, tmp_path):
        # A marker not written through this object (e.g. by another process)
        # is still an injection on that sensor and must survive a withdrawal
        # aimed at a different sensor.
        marker(tmp_path, "leakage2").write_text(LeakSeverity.CRITICAL.value)
        leak_test.set_test_leak("leakage1", True)

        assert leak_test.set_test_leak("leakage1", False) == True
        assert leak_test.is_test_leak_enabled("leakage1") == False
        assert leak_test.is_test_leak_enabled("leakage2") == True
        leaking = lc.get_leak_sensor_status()
        assert [s.get_name() for s in leaking] == ["leakage2"]
        assert leaking[0].get_leak_severity() is LeakSeverity.CRITICAL


class TestMarkerFiles:
    """Platform-specific behaviour of the marker-file backing store."""

    def test_marker_is_created_under_leak_test_dir(self, leak_test, tmp_path):
        leak_test.set_test_leak("leakage1", True, severity=LeakSeverity.CRITICAL)
        assert marker(tmp_path, "leakage1").read_text() == LeakSeverity.CRITICAL.value
        assert sorted(os.listdir(tmp_path)) == [marker(tmp_path, "leakage1").name]

    def test_leak_test_dir_is_created_on_demand(self, tmp_path):
        leak_dir = tmp_path / "nested" / "leak_sim"
        lc = liquid_cooling.LiquidCooling(leak_test_dir=str(leak_dir))
        leak_test = lc.get_leak_sensor_test()

        # Reading and withdrawing before the directory exists are no-ops.
        assert lc.get_leak_sensor_status() == []
        assert leak_test.is_test_leak_enabled("leakage1") == False
        assert leak_test.clear_test_leaks() == True
        assert not leak_dir.exists()

        assert leak_test.set_test_leak("leakage1", True) == True
        assert marker(leak_dir, "leakage1").exists()

    def test_injection_survives_new_liquid_cooling_instance(self, tmp_path):
        # thermalctld and the injection CLI hold separate LiquidCooling objects,
        # so the state has to come from the markers rather than from memory.
        liquid_cooling.LiquidCooling(
            leak_test_dir=str(tmp_path)).get_leak_sensor_test().set_test_leak(
                "leakage1", True, severity=LeakSeverity.MINOR)

        lc = liquid_cooling.LiquidCooling(leak_test_dir=str(tmp_path))
        assert lc.get_leak_sensor_test().is_test_leak_enabled("leakage1") == True
        leaking = lc.get_leak_sensor_status()
        assert len(leaking) == 1
        assert leaking[0].get_name() == "leakage1"
        assert leaking[0].is_test_leak() == True
        assert leaking[0].get_leak_severity() is LeakSeverity.MINOR

    def test_withdrawal_from_other_instance_is_observed(self, tmp_path):
        injector = liquid_cooling.LiquidCooling(leak_test_dir=str(tmp_path))
        poller = liquid_cooling.LiquidCooling(leak_test_dir=str(tmp_path))

        injector.get_leak_sensor_test().set_test_leak("leakage1", True)
        assert len(poller.get_leak_sensor_status()) == 1

        assert injector.get_leak_sensor_test().clear_test_leaks() == True
        assert poller.get_leak_sensor_status() == []
        assert poller.get_leak_sensor(0).is_test_leak() == False

    def test_accessors_report_last_poll(self, lc, leak_test):
        # Only is_leak() re-reads the marker; the other accessors report the
        # state recorded by the last poll, so one poll gives a consistent view.
        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == False

        leak_test.set_test_leak("leakage1", True)
        assert sensor.is_test_leak() == False
        assert sensor.get_leak_severity() is None

        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True
        assert sensor.get_leak_severity() is LeakSeverity.MINOR

    def test_invalid_marker_content_reports_critical(self, lc, tmp_path):
        marker(tmp_path, "leakage1").write_text("bogus")

        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True
        assert sensor.get_leak_severity() is liquid_cooling.HW_SEVERITY
        assert sensor.get_leak_severity() is LeakSeverity.CRITICAL

    def test_unwritable_leak_test_dir_returns_false(self, tmp_path):
        # A regular file in place of the marker directory makes injection fail;
        # the API reports that rather than raising.
        bogus_dir = tmp_path / "not_a_dir"
        bogus_dir.write_text("")
        leak_test = liquid_cooling.LiquidCooling(
            leak_test_dir=str(bogus_dir)).get_leak_sensor_test()

        assert leak_test.set_test_leak("leakage1", True) == False
        assert leak_test.is_test_leak_enabled("leakage1") == False

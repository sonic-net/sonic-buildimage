#
# test_liquid_cooling.py
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
"""Unit tests for the Nexthop BMC (Aspeed) liquid cooling implementation in
liquid_cooling.py: the GPIO-backed leak sensors and the leak test API.

These run on any host: GPIO reads go through a fake, the injection markers are
redirected to a tmp directory and syslog is stubbed, so no BMC hardware or pmon
container is required.

The leak test API contract is defined by sonic-platform-common
(sonic_platform_base/leakage_sensor_test_base.py). The vendor tests here
mirror the expectations of its base test suite
(tests/leakage_sensor_test_base_test.py) against the Nexthop implementation, plus
the marker-file behaviour specific to this platform.
"""

import importlib.util
import os
import subprocess
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

# sonic_platform is not installed in the unit-test environment. Bind the
# package to the source directory without executing its __init__.py, so that
# liquid_cooling.py can import sonic_platform.leak_detect, and load the module
# under test by path.
_platform = sys.modules.get("sonic_platform")
if _platform is None:
    _platform = types.ModuleType("sonic_platform")
    sys.modules["sonic_platform"] = _platform
_platform.__path__ = [os.path.join(COMMON_DIR, "sonic_platform")]

_spec = importlib.util.spec_from_file_location(
    "nexthop_liquid_cooling",
    os.path.join(COMMON_DIR, "sonic_platform", "liquid_cooling.py"))
liquid_cooling = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(liquid_cooling)

SENSOR_NAMES = ["coldplate_leak", "driptray_leak"]
LINES = {"coldplate_leak": "leak_fault_0", "driptray_leak": "leak_fault_1"}


def marker(tmp_path, sensor_name):
    return tmp_path / "{}.{}".format(sensor_name, liquid_cooling.TEST_LEAK_SUFFIX)


class FakeGpio:
    """Stands in for gpioget. Lines default to dry; a value of None makes the
    read fail as an absent or claimed line would."""

    def __init__(self):
        self.values = {line: "0" for line in LINES.values()}
        self.reads = []

    def __call__(self, line):
        self.reads.append(line)
        value = self.values[line]
        if value is None:
            raise liquid_cooling.leak_detect.LeakLineError(
                "cannot read {}".format(line))
        return value + "\n"


@pytest.fixture
def gpio():
    return FakeGpio()


def make_lc(leak_test_dir, gpio=None):
    return liquid_cooling.LiquidCooling(leak_test_dir=str(leak_test_dir),
                                        read_line=gpio or FakeGpio())


@pytest.fixture
def lc(tmp_path, gpio):
    return make_lc(tmp_path, gpio)


@pytest.fixture
def leak_test(lc):
    return lc.get_leak_sensor_test()


class TestSensors:
    """Sensor inventory and profiles."""

    def test_sensors(self, lc):
        assert lc.get_num_leak_sensors() == 2
        names = [s.get_name() for s in lc.get_all_leak_sensors()]
        assert names == SENSOR_NAMES

        coldplate, driptray = lc.get_all_leak_sensors()
        assert coldplate.get_leak_sensor_type() == "FlexPCB"
        assert coldplate.get_leak_sensor_location() == "ColdPlate"
        assert driptray.get_leak_sensor_type() == "Rope"
        assert driptray.get_leak_sensor_location() == "DripTray"
        for sensor in (coldplate, driptray):
            assert sensor.line == LINES[sensor.get_name()]
            assert sensor.is_leak_sensor_ok() == True

    def test_one_profile_per_sensor_type(self, lc):
        profiles = lc.get_all_profiles()
        assert sorted(p.get_type() for p in profiles) == ["FlexPCB", "Rope"]
        for sensor in lc.get_all_leak_sensors():
            profile = sensor.get_leak_profile()
            assert isinstance(profile, LeakSensorProfileBase)
            assert profile.get_type() == sensor.get_leak_sensor_type()
            assert lc.get_profile(profile.get_type()) is profile
            assert profile.get_leak_max_minor_duration_sec() == \
                liquid_cooling.DEFAULT_MAX_MINOR_DURATION_SEC

    def test_no_leak_when_lines_are_dry(self, lc):
        assert lc.get_leak_sensor_status() == []
        for sensor in lc.get_all_leak_sensors():
            assert sensor.is_leak() == False
            assert sensor.is_test_leak() == False
            assert sensor.get_leak_severity() is None
            assert sensor.is_leak_sensor_ok() == True


class TestHardware:
    """Leak state read from the BMC GPIO lines, and how it combines with an
    injected test leak."""

    @pytest.mark.parametrize("name,severity", [
        ("coldplate_leak", LeakSeverity.MINOR),
        ("driptray_leak", LeakSeverity.CRITICAL),
    ])
    def test_asserted_line_is_a_real_leak(self, lc, gpio, name, severity):
        gpio.values[LINES[name]] = "1"

        leaking = lc.get_leak_sensor_status()
        assert [s.get_name() for s in leaking] == [name]
        assert leaking[0].is_test_leak() == False
        assert leaking[0].get_leak_severity() is severity
        assert leaking[0].is_leak_sensor_ok() == True

    def test_each_sensor_reads_its_own_line(self, lc, gpio):
        for sensor in lc.get_all_leak_sensors():
            gpio.reads.clear()
            sensor.is_leak()
            assert gpio.reads == [LINES[sensor.get_name()]]

    def test_clearing_line_clears_leak(self, lc, gpio):
        sensor = lc.get_leak_sensor(1)
        gpio.values["leak_fault_1"] = "1"
        assert sensor.is_leak() == True
        gpio.values["leak_fault_1"] = "0"
        assert sensor.is_leak() == False
        assert sensor.get_leak_severity() is None

    def test_hardware_leak_takes_precedence_over_injection(self, lc, gpio,
                                                           leak_test):
        # A real leak must never be reported as a test leak, or consumers
        # would skip mitigation for it.
        sensor = lc.get_leak_sensor(0)
        leak_test.set_test_leak("coldplate_leak", True,
                                severity=LeakSeverity.CRITICAL)
        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True

        gpio.values["leak_fault_0"] = "1"
        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == False
        assert sensor.get_leak_severity() is LeakSeverity.MINOR

    def test_withdrawal_returns_to_hardware_state(self, lc, gpio, leak_test):
        # A leak that starts while an injection is active is still reported
        # once the injection is withdrawn.
        sensor = lc.get_leak_sensor(1)
        leak_test.set_test_leak("driptray_leak", True)
        assert sensor.is_leak() == True
        gpio.values["leak_fault_1"] = "1"
        assert leak_test.set_test_leak("driptray_leak", False) == True

        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == False
        assert sensor.get_leak_severity() is LeakSeverity.CRITICAL

    @pytest.mark.parametrize("value", [None, "banana", ""])
    def test_unreadable_line_is_a_faulty_sensor(self, lc, gpio, value):
        # An absent DTB entry, a line held by a gpio-hog or output we do not
        # understand must show up as a faulty sensor, not a clean dry reading.
        gpio.values["leak_fault_0"] = value
        sensor = lc.get_leak_sensor(0)

        assert sensor.is_leak_sensor_ok() == False
        assert sensor.is_leak() == False
        assert sensor.is_leak_sensor_ok() == False
        assert lc.get_leak_sensor(1).is_leak_sensor_ok() == True

    def test_injection_is_reported_on_unreadable_line(self, lc, gpio,
                                                      leak_test):
        gpio.values["leak_fault_0"] = None
        leak_test.set_test_leak("coldplate_leak", True)
        sensor = lc.get_leak_sensor(0)

        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True
        assert sensor.is_leak_sensor_ok() == False


class TestReadGpioLine:
    """read_gpio_line(), the default line reader, with gpioget replaced."""

    def _run(self, monkeypatch, result=None, exc=None):
        calls = []

        def fake_run(cmd, **kwargs):
            calls.append(cmd)
            if exc is not None:
                raise exc
            return result

        monkeypatch.setattr(liquid_cooling.subprocess, "run", fake_run)
        return calls

    def test_reads_line_by_name(self, monkeypatch):
        calls = self._run(monkeypatch, subprocess.CompletedProcess(
            [], 0, stdout="1\n", stderr=""))
        assert liquid_cooling.read_gpio_line("leak_fault_0") == "1\n"
        assert calls == [["gpioget", "--strict", "--numeric", "--consumer",
                          "leak-detect", "leak_fault_0"]]

    @pytest.mark.parametrize("result,exc", [
        (subprocess.CompletedProcess([], 1, stdout="",
                                     stderr="Device or resource busy"), None),
        (None, FileNotFoundError("gpioget")),
        (None, subprocess.TimeoutExpired("gpioget", 5)),
    ])
    def test_failures_raise(self, monkeypatch, result, exc):
        self._run(monkeypatch, result, exc)
        with pytest.raises(liquid_cooling.leak_detect.LeakLineError):
            liquid_cooling.read_gpio_line("leak_fault_0")


class TestLeakTestApi:
    """Nexthop BMC implementation of the LeakageSensorTestBase contract."""

    def test_leak_test_interface(self, lc, leak_test):
        assert isinstance(leak_test, LeakageSensorTestBase)
        assert leak_test.is_leak_test_supported() == True
        # The same object is handed out on every call.
        assert lc.get_leak_sensor_test() is leak_test

    def test_severity_is_keyword_only(self, leak_test):
        with pytest.raises(TypeError):
            leak_test.set_test_leak("coldplate_leak", True, LeakSeverity.CRITICAL)

    def test_default_severity_is_minor(self, lc, leak_test, tmp_path):
        assert leak_test.set_test_leak("coldplate_leak", True) == True
        assert marker(tmp_path, "coldplate_leak").read_text() == LeakSeverity.MINOR.value

        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == True
        assert sensor.get_leak_severity() is LeakSeverity.MINOR

    def test_set_test_leak_is_idempotent(self, leak_test):
        assert leak_test.set_test_leak("coldplate_leak", True) == True
        assert leak_test.set_test_leak("coldplate_leak", True) == True
        assert leak_test.is_test_leak_enabled("coldplate_leak") == True
        assert leak_test.set_test_leak("coldplate_leak", False) == True
        assert leak_test.set_test_leak("coldplate_leak", False) == True
        assert leak_test.is_test_leak_enabled("coldplate_leak") == False
        assert leak_test.clear_test_leaks() == True

    def test_reinjection_updates_severity(self, lc, leak_test):
        leak_test.set_test_leak("coldplate_leak", True, severity=LeakSeverity.MINOR)
        leak_test.set_test_leak("coldplate_leak", True, severity=LeakSeverity.CRITICAL)

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
        assert leak_test.set_test_leak("coldplate_leak", True,
                                       severity=LeakSeverity.CRITICAL) == True
        assert leak_test.is_test_leak_enabled("coldplate_leak") == True
        assert leak_test.is_test_leak_enabled("driptray_leak") == False

        leaking = lc.get_leak_sensor_status()
        assert len(leaking) == 1
        assert leaking[0].get_name() == "coldplate_leak"
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
        marker(tmp_path, "driptray_leak").write_text(LeakSeverity.CRITICAL.value)
        leak_test.set_test_leak("coldplate_leak", True)

        assert leak_test.set_test_leak("coldplate_leak", False) == True
        assert leak_test.is_test_leak_enabled("coldplate_leak") == False
        assert leak_test.is_test_leak_enabled("driptray_leak") == True
        leaking = lc.get_leak_sensor_status()
        assert [s.get_name() for s in leaking] == ["driptray_leak"]
        assert leaking[0].get_leak_severity() is LeakSeverity.CRITICAL


class TestMarkerFiles:
    """Platform-specific behaviour of the marker-file backing store."""

    def test_marker_is_created_under_leak_test_dir(self, leak_test, tmp_path):
        leak_test.set_test_leak("coldplate_leak", True, severity=LeakSeverity.CRITICAL)
        assert marker(tmp_path, "coldplate_leak").read_text() == LeakSeverity.CRITICAL.value
        assert sorted(os.listdir(tmp_path)) == [marker(tmp_path, "coldplate_leak").name]

    def test_leak_test_dir_is_created_on_demand(self, tmp_path):
        leak_dir = tmp_path / "nested" / "leak_sim"
        lc = make_lc(leak_dir)
        leak_test = lc.get_leak_sensor_test()

        # Reading and withdrawing before the directory exists are no-ops.
        assert lc.get_leak_sensor_status() == []
        assert leak_test.is_test_leak_enabled("coldplate_leak") == False
        assert leak_test.clear_test_leaks() == True
        assert not leak_dir.exists()

        assert leak_test.set_test_leak("coldplate_leak", True) == True
        assert marker(leak_dir, "coldplate_leak").exists()

    def test_injection_survives_new_liquid_cooling_instance(self, tmp_path):
        # thermalctld and the injection CLI hold separate LiquidCooling objects,
        # so the state has to come from the markers rather than from memory.
        make_lc(tmp_path).get_leak_sensor_test().set_test_leak(
                "coldplate_leak", True, severity=LeakSeverity.MINOR)

        lc = make_lc(tmp_path)
        assert lc.get_leak_sensor_test().is_test_leak_enabled("coldplate_leak") == True
        leaking = lc.get_leak_sensor_status()
        assert len(leaking) == 1
        assert leaking[0].get_name() == "coldplate_leak"
        assert leaking[0].is_test_leak() == True
        assert leaking[0].get_leak_severity() is LeakSeverity.MINOR

    def test_withdrawal_from_other_instance_is_observed(self, tmp_path):
        injector = make_lc(tmp_path)
        poller = make_lc(tmp_path)

        injector.get_leak_sensor_test().set_test_leak("coldplate_leak", True)
        assert len(poller.get_leak_sensor_status()) == 1

        assert injector.get_leak_sensor_test().clear_test_leaks() == True
        assert poller.get_leak_sensor_status() == []
        assert poller.get_leak_sensor(0).is_test_leak() == False

    def test_accessors_report_last_poll(self, lc, leak_test):
        # Only is_leak() re-reads the marker; the other accessors report the
        # state recorded by the last poll, so one poll gives a consistent view.
        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == False

        leak_test.set_test_leak("coldplate_leak", True)
        assert sensor.is_test_leak() == False
        assert sensor.get_leak_severity() is None

        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True
        assert sensor.get_leak_severity() is LeakSeverity.MINOR

    def test_invalid_marker_content_reports_critical(self, lc, tmp_path):
        marker(tmp_path, "coldplate_leak").write_text("bogus")

        sensor = lc.get_leak_sensor(0)
        assert sensor.is_leak() == True
        assert sensor.is_test_leak() == True
        assert sensor.get_leak_severity() is \
            liquid_cooling.INVALID_INJECTION_SEVERITY
        assert sensor.get_leak_severity() is LeakSeverity.CRITICAL

    def test_unwritable_leak_test_dir_returns_false(self, tmp_path):
        # A regular file in place of the marker directory makes injection fail;
        # the API reports that rather than raising.
        bogus_dir = tmp_path / "not_a_dir"
        bogus_dir.write_text("")
        leak_test = make_lc(bogus_dir).get_leak_sensor_test()

        assert leak_test.set_test_leak("coldplate_leak", True) == False
        assert leak_test.is_test_leak_enabled("coldplate_leak") == False

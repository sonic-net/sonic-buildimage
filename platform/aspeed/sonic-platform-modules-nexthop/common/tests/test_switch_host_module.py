"""Unit tests for the switch CPU power control in switch_host_module.py.

Self-contained like test_cooling.py: the module is loaded by path with the two
sonic_platform_base imports stubbed, and subprocess.run is replaced by a fake
that records the gpioset/gpioget invocations, so no BMC hardware is required.
"""

import importlib.util
import os
import subprocess
import sys
import types

import pytest

_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_COMMON_DIR = os.path.dirname(_TESTS_DIR)
MODULE_PATH = os.path.join(_COMMON_DIR, "sonic_platform", "switch_host_module.py")


def _load():
    stubs = {
        "sonic_platform_base": types.ModuleType("sonic_platform_base"),
        "sonic_platform_base.module_base": types.ModuleType("sonic_platform_base.module_base"),
        "sonic_platform_base.sonic_eeprom": types.ModuleType("sonic_platform_base.sonic_eeprom"),
        "sonic_platform_base.sonic_eeprom.eeprom_tlvinfo":
            types.ModuleType("sonic_platform_base.sonic_eeprom.eeprom_tlvinfo"),
    }

    class ModuleBase:
        MODULE_STATUS_ONLINE = "Online"
        MODULE_STATUS_OFFLINE = "Offline"
        MODULE_STATUS_FAULT = "Fault"
        MODULE_TYPE_SWITCH_HOST = "SWITCH_HOST"

    stubs["sonic_platform_base.module_base"].ModuleBase = ModuleBase
    stubs["sonic_platform_base.sonic_eeprom.eeprom_tlvinfo"].TlvInfoDecoder = object

    saved = {name: sys.modules.get(name) for name in stubs}
    sys.modules.update(stubs)
    try:
        spec = importlib.util.spec_from_file_location("nexthop_switch_host_under_test",
                                                      MODULE_PATH)
        assert spec is not None and spec.loader is not None, \
            "cannot load {}".format(MODULE_PATH)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        return mod
    finally:
        for name, prev in saved.items():
            if prev is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = prev


shm = _load()
LINE = shm.SwitchHostModule.CPE_CTRL_LINE


class FakeGpio:
    """Records gpioset/gpioget calls and answers with configurable results."""

    def __init__(self):
        self.calls = []
        self.set_rc = 0
        self.get_rc = 0
        self.get_stdout = "1\n"
        self.raise_on_run = None

    def run(self, cmd, **kwargs):
        self.calls.append(cmd)
        if self.raise_on_run:
            raise self.raise_on_run
        if cmd[0] == "gpioset":
            return subprocess.CompletedProcess(cmd, self.set_rc, "", "set error")
        if cmd[0] == "gpioget":
            return subprocess.CompletedProcess(cmd, self.get_rc, self.get_stdout, "get error")
        raise AssertionError(f"unexpected command {cmd}")

    def toggles(self, cmd):
        """The --toggle period list of a gpioset command."""
        return cmd[cmd.index("--toggle") + 1].split(",")


@pytest.fixture
def gpio(monkeypatch):
    fake = FakeGpio()
    monkeypatch.setattr(shm.subprocess, "run", fake.run)
    monkeypatch.setattr(shm.time, "sleep", lambda s: None)
    return fake


def test_power_off_is_one_gpioset_with_two_falling_edges(gpio):
    module = shm.SwitchHostModule()
    assert module.set_admin_state(False) is True

    assert len(gpio.calls) == 1
    cmd = gpio.calls[0]
    assert cmd[0] == "gpioset"
    assert cmd[-1] == f"{LINE}=1"
    # high -5ms-> low -5ms-> high -5ms-> low, then exit
    assert gpio.toggles(cmd) == ["5ms", "5ms", "5ms", "0"]


def test_power_off_pulse_count_is_configurable(gpio, monkeypatch):
    monkeypatch.setattr(shm.SwitchHostModule, "POWER_OFF_PULSE_COUNT", 3)
    shm.SwitchHostModule().set_admin_state(False)
    assert gpio.toggles(gpio.calls[0]) == ["5ms"] * 5 + ["0"]


def test_power_off_toggle_count_is_odd_so_line_ends_low():
    count = shm.SwitchHostModule.POWER_OFF_PULSE_COUNT
    assert (2 * count - 1) % 2 == 1


def test_power_off_uses_named_consumer(gpio):
    shm.SwitchHostModule().set_admin_state(False)
    cmd = gpio.calls[0]
    assert cmd[cmd.index("--consumer") + 1] == shm.SwitchHostModule.GPIO_CONSUMER


def test_power_off_restores_high_when_gpioset_fails(gpio):
    gpio.set_rc = 1
    assert shm.SwitchHostModule().set_admin_state(False) is False
    # Failed pulse train, then a recovery power-on
    assert [gpio.toggles(c) for c in gpio.calls] == [["5ms", "5ms", "5ms", "0"], ["0"]]


def test_power_off_fails_when_gpioset_missing(gpio):
    gpio.raise_on_run = FileNotFoundError("gpioset")
    assert shm.SwitchHostModule().set_admin_state(False) is False


def test_power_on_drives_high_and_exits(gpio):
    assert shm.SwitchHostModule().set_admin_state(True) is True
    assert len(gpio.calls) == 1
    cmd = gpio.calls[0]
    assert cmd[0] == "gpioset"
    assert cmd[-1] == f"{LINE}=1"
    assert gpio.toggles(cmd) == ["0"]


def test_power_on_fails_when_gpioset_fails(gpio):
    gpio.set_rc = 1
    assert shm.SwitchHostModule().set_admin_state(True) is False


def test_power_cycle_pulses_off_waits_then_powers_on(gpio, monkeypatch):
    sleeps = []
    monkeypatch.setattr(shm.time, "sleep", sleeps.append)
    module = shm.SwitchHostModule()

    assert module.do_power_cycle() is True
    assert [c[0] for c in gpio.calls] == ["gpioset", "gpioset"]
    assert gpio.toggles(gpio.calls[0]) == ["5ms", "5ms", "5ms", "0"]
    assert gpio.toggles(gpio.calls[1]) == ["0"]
    assert sleeps == [module.POWER_CYCLE_OFF_SEC]


def test_power_cycle_does_not_power_on_after_failed_power_off(gpio, monkeypatch):
    sleeps = []
    monkeypatch.setattr(shm.time, "sleep", sleeps.append)
    module = shm.SwitchHostModule()
    monkeypatch.setattr(module, "_power_off", lambda: False)

    assert module.do_power_cycle() is False
    assert gpio.calls == []
    assert sleeps == []


def test_reboot_delegates_to_power_cycle(gpio):
    assert shm.SwitchHostModule().reboot() is True
    assert len(gpio.calls) == 2


@pytest.mark.parametrize("stdout,expected", [
    ("1\n", "Online"),
    ("0\n", "Offline"),
])
def test_oper_status_follows_line_level(gpio, stdout, expected):
    gpio.get_stdout = stdout
    assert shm.SwitchHostModule().get_oper_status() == expected


def test_oper_status_reads_as_is(gpio):
    # Requesting the line as an input would stop driving the CPU
    shm.SwitchHostModule().get_oper_status()
    cmd = gpio.calls[0]
    assert cmd[0] == "gpioget"
    assert "--as-is" in cmd
    assert "--numeric" in cmd
    assert cmd[-1] == LINE


def test_oper_status_fault_when_gpioget_fails(gpio):
    gpio.get_rc = 1
    assert shm.SwitchHostModule().get_oper_status() == "Fault"


def test_oper_status_fault_on_unparseable_output(gpio):
    gpio.get_stdout = "active\n"
    assert shm.SwitchHostModule().get_oper_status() == "Fault"

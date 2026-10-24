"""Unit tests for switch_host_module.py.

Self-contained by design: sonic_platform_base is stubbed and
switch_host_module.py is loaded directly by path, so neither the real
sonic_platform_base package nor any EEPROM/BMC hardware is required. Two areas
are covered:

- EEPROM TLV reading: the switch-card EEPROM is statically instantiated by the
  device tree, so the module just reads the sysfs path; here that read is stubbed
  via builtins.open.
- Switch CPU power control: subprocess.run is replaced by a fake that records the
  gpioset/gpioget invocations.
"""

import importlib.util
import os
import subprocess
import sys
import types
from unittest import mock

import pytest

# ONIE TlvInfo type codes used by the parser under test.
_TLV_CODE_PRODUCT_NAME = 0x21
_TLV_CODE_SERIAL_NUMBER = 0x23
_TLV_CODE_MAC_BASE = 0x24
_TLV_CODE_CRC_32 = 0xFE

EEPROM_PATH = "/sys/bus/i2c/devices/10-0050/eeprom"


def _install_stubs():
    """Install minimal sonic_platform_base stubs, returning keys we created.

    switch_host_module imports ModuleBase and TlvInfoDecoder at load time. The
    stubs only need to exist in sys.modules while the module executes; the module
    captures the class references in its own globals. Returning the list of keys
    we added lets the caller restore sys.modules afterwards so this suite does not
    leak a partial stub that would shadow sibling tests (e.g. tests/test_watchdog.py,
    which installs its own sonic_platform_base.watchdog_base stub).
    """
    created = []

    def ensure(name):
        if name in sys.modules:
            return sys.modules[name], False
        mod = types.ModuleType(name)
        sys.modules[name] = mod
        created.append(name)
        return mod, True

    base, _ = ensure("sonic_platform_base")
    module_base, _ = ensure("sonic_platform_base.module_base")
    if not hasattr(module_base, "ModuleBase"):
        class ModuleBase:
            MODULE_TYPE_SWITCH_HOST = "SWITCH_HOST"
            MODULE_STATUS_ONLINE = "Online"
            MODULE_STATUS_OFFLINE = "Offline"
            MODULE_STATUS_FAULT = "Fault"

            def __init__(self, *args, **kwargs):
                pass

        module_base.ModuleBase = ModuleBase
    base.module_base = module_base

    sonic_eeprom, _ = ensure("sonic_platform_base.sonic_eeprom")
    base.sonic_eeprom = sonic_eeprom
    eeprom_tlvinfo, _ = ensure("sonic_platform_base.sonic_eeprom.eeprom_tlvinfo")
    if not hasattr(eeprom_tlvinfo, "TlvInfoDecoder"):
        class TlvInfoDecoder:
            _TLV_CODE_PRODUCT_NAME = _TLV_CODE_PRODUCT_NAME
            _TLV_CODE_SERIAL_NUMBER = _TLV_CODE_SERIAL_NUMBER
            _TLV_CODE_MAC_BASE = _TLV_CODE_MAC_BASE
            _TLV_CODE_CRC_32 = _TLV_CODE_CRC_32

        eeprom_tlvinfo.TlvInfoDecoder = TlvInfoDecoder
    sonic_eeprom.eeprom_tlvinfo = eeprom_tlvinfo

    return created


def _load():
    created = _install_stubs()
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    mod_path = os.path.join(os.path.dirname(tests_dir), "sonic_platform",
                            "switch_host_module.py")
    spec = importlib.util.spec_from_file_location(
        "switch_host_module_under_test", mod_path)
    assert spec is not None and spec.loader is not None, \
        "cannot load {}".format(mod_path)
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    finally:
        for name in created:
            sys.modules.pop(name, None)
    return module


shm = _load()


def _make_tlvinfo(tlvs):
    """Build an ONIE TlvInfo blob from a list of (code, value_bytes)."""
    body = b"".join(bytes([code, len(val)]) + val for code, val in tlvs)
    total_len = len(body)
    header = b"TlvInfo\x00" + bytes(
        [0x01, (total_len >> 8) & 0xFF, total_len & 0xFF])
    return header + body


def _module():
    return shm.SwitchHostModule()


def test_read_eeprom_tlv_returns_requested_value():
    blob = _make_tlvinfo([
        (_TLV_CODE_PRODUCT_NAME, b"NH-B27"),
        (_TLV_CODE_SERIAL_NUMBER, b"SN12345"),
        (_TLV_CODE_CRC_32, b"\x00\x00\x00\x00"),
    ])
    with mock.patch("builtins.open", mock.mock_open(read_data=blob)):
        assert _module()._read_eeprom_tlv(_TLV_CODE_SERIAL_NUMBER) == "SN12345"


def test_get_serial_reads_serial_tlv():
    blob = _make_tlvinfo([(_TLV_CODE_SERIAL_NUMBER, b"ABC-999")])
    with mock.patch("builtins.open", mock.mock_open(read_data=blob)):
        assert _module().get_serial() == "ABC-999"


def test_read_eeprom_tlv_missing_tlv_returns_na():
    blob = _make_tlvinfo([
        (_TLV_CODE_PRODUCT_NAME, b"NH-B27"),
        (_TLV_CODE_CRC_32, b"\x00\x00\x00\x00"),
    ])
    with mock.patch("builtins.open", mock.mock_open(read_data=blob)):
        assert _module()._read_eeprom_tlv(_TLV_CODE_SERIAL_NUMBER) == "N/A"


def test_read_eeprom_tlv_stops_at_crc():
    # A serial TLV placed after the CRC must not be returned.
    blob = _make_tlvinfo([
        (_TLV_CODE_CRC_32, b"\x00\x00\x00\x00"),
        (_TLV_CODE_SERIAL_NUMBER, b"LATE"),
    ])
    with mock.patch("builtins.open", mock.mock_open(read_data=blob)):
        assert _module()._read_eeprom_tlv(_TLV_CODE_SERIAL_NUMBER) == "N/A"


def test_read_eeprom_tlv_invalid_header_returns_na():
    with mock.patch("builtins.open", mock.mock_open(read_data=b"NOTTLV\x00\x00")):
        assert _module()._read_eeprom_tlv(_TLV_CODE_SERIAL_NUMBER) == "N/A"


def test_read_eeprom_tlv_read_error_returns_na():
    with mock.patch("builtins.open", side_effect=OSError("no device")):
        assert _module()._read_eeprom_tlv(_TLV_CODE_SERIAL_NUMBER) == "N/A"


# --------------------------------------------------------------------------
# Switch CPU power control (gpioset/gpioget)
# --------------------------------------------------------------------------

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

#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the CF2 read_mgmt_switch_version chip-selection and cache logic."""

import importlib.machinery
import importlib.util
import os
import sys

import pytest


sys.dont_write_bytecode = True

MV88E6341_PLATFORM = "x86_64-nexthop_4210-r0021"
MV88E6191X_PLATFORM = "x86_64-nexthop_4210-r1032"


@pytest.fixture(scope="function")
def script(monkeypatch, tmp_path):
    test_dir = os.path.dirname(os.path.realpath(__file__))
    script_path = os.path.join(test_dir, "../../../cf2/utils/read_mgmt_switch_version.py")
    loader = importlib.machinery.SourceFileLoader("read_mgmt_switch_version", script_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[loader.name] = module
    try:
        spec.loader.exec_module(module)
        monkeypatch.setattr(module, "CACHE_FILE", str(tmp_path / "mgmt_switch_version"))
        monkeypatch.setattr(module, "LOCK_FILE", str(tmp_path / "lock" / "mgmt_switch_version.lock"))
        module._syslog_calls = []
        monkeypatch.setattr(module.syslog, "syslog", lambda prio, msg: module._syslog_calls.append((prio, msg)))
        monkeypatch.setattr(module.os, "geteuid", lambda: 0)
        monkeypatch.setattr(module.device_info, "get_platform", lambda **kw: MV88E6191X_PLATFORM)
        yield module
    finally:
        sys.modules.pop(loader.name, None)


class FakeSmi:
    def __init__(self, switch_id, version="1.4", timeout=False, absent=False):
        self._switch_id = switch_id
        self._version = version
        self._timeout = timeout
        self._absent = absent

    def read_switch_id(self, port0_device):
        from nexthop import mdio_lib

        if self._timeout:
            raise mdio_lib.SmiTimeoutError("SMIBusy did not clear")
        if self._absent:
            raise mdio_lib.SmiNoDeviceError("no device at MDIO address 0x1E")
        return self._switch_id

    def read_config_version(self):
        return self._version


def scratch_image(major, minor):
    """Minimal 88E6191X-style image: two scratch writes in the order real images use."""
    def write(value):
        return b"\x21" + value.to_bytes(2, "little") + b"\xe5\x21\x1c\x1a\xe5\xcd\xe2\x04\xf1\xf1"

    return b"\xed\x00" * 8 + write(0x0000) + write(0x8100 | minor) + write(0x8000 | major) + b"\xc9"


def run_main(script, monkeypatch, capsys, argv=()):
    monkeypatch.setattr(sys, "argv", ["read_mgmt_switch_version.py", *argv])
    rc = script.main()
    return rc, capsys.readouterr().out.strip()


def test_88e6191x_default_decodes_version_from_eeprom_image(script, monkeypatch, capsys):
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: scratch_image(1, 4))
    monkeypatch.setattr(script, "_make_smi", lambda: pytest.fail("MDIO must not run by default"))
    monkeypatch.setattr(script, "_read_88e6341_version", lambda: pytest.fail("88E6341 path must not run"))
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "1.4")
    assert open(script.CACHE_FILE).read() == "1.4"


def test_88e6191x_mdio_flag_reads_live_scratch_bytes(script, monkeypatch, capsys):
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: pytest.fail("EEPROM must not be read with --mdio"))
    monkeypatch.setattr(script, "_make_smi", lambda: FakeSmi(script.MV88E6191X_SWITCH_ID, "1.4"))
    rc, out = run_main(script, monkeypatch, capsys, argv=["--mdio"])
    assert (rc, out) == (0, "1.4")
    assert open(script.CACHE_FILE).read() == "1.4"


@pytest.mark.parametrize("major, minor", [(1, 3), (1, 4), (4, 1)])
def test_decode_88e6191x_version_from_image(script, major, minor):
    assert script._decode_88e6191x_version(scratch_image(major, minor)) == f"{major}.{minor}"


def test_decode_88e6191x_version_rejects_image_without_stamp(script):
    with pytest.raises(ValueError):
        script._decode_88e6191x_version(b"\xed\x00" * 64 + b"\x21\x00\x00\xe5\x21\x1c\x1a\xe5\xcd")


def test_88e6341_card_reads_eeprom_without_touching_mdio(script, monkeypatch, capsys):
    monkeypatch.setattr(script.device_info, "get_platform", lambda **kw: MV88E6341_PLATFORM)
    monkeypatch.setattr(script, "_make_smi", lambda: pytest.fail("MDIO must not run on an 88E6341 card"))
    monkeypatch.setattr(script, "_read_88e6341_version", lambda: (2, 3))
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "0.2")


def test_88e6341_vlan_flag_selects_vlan_byte_and_separate_cache(script, monkeypatch, capsys):
    monkeypatch.setattr(script.device_info, "get_platform", lambda **kw: MV88E6341_PLATFORM)
    monkeypatch.setattr(script, "_read_88e6341_version", lambda: (2, 3))
    rc, out = run_main(script, monkeypatch, capsys, argv=["--vlan"])
    assert (rc, out) == (0, "0.3")
    assert open(script.CACHE_FILE + "_vlan").read() == "0.3"
    assert not os.path.exists(script.CACHE_FILE)


@pytest.mark.parametrize("smi_kwargs", [{"timeout": True}, {"absent": True}], ids=["smi-busy-stuck", "bus-reads-all-ones"])
def test_mdio_unavailable_reports_na_and_does_not_cache_or_touch_eeprom(script, monkeypatch, capsys, smi_kwargs):
    monkeypatch.setattr(script, "_make_smi", lambda: FakeSmi(0, **smi_kwargs))
    monkeypatch.setattr(script, "_read_88e6341_version", lambda: pytest.fail("EEPROM path must not run"))
    rc, out = run_main(script, monkeypatch, capsys, argv=["--mdio"])
    assert (rc, out) == (0, "N/A")
    assert not os.path.exists(script.CACHE_FILE)


def test_wrong_switch_id_on_88e6191x_platform_reports_na_and_logs_error(script, monkeypatch, capsys):
    monkeypatch.setattr(script, "_make_smi", lambda: FakeSmi(0x3410))
    monkeypatch.setattr(script, "_read_88e6341_version", lambda: pytest.fail("EEPROM path must not run"))
    rc, out = run_main(script, monkeypatch, capsys, argv=["--mdio"])
    assert (rc, out) == (0, "N/A")
    assert not os.path.exists(script.CACHE_FILE)
    assert [(p, "0x3410" in m) for p, m in script._syslog_calls] == [(script.syslog.LOG_ERR, True)]


@pytest.mark.parametrize(
    "exc, phrase",
    [
        (ImportError("No module named 'pddfparse'"), "pddfparse not importable"),
        (ValueError("no version stamp"), "no recognizable version stamp"),
        (FileNotFoundError(2, "No such file", "/sys/bus/i2c/devices/12-0050/eeprom"), "sysfs node missing"),
        (OSError(19, "No such device"), "FPGA or i2c access failed"),
        (RuntimeError("no eeprom path for MGMT-SWCH-EEPROM"), "platform config error"),
        (KeyError("CPUCARD_FPGA"), "unexpected KeyError"),
    ],
    ids=lambda v: type(v).__name__ if isinstance(v, Exception) else None,
)
def test_any_read_failure_prints_na_and_logs_err(script, monkeypatch, capsys, exc, phrase):
    def boom():
        raise exc

    monkeypatch.setattr(script, "_read_88e6191x_image", boom)
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "N/A")
    assert not os.path.exists(script.CACHE_FILE)
    (prio, msg), = script._syslog_calls
    assert prio == script.syslog.LOG_ERR
    assert phrase in msg


def test_cache_write_failure_still_prints_version(script, monkeypatch, capsys):
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: scratch_image(1, 3))
    monkeypatch.setattr(script, "CACHE_FILE", "/nonexistent-dir/mgmt_switch_version")
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "1.3")
    assert script._syslog_calls[0][0] == script.syslog.LOG_WARNING


@pytest.mark.parametrize(
    "platform, has_88e6191x",
    [
        ("x86_64-nexthop_4210-r0021", False),
        ("x86_64-nexthop_4210-r1032", True),
        ("x86_64-nexthop_4210-r1042", True),
        ("x86_64-nexthop_4210_128p-r1030", True),
        ("x86_64-nexthop_4210-r0", True),
        ("x86_64-nexthop_4210-r1", True),
        (None, True),
    ],
)
def test_has_88e6191x_from_cpu_card_field_or_ga_scheme(script, platform, has_88e6191x):
    assert script._has_88e6191x(platform) is has_88e6191x


def test_cache_hit_skips_hardware(script, monkeypatch, capsys):
    with open(script.CACHE_FILE, "w") as f:
        f.write("1.1\n")
    monkeypatch.setattr(script, "_make_smi", lambda: pytest.fail("MDIO must not run"))
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: pytest.fail("EEPROM must not be read"))
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "1.1")


def test_no_cache_flag_bypasses_stale_cache_and_refreshes_it(script, monkeypatch, capsys):
    with open(script.CACHE_FILE, "w") as f:
        f.write("1.3\n")
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: scratch_image(1, 4))
    rc, out = run_main(script, monkeypatch, capsys, argv=["--no-cache"])
    assert (rc, out) == (0, "1.4")
    assert open(script.CACHE_FILE).read() == "1.4"


def test_non_root_cache_hit_does_not_escalate(script, monkeypatch, capsys):
    with open(script.CACHE_FILE, "w") as f:
        f.write("1.3")
    monkeypatch.setattr(script.os, "geteuid", lambda: 1000)
    monkeypatch.setattr(script.subprocess, "call", lambda *a, **k: pytest.fail("must not sudo on a cache hit"))
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "1.3")


def test_non_root_cache_miss_reexecs_with_sudo(script, monkeypatch, capsys):
    calls = []
    monkeypatch.setattr(script.os, "geteuid", lambda: 1000)
    monkeypatch.setattr(script.subprocess, "call", lambda cmd: calls.append(cmd) or 0)
    monkeypatch.setattr(script, "_read_88e6191x_image", lambda: pytest.fail("hardware must not be read unprivileged"))
    rc, _ = run_main(script, monkeypatch, capsys, argv=["--no-cache"])
    assert rc == 0
    assert calls == [["sudo", "read_mgmt_switch_version.py", "--no-cache"]]


def test_mdio_path_takes_lock(script, monkeypatch, capsys):
    monkeypatch.setattr(script, "_make_smi", lambda: FakeSmi(script.MV88E6191X_SWITCH_ID, "1.1"))
    run_main(script, monkeypatch, capsys, argv=["--mdio"])
    assert os.path.exists(script.LOCK_FILE)


def test_eeprom_path_takes_lock_around_pddf_and_gate(script, monkeypatch, capsys):
    events = []

    class FakePddf:
        def create_subtree(self, name):
            events.append(("create", name))

        def delete_subtree(self, name):
            events.append(("delete", name))

        def get_path(self, name, attr):
            return "/dev/null"

    def fake_flock(fd, op):
        events.append(("flock", op))

    monkeypatch.setattr(script, "_pddfparse", lambda: FakePddf())
    monkeypatch.setattr(script, "_cpu_fpga_mgmt_toggle", lambda v: events.append(("gate", v)))
    monkeypatch.setattr(script.fcntl, "flock", fake_flock)
    monkeypatch.setattr(script, "_decode_88e6191x_version", lambda image: "1.3")
    rc, out = run_main(script, monkeypatch, capsys)
    assert (rc, out) == (0, "1.3")
    assert events == [
        ("flock", script.fcntl.LOCK_EX),
        ("gate", 1),
        ("create", "MGMT-SWCH-EEPROM"),
        ("delete", "MGMT-SWCH-EEPROM"),
        ("gate", 0),
    ]

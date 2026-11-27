# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import binascii
import errno
import io
import os
import struct
import sys
import types
from unittest.mock import Mock

import pytest

# conftest mocks these; the IDPROM reader needs the real ONIE decoder.
_REAL_EEPROM_MODULES = (
    "sonic_platform_base.sonic_eeprom",
    "sonic_platform_base.sonic_eeprom.eeprom_tlvinfo",
)
SONIC_PLATFORM_BASE_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "../../../../../../src/sonic-platform-common/sonic_platform_base")
)
# A 24c64 IDPROM, as exposed by at24.
IDPROM_PART_SIZE = 8192


@pytest.fixture
def nexthop_device_info(monkeypatch, tmp_path):
    sonic_platform_base = types.ModuleType("sonic_platform_base")
    sonic_platform_base.__path__ = [SONIC_PLATFORM_BASE_PATH]
    monkeypatch.setitem(sys.modules, "sonic_platform_base", sonic_platform_base)
    for name in _REAL_EEPROM_MODULES:
        monkeypatch.delitem(sys.modules, name, raising=False)
    monkeypatch.setitem(sys.modules, "redis", Mock())
    from nexthop import nexthop_device_info

    monkeypatch.setattr(nexthop_device_info, "CPU_CARD_IDPROM_GLOB", str(tmp_path / "i2c-PRP0001:*" / "eeprom"))
    monkeypatch.setattr(nexthop_device_info, "CPU_CARD_IDPROM_CACHE", str(tmp_path / "cache" / "cpucard_idprom"))
    yield nexthop_device_info


@pytest.fixture
def machine_conf(nexthop_device_info, monkeypatch):
    conf = {}
    monkeypatch.setattr(nexthop_device_info.device_info, "get_machine_info", lambda: conf)
    yield conf


@pytest.fixture
def syslog_mock(nexthop_device_info, monkeypatch):
    mock = Mock()
    monkeypatch.setattr(nexthop_device_info.syslog, "syslog", mock)
    yield mock


def tlvinfo(*tlvs):
    body = b"".join(bytes([code, len(value)]) + value for code, value in tlvs)
    blob = b"TlvInfo\x00\x01" + struct.pack(">H", len(body) + 6) + body + b"\xfe\x04"
    return blob + struct.pack(">I", binascii.crc32(blob) & 0xFFFFFFFF)


@pytest.fixture
def idprom(tmp_path):
    def write(data, name="i2c-PRP0001:00"):
        os.makedirs(tmp_path / name)
        path = tmp_path / name / "eeprom"
        path.write_bytes(data + b"\xff" * (IDPROM_PART_SIZE - len(data)))
        return str(path)

    yield write


def test_machine_conf_value(nexthop_device_info, machine_conf):
    machine_conf["onie_platform"] = "x86_64-nexthop_4010-r0"

    assert nexthop_device_info.get_machine_conf_value("onie_platform") == "x86_64-nexthop_4010-r0"


def test_machine_conf_missing_key(nexthop_device_info, machine_conf):
    assert nexthop_device_info.get_machine_conf_value("onie_device_version") is None


def test_machine_conf_missing_file(nexthop_device_info, monkeypatch):
    monkeypatch.setattr(nexthop_device_info.device_info, "get_machine_info", lambda: None)

    assert nexthop_device_info.get_machine_conf_value("onie_platform") is None


def test_idprom_arbitrary_field(nexthop_device_info, idprom):
    idprom(tlvinfo((0x21, b"NH-4010"), (0x23, b"SN123"), (0x26, b"\x02")))

    assert nexthop_device_info.get_cpu_card_idprom_field(0x21) == b"NH-4010"
    assert nexthop_device_info.get_cpu_card_idprom_field(0x23) == b"SN123"


def test_idprom_missing_field_raises(nexthop_device_info, idprom):
    idprom(tlvinfo((0x21, b"NH-4010")))

    with pytest.raises(RuntimeError):
        nexthop_device_info.get_cpu_card_idprom_field(0x26)


def test_no_idprom_raises_not_found(nexthop_device_info):
    with pytest.raises(FileNotFoundError):
        nexthop_device_info.read_cpu_card_idprom()


def test_bad_crc_raises(nexthop_device_info, idprom):
    data = bytearray(tlvinfo((0x26, b"\x02")))
    data[-1] ^= 0xFF
    idprom(bytes(data))

    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()


def test_blank_idprom_raises(nexthop_device_info, idprom):
    idprom(b"")

    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()


def test_io_error_raises(nexthop_device_info, idprom, monkeypatch):
    path = idprom(tlvinfo((0x26, b"\x02")))
    real_open = io.open

    def failing_open(file, mode="r", *args, **kwargs):
        if str(file) == path:
            raise OSError(errno.EIO, "Input/output error")
        return real_open(file, mode, *args, **kwargs)

    monkeypatch.setattr(io, "open", failing_open)
    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()


def test_multiple_idproms_raises(nexthop_device_info, idprom):
    idprom(tlvinfo((0x26, b"\x02")), name="i2c-PRP0001:00")
    idprom(tlvinfo((0x26, b"\x05")), name="i2c-PRP0001:01")

    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()


def test_idprom_read_is_cached(nexthop_device_info, idprom, monkeypatch):
    path = idprom(tlvinfo((0x26, b"\x02")))
    assert nexthop_device_info.get_device_version(skip_machine_conf=True) == 2

    with open(path, "r+b") as f:
        f.write(tlvinfo((0x26, b"\x05")))
    monkeypatch.setattr(nexthop_device_info.eeprom_utils.Eeprom, "set_cache_name", Mock())  # the device lock
    assert nexthop_device_info.get_device_version(skip_machine_conf=True) == 2
    nexthop_device_info.eeprom_utils.Eeprom.set_cache_name.assert_not_called()


def test_cache_write_failure_does_not_fail_the_read(nexthop_device_info, idprom, syslog_mock, monkeypatch):
    idprom(tlvinfo((0x26, b"\x02")))

    def failing_write_cache(self, e):
        raise OSError("No space left on device")

    monkeypatch.setattr(nexthop_device_info.eeprom_utils.Eeprom, "write_cache", failing_write_cache)
    assert nexthop_device_info.get_device_version(skip_machine_conf=True) == 2
    assert syslog_mock.call_args.args[0] == nexthop_device_info.syslog.LOG_WARNING


def test_invalid_idprom_is_not_cached(nexthop_device_info, idprom):
    idprom(b"")

    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()
    assert not os.path.exists(nexthop_device_info.CPU_CARD_IDPROM_CACHE)


def test_invalid_cache_is_removed(nexthop_device_info, idprom):
    idprom(tlvinfo((0x26, b"\x02")))
    cache = nexthop_device_info.CPU_CARD_IDPROM_CACHE
    os.makedirs(os.path.dirname(cache))
    with open(cache, "wb") as f:
        f.write(b"\xff" * 64)

    with pytest.raises(RuntimeError):
        nexthop_device_info.read_cpu_card_idprom()
    assert not os.path.exists(cache)
    assert nexthop_device_info.get_device_version(skip_machine_conf=True) == 2


@pytest.mark.parametrize("value, expected", [("0", 0), ("3", 3)])
def test_device_version_prefers_machine_conf(nexthop_device_info, machine_conf, idprom, value, expected):
    machine_conf["onie_device_version"] = value
    idprom(tlvinfo((0x26, b"\x07")))

    assert nexthop_device_info.get_device_version() == expected


def test_device_version_from_machine_conf_does_not_read_the_idprom(nexthop_device_info, machine_conf):
    machine_conf["onie_device_version"] = "1"

    assert nexthop_device_info.get_device_version() == 1


@pytest.mark.parametrize("conf", [{}, {"onie_device_version": ""}, {"onie_device_version": "ERROR: TLV"}])
def test_device_version_falls_back_to_idprom_and_logs_info(nexthop_device_info, machine_conf, idprom, syslog_mock, conf):
    machine_conf.update(conf)
    idprom(tlvinfo((0x21, b"NH-4010"), (0x26, b"\x02")))

    assert nexthop_device_info.get_device_version() == 2
    assert syslog_mock.call_args.args[0] == nexthop_device_info.syslog.LOG_INFO


def test_device_version_skip_machine_conf_reads_only_idprom(nexthop_device_info, machine_conf, idprom, syslog_mock):
    machine_conf["onie_device_version"] = "3"
    idprom(tlvinfo((0x26, b"\x02")))

    assert nexthop_device_info.get_device_version(skip_machine_conf=True) == 2
    syslog_mock.assert_not_called()


def test_device_version_not_programmed_raises(nexthop_device_info, machine_conf, idprom):
    idprom(tlvinfo((0x21, b"NH-4010")))

    with pytest.raises(RuntimeError):
        nexthop_device_info.get_device_version()


@pytest.mark.parametrize("value", [b"", b"\x01\x02"])
def test_device_version_wrong_length_raises(nexthop_device_info, machine_conf, idprom, value):
    idprom(tlvinfo((0x26, value)))

    with pytest.raises(ValueError):
        nexthop_device_info.get_device_version()

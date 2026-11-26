"""Unit tests for eeprom.py.

Self-contained by design: nexthop_utils.eeprom_utils is stubbed and eeprom.py is
loaded directly by path, so neither the real decoder nor the BMC IDPROM is
required. The stub decodes every TLV as ASCII text, which is enough to check how
system_eeprom_info() walks the TlvInfo image.
"""

import importlib.util
import os
import sys
import types
from unittest import mock

import pytest

_TLV_CODE_PRODUCT_NAME = 0x21
_TLV_CODE_SERIAL_NUMBER = 0x23
_TLV_CODE_VENDOR_EXT = 0xFD
_TLV_CODE_CRC_32 = 0xFE

_MODULE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sonic_platform", "eeprom.py")


class _StubEeprom:
    _TLV_INFO_HDR_LEN = 11
    _TLV_CODE_VENDOR_EXT = _TLV_CODE_VENDOR_EXT
    _TLV_CODE_CRC_32 = _TLV_CODE_CRC_32

    def __init__(self, *args, **kwargs):
        pass

    def read_eeprom(self):
        raise NotImplementedError

    def is_valid_tlvinfo_header(self, e):
        return len(e) >= self._TLV_INFO_HDR_LEN and bytes(e[0:8]) == b"TlvInfo\x00"

    def decoder(self, s, t):
        return "name", bytes(t[2:]).decode("ascii")


@pytest.fixture(scope="module")
def eeprom_cls():
    stubs = {"nexthop_utils": types.ModuleType("nexthop_utils"), "nexthop_utils.eeprom_utils": types.ModuleType("nexthop_utils.eeprom_utils")}
    stubs["nexthop_utils.eeprom_utils"].Eeprom = _StubEeprom
    stubs["nexthop_utils"].eeprom_utils = stubs["nexthop_utils.eeprom_utils"]
    with mock.patch.dict(sys.modules, stubs):
        spec = importlib.util.spec_from_file_location("nexthop_bmc_eeprom_under_test", _MODULE_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    return module.Eeprom


def _tlv(code, value):
    return bytes([code, len(value)]) + value


def _image(*tlvs):
    body = b"".join(tlvs)
    return bytearray(b"TlvInfo\x00" + b"\x01" + len(body).to_bytes(2, "big") + body)


def _eeprom(eeprom_cls, image=None, read_error=None):
    with mock.patch("os.path.exists", return_value=True):
        eeprom = eeprom_cls()
    eeprom.read_eeprom = mock.Mock(return_value=image, side_effect=read_error)
    return eeprom


def test_system_eeprom_info_is_keyed_by_type_code(eeprom_cls):
    image = _image(_tlv(_TLV_CODE_PRODUCT_NAME, b"NH-BMC"), _tlv(_TLV_CODE_SERIAL_NUMBER, b"SN123"), _tlv(_TLV_CODE_CRC_32, b"\x41\x42\x43\x44"))
    assert _eeprom(eeprom_cls, image).system_eeprom_info() == {"0x21": "NH-BMC", "0x23": "SN123", "0xFE": "ABCD"}


def test_system_eeprom_info_lists_every_vendor_extension(eeprom_cls):
    image = _image(_tlv(_TLV_CODE_VENDOR_EXT, b"\x00\x01"), _tlv(_TLV_CODE_VENDOR_EXT, b"\xab"), _tlv(_TLV_CODE_CRC_32, b"ABCD"))
    assert _eeprom(eeprom_cls, image).system_eeprom_info()["0xFD"] == ["0x00 0x01", "0xAB"]


def test_system_eeprom_info_stops_at_the_crc(eeprom_cls):
    image = _image(_tlv(_TLV_CODE_CRC_32, b"ABCD"), _tlv(_TLV_CODE_PRODUCT_NAME, b"after"))
    assert _eeprom(eeprom_cls, image).system_eeprom_info() == {"0xFE": "ABCD"}


def test_system_eeprom_info_drops_a_tlv_that_runs_past_the_image(eeprom_cls):
    image = _image(_tlv(_TLV_CODE_PRODUCT_NAME, b"NH-BMC"))
    image += bytes([_TLV_CODE_SERIAL_NUMBER, 40]) + b"short"
    image[9:11] = (len(image) - 11).to_bytes(2, "big")
    assert _eeprom(eeprom_cls, image).system_eeprom_info() == {"0x21": "NH-BMC"}


def test_system_eeprom_info_is_empty_without_a_tlvinfo_header(eeprom_cls):
    assert _eeprom(eeprom_cls, bytearray(b"\xff" * 64)).system_eeprom_info() == {}


def test_system_eeprom_info_is_empty_when_the_read_fails(eeprom_cls):
    assert _eeprom(eeprom_cls, read_error=OSError("no device")).system_eeprom_info() == {}

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

import subprocess
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import pytest

from sonic_platform.eeprom import (
    EEPROM_BMC_PATH,
    EEPROM_SYSTEM_PATH,
    Eeprom,
    EepromBMC,
    EepromSystem,
    IpmiFru,
)


# Sample stdout from `ipmi-fru --fru-file=<path>`. The parser keys off the labels
# listed in `IpmiFru.PARSING_RULES`.
IPMI_FRU_OUTPUT = (
    "FRU Inventory Area Size: 256 bytes\n"
    "FRU Board Product Name: AST2700-BMC\n"
    "FRU Board Part Number: MBF-AST2700-001\n"
    "FRU Board Serial Number: SN1234567\n"
    "FRU Board Custom Info: MAC: aa:bb:cc:dd:ee:ff\n"
    "FRU Product Version: A0\n"
)


@pytest.fixture(autouse=True)
def _isolate_ipmi_fru_cache():
    """
    Reset the `functools.cache` on `IpmiFru.get_tlv_dict` between tests.

    The cache is per-instance so this is normally safe, but mocking `subprocess.run`
    means a stale cached entry could leak if a test reused the same instance.
    """
    IpmiFru.get_tlv_dict.cache_clear()
    yield
    IpmiFru.get_tlv_dict.cache_clear()


class TestIpmiFru:

    def test_get_fru_info_invokes_ipmi_fru(self):
        fru = IpmiFru("/var/run/fake")
        fake_result = SimpleNamespace(stdout=IPMI_FRU_OUTPUT, stderr="", returncode=0)
        with patch("sonic_platform.eeprom.subprocess.run", return_value=fake_result) as sp:
            assert fru.get_fru_info() == IPMI_FRU_OUTPUT
        sp.assert_called_once()
        cmd = sp.call_args[0][0]
        assert "ipmi-fru" in cmd
        assert "/var/run/fake" in cmd

    def test_get_fru_info_returns_empty_on_nonzero_exit(self):
        fru = IpmiFru("/var/run/fake")
        fake_result = SimpleNamespace(stdout="garbage", stderr="boom", returncode=2)
        with patch("sonic_platform.eeprom.subprocess.run", return_value=fake_result):
            assert fru.get_fru_info() == ""

    def test_get_fru_info_returns_empty_when_binary_missing(self):
        fru = IpmiFru("/var/run/fake")
        with patch("sonic_platform.eeprom.subprocess.run", side_effect=FileNotFoundError("ipmi-fru")):
            assert fru.get_fru_info() == ""

    def test_get_fru_info_returns_empty_on_timeout(self):
        fru = IpmiFru("/var/run/fake")
        timeout_exc = subprocess.TimeoutExpired(cmd="ipmi-fru", timeout=fru.IPMI_FRU_TIMEOUT)
        with patch("sonic_platform.eeprom.subprocess.run", side_effect=timeout_exc):
            assert fru.get_fru_info() == ""

    def test_get_tlv_list_contains_expected_codes(self):
        fru = IpmiFru("/dev/null")
        tlv_list = fru.get_tlv_list()
        assert Eeprom._TLV_CODE_PRODUCT_NAME in tlv_list
        assert Eeprom._TLV_CODE_PART_NUMBER in tlv_list
        assert Eeprom._TLV_CODE_SERIAL_NUMBER in tlv_list
        assert Eeprom._TLV_CODE_MAC_BASE in tlv_list
        assert Eeprom._TLV_CODE_LABEL_REVISION in tlv_list

    def test_get_tlv_dict_parses_output(self):
        fru = IpmiFru("/dev/null")
        with patch.object(fru, "get_fru_info", return_value=IPMI_FRU_OUTPUT):
            tlvs = fru.get_tlv_dict()
        assert tlvs[Eeprom._TLV_CODE_PRODUCT_NAME] == "AST2700-BMC"
        assert tlvs[Eeprom._TLV_CODE_PART_NUMBER] == "MBF-AST2700-001"
        assert tlvs[Eeprom._TLV_CODE_SERIAL_NUMBER] == "SN1234567"
        assert tlvs[Eeprom._TLV_CODE_MAC_BASE] == "aa:bb:cc:dd:ee:ff"
        assert tlvs[Eeprom._TLV_CODE_LABEL_REVISION] == "A0"

    def test_get_tlv_dict_ignores_unknown_lines(self):
        fru = IpmiFru("/dev/null")
        noisy = "Random line\nAnother: thing\nFRU Board Serial Number: XYZ\n"
        with patch.object(fru, "get_fru_info", return_value=noisy):
            tlvs = fru.get_tlv_dict()
        assert tlvs == {Eeprom._TLV_CODE_SERIAL_NUMBER: "XYZ"}


class TestEeprom:

    def _make_eeprom(self, available_in_redis=False):
        # The `TlvInfoDecoder` base class reaches for sonic-db; the `Eeprom` subclass
        # only accesses it through `_redis_hget` which we override in each test.
        return Eeprom("/var/run/fake-fru", available_in_redis=available_in_redis)

    def test_eeprom_dict_get_returns_fru_dict_when_redis_disabled(self):
        eeprom = self._make_eeprom(available_in_redis=False)
        with patch.object(eeprom.fru, "get_tlv_dict", return_value={Eeprom._TLV_CODE_PRODUCT_NAME: "P"}):
            data = eeprom._eeprom_dict_get()
        assert data == {Eeprom._TLV_CODE_PRODUCT_NAME: "P"}

    def test_eeprom_dict_get_falls_back_when_redis_uninitialized(self):
        eeprom = self._make_eeprom(available_in_redis=True)
        eeprom._redis_hget = MagicMock(return_value="0")
        with patch.object(eeprom.fru, "get_tlv_dict", return_value={Eeprom._TLV_CODE_SERIAL_NUMBER: "S"}):
            data = eeprom._eeprom_dict_get()
        assert data == {Eeprom._TLV_CODE_SERIAL_NUMBER: "S"}

    def test_eeprom_init_populates_info_and_raw(self):
        eeprom = self._make_eeprom()
        tlv_dict = {
            Eeprom._TLV_CODE_PRODUCT_NAME: "AST2700-BMC",
            Eeprom._TLV_CODE_PART_NUMBER: "MBF-AST2700-001",
            Eeprom._TLV_CODE_SERIAL_NUMBER: "SN1234567",
            Eeprom._TLV_CODE_MAC_BASE: "aa:bb:cc:dd:ee:ff",
            Eeprom._TLV_CODE_LABEL_REVISION: "A0",
        }
        with patch.object(eeprom, "_eeprom_dict_get", return_value=tlv_dict):
            eeprom._eeprom_init()

        assert eeprom._eeprom_raw is not None
        assert eeprom._eeprom_raw.startswith(Eeprom._TLV_INFO_ID_STRING)

        info = eeprom._eeprom_info_dict
        assert info[hex(Eeprom._TLV_CODE_PRODUCT_NAME)] == "AST2700-BMC"
        assert info[hex(Eeprom._TLV_CODE_PART_NUMBER)] == "MBF-AST2700-001"
        assert info[hex(Eeprom._TLV_CODE_SERIAL_NUMBER)] == "SN1234567"
        assert info[hex(Eeprom._TLV_CODE_MAC_BASE)] == "aa:bb:cc:dd:ee:ff"
        assert info[hex(Eeprom._TLV_CODE_LABEL_REVISION)] == "A0"
        # The checksum is appended as a CRC32 entry in ONIE hex format.
        crc_key = hex(Eeprom._TLV_CODE_CRC_32)
        assert crc_key in info
        crc_value = info[crc_key]
        assert isinstance(crc_value, str)
        assert crc_value == f"0x{int(crc_value, 16):08X}"

    def test_getter_helpers_use_system_eeprom_info(self):
        eeprom = self._make_eeprom()
        info = {
            hex(Eeprom._TLV_CODE_PRODUCT_NAME): "prod",
            hex(Eeprom._TLV_CODE_PART_NUMBER): "part",
            hex(Eeprom._TLV_CODE_SERIAL_NUMBER): "serial",
            hex(Eeprom._TLV_CODE_MAC_BASE): "11:22:33:44:55:66",
            hex(Eeprom._TLV_CODE_LABEL_REVISION): "A0",
        }
        eeprom._eeprom_info_dict = info

        assert eeprom.get_product_name() == "prod"
        assert eeprom.get_part_number() == "part"
        assert eeprom.get_serial_number() == "serial"
        assert eeprom.get_base_mac() == "11:22:33:44:55:66"
        assert eeprom.get_revision() == "A0"

    def test_getters_return_none_when_code_missing(self):
        eeprom = self._make_eeprom()
        eeprom._eeprom_info_dict = {}
        assert eeprom.get_product_name() is None
        assert eeprom.get_part_number() is None
        assert eeprom.get_serial_number() is None
        assert eeprom.get_base_mac() is None
        assert eeprom.get_revision() is None

    def test_read_eeprom_caches_raw_buffer(self):
        eeprom = self._make_eeprom()
        with patch.object(eeprom, "_eeprom_init") as init:
            def fake_init():
                eeprom._eeprom_raw = b"RAW"
                eeprom._eeprom_info_dict = {}
            init.side_effect = fake_init

            first = eeprom.read_eeprom()
            second = eeprom.read_eeprom()

        assert first == b"RAW"
        assert second == b"RAW"
        init.assert_called_once()

    def test_get_system_eeprom_info_caches(self):
        eeprom = self._make_eeprom()
        with patch.object(eeprom, "_eeprom_init") as init:
            def fake_init():
                eeprom._eeprom_info_dict = {"a": 1}
                eeprom._eeprom_raw = b"X"
            init.side_effect = fake_init

            info = eeprom.get_system_eeprom_info()
            info2 = eeprom.get_system_eeprom_info()

        assert info == {"a": 1}
        assert info is info2
        init.assert_called_once()


class TestEepromBMCAndSystem:

    def test_eeprom_bmc_uses_bmc_path_and_redis(self):
        with patch("sonic_platform.eeprom.IpmiFru") as fru_cls:
            bmc = EepromBMC()
        fru_cls.assert_called_once_with(EEPROM_BMC_PATH)
        assert bmc.available_in_redis is True

    def test_eeprom_system_uses_system_path_no_redis(self):
        with patch("sonic_platform.eeprom.IpmiFru") as fru_cls:
            system = EepromSystem()
        fru_cls.assert_called_once_with(EEPROM_SYSTEM_PATH)
        assert system.available_in_redis is False

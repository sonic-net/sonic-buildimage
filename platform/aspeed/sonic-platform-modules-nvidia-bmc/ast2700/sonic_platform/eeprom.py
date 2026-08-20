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

from functools import reduce, cache
import subprocess

try:
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_py_common.logger import Logger
    from sonic_platform.utils import hw_mgmt_path
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()

EEPROM_BMC_PATH = hw_mgmt_path("eeprom", "eeprom_bmc")
EEPROM_SYSTEM_PATH = hw_mgmt_path("eeprom", "eeprom_system")


class IpmiFru:
    PARSING_RULES = {
        "FRU Board Product Name:": TlvInfoDecoder._TLV_CODE_PRODUCT_NAME,
        "FRU Board Part Number:": TlvInfoDecoder._TLV_CODE_PART_NUMBER,
        "FRU Board Serial Number:": TlvInfoDecoder._TLV_CODE_SERIAL_NUMBER,
        "FRU Board Custom Info: MAC:": TlvInfoDecoder._TLV_CODE_MAC_BASE,
        "FRU Product Version:": TlvInfoDecoder._TLV_CODE_LABEL_REVISION,
    }

    IPMI_FRU_BIN = "ipmi-fru"
    IPMI_FRU_TIMEOUT = 20

    def __init__(self, fru_file):
        self.fru_file = fru_file

    def get_fru_info(self):
        cmd = [self.IPMI_FRU_BIN, "--fru-file", self.fru_file]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=False, timeout=self.IPMI_FRU_TIMEOUT)
        except subprocess.TimeoutExpired as exc:
            logger.log_error(f"timeout executing {' '.join(cmd)}: {exc}")
            return ""
        except OSError as exc:
            logger.log_error(f"failed to execute {' '.join(cmd)}: {exc}")
            return ""

        if result.returncode != 0:
            stderr = (getattr(result, "stderr", "") or "").strip()
            logger.log_error(f"{' '.join(cmd)} exited with {result.returncode}: {stderr}")
            return ""

        return result.stdout or ""

    def get_tlv_list(self):
        return list(self.PARSING_RULES.values())

    @cache
    def get_tlv_dict(self):
        tlv_dict = {}
        fru_info = self.get_fru_info()
        for line in fru_info.splitlines():
            for key, value in self.PARSING_RULES.items():
                if key in line:
                    tlv_dict[value] = line.split(key)[1].strip()
        return tlv_dict


class Eeprom(TlvInfoDecoder):

    def __init__(self, fru_file, available_in_redis=False):
        self.fru = IpmiFru(fru_file)
        self._eeprom_info_dict = None
        self._eeprom_raw = None
        self.available_in_redis = available_in_redis
        super().__init__('', 0, '', True)

    def _read_from_redis(self):
        db_initialized = self._redis_hget('EEPROM_INFO|State', 'Initialized')
        if db_initialized != '1':
            return None
        data = {}
        for code in self.fru.get_tlv_list():
            value = self._redis_hget(f'EEPROM_INFO|{hex(code)}', 'Value')
            if value:
                data[code] = value
        return data

    def _eeprom_dict_get(self):
        if self.available_in_redis:
            from_redis = self._read_from_redis()
            if from_redis:
                return from_redis
        return self.fru.get_tlv_dict()

    def _eeprom_init_raw(self, tlvs_info):
        """
        Initialize `_eeprom_raw` by encoding `tlvs_info` values via `TlvInfoDecoder.encoder`.
        """
        encoded = [self.encoder((k,), tlvs_info[k]) for k in sorted(tlvs_info.keys())]
        tlvs = reduce(lambda x, y: x + y, encoded, bytearray())

        # Append the TLV_CODE_CRC_32 [type, len] header used for the checksum entry.
        tlvs += bytearray([self._TLV_CODE_CRC_32]) + bytearray([4])

        # Reserve 4 extra bytes for the checksum value computed and appended below.
        tlvs_len = len(tlvs) + 4

        tlvs_len_header = bytearray([(tlvs_len >> 8) & 0xFF]) + bytearray([tlvs_len & 0xFF])
        header = self._TLV_INFO_ID_STRING + bytearray([self._TLV_INFO_VERSION]) + tlvs_len_header
        raw = header + tlvs
        checksum = self.calculate_checksum(raw)
        raw += self.encode_checksum(checksum)
        self._eeprom_raw = raw
        return checksum

    def _eeprom_init(self):
        tlv_dict = self._eeprom_dict_get()
        checksum = self._eeprom_init_raw(tlv_dict)

        self._eeprom_info_dict = {hex(k): v for k, v in tlv_dict.items()}
        self._eeprom_info_dict[hex(self._TLV_CODE_CRC_32)] = f"0x{checksum:08X}"

    def _get_eeprom_value(self, code):
        info = self.get_system_eeprom_info()
        return info.get(hex(code), None)

    def read_eeprom(self):
        if self._eeprom_raw is None:
            self._eeprom_init()
        return self._eeprom_raw

    def get_system_eeprom_info(self):
        if self._eeprom_info_dict is None:
            self._eeprom_init()
        return self._eeprom_info_dict

    def get_base_mac(self):
        return self._get_eeprom_value(self._TLV_CODE_MAC_BASE)

    def get_serial_number(self):
        return self._get_eeprom_value(self._TLV_CODE_SERIAL_NUMBER)

    def get_product_name(self):
        return self._get_eeprom_value(self._TLV_CODE_PRODUCT_NAME)

    def get_part_number(self):
        return self._get_eeprom_value(self._TLV_CODE_PART_NUMBER)

    def get_revision(self):
        return self._get_eeprom_value(self._TLV_CODE_LABEL_REVISION)


class EepromBMC(Eeprom):
    def __init__(self):
        super().__init__(EEPROM_BMC_PATH, True)


class EepromSystem(Eeprom):
    def __init__(self):
        super().__init__(EEPROM_SYSTEM_PATH, False)

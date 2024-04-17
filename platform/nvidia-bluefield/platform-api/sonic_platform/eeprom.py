#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
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

from functools import reduce
import re
import enum
import subprocess

from sonic_py_common.logger import Logger
try:
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

from .utils import default_return

logger = Logger()


class Vpd:

    class VpdAttrs(enum.Enum):
        product_name_id = 0
        part_number_id = 1
        revision = 2
        serian_number = 3
        connectx_fw_ver = 4
        board_id = 5
        node_guid = 6
        sys_image_guid = 7

    vpd_attrs_map = {
        ('IDTAG', 'Board Id'): VpdAttrs.product_name_id,
        ('PN', 'Part Number'): VpdAttrs.part_number_id,
        ('EC', 'Revision'): VpdAttrs.revision,
        ('SN', 'Serial Number'): VpdAttrs.serian_number,
    }

    def __init__(self, fwmngr):
        self.fwmngr = fwmngr
        self.vpd_attrs = {}

    def _read_raw(self):
        p = subprocess.Popen(
            ["mlxvpd", "-d", self.fwmngr.get_pci_dev_name()],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        raw_data, err = p.communicate()

        if err:
            raise RuntimeError(err.decode())

        return raw_data.decode().split('\n')

    def _read(self):
        vpd_attrs = {}

        for line in self._read_raw():
            line = line.strip()
            if not line:
                continue

            for (vpd, descr), idx in self.vpd_attrs_map.items():
                match = re.search(f'{vpd}\s+{descr}', line)
                if not match:
                    continue
                vpd_attrs[idx] = line.replace(match.group(), '').strip()

        return vpd_attrs

    def _get_vpd_attrs(self):
        if not self.vpd_attrs:
            self.vpd_attrs = self._read()

        return self.vpd_attrs

    def get_serial_number(self):
        self._get_vpd_attrs()
        return self.vpd_attrs[self.VpdAttrs.serian_number]

    def get_product_name(self):
        self._get_vpd_attrs()
        return self.vpd_attrs[self.VpdAttrs.product_name_id]

    def get_part_number(self):
        self._get_vpd_attrs()
        return self.vpd_attrs[self.VpdAttrs.part_number_id]

    def get_revision(self):
        self._get_vpd_attrs()
        return self.vpd_attrs[self.VpdAttrs.revision]


class FwManager:

    class FwInfo(enum.Enum):
        base_mac = 0
        pci_dev_name = 1

    fw_info_fields_map = {
        'Base MAC': FwInfo.base_mac,
        'PCI Device Name': FwInfo.pci_dev_name
    }

    def __init__(self):
        self._fw_info = {}

    def _read_fw_info_raw(self):
        p = subprocess.Popen(
            "mlxfwmanager", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        raw_data, err = p.communicate()

        if err:
            raise RuntimeError(err.decode())

        return raw_data.decode().split('\n')

    def _read_fw_info(self):
        fw_info = {}

        for line in self._read_fw_info_raw():
            line = line.strip()
            if not line:
                continue

            line = line.split(': ')
            if len(line) != 2 or line[0] not in self.fw_info_fields_map:
                continue

            fw_info[self.fw_info_fields_map[line[0]]] = line[1].strip()

        return fw_info

    def _get_fw_info(self):
        if not self._fw_info:
            self._fw_info = self._read_fw_info()

        return self._fw_info

    def get_base_mac(self):
        self._get_fw_info()
        mac = self._fw_info[self.FwInfo.base_mac]
        return ":".join([mac[i] + mac[i + 1] for i in range(0, len(mac), 2)])

    def get_pci_dev_name(self):
        self._get_fw_info()
        return self._fw_info[self.FwInfo.pci_dev_name]


class Eeprom(TlvInfoDecoder):

    def __init__(self):
        self._fwmngr = FwManager()
        self._vpd = Vpd(self._fwmngr)
        self._eeprom_info_dict = None
        self._eeprom_raw = None
        super(Eeprom, self).__init__('', 0, '', True)

    @default_return(return_value='Undefined.')
    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._get_eeprom_value(self._TLV_CODE_MAC_BASE)

    @default_return(return_value='Undefined.')
    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._get_eeprom_value(self._TLV_CODE_SERIAL_NUMBER)

    @default_return(return_value='Undefined.')
    def get_product_name(self):
        """
        Retrieves the hardware product name for the chassis

        Returns:
            A string containing the hardware product name for this chassis.
        """
        return self._get_eeprom_value(self._TLV_CODE_PRODUCT_NAME)

    @default_return(return_value='Undefined.')
    def get_part_number(self):
        """
        Retrieves the hardware part number for the chassis

        Returns:
            A string containing the hardware part number for this chassis.
        """
        return self._get_eeprom_value(self._TLV_CODE_PART_NUMBER)

    @default_return(return_value='Undefined.')
    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._get_eeprom_value(self._TLV_CODE_LABEL_REVISION)

    @default_return({})
    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        if not self._eeprom_info_dict:
            self._eeprom_init()
        return self._eeprom_info_dict

    def _get_eeprom_value(self, code):
        """Helper function to help get EEPROM data by code

        Args:
            code (int): EEPROM TLV code

        Returns:
            str: value of EEPROM TLV
        """
        eeprom_info_dict = self.get_system_eeprom_info()
        return eeprom_info_dict[hex(code)]

    def _eeprom_init_raw(self, tlvs_info):
        """Initializes the _eeprom_raw by encoding tlvs_info's values via TlvInfoDecoder.encoder

        Args:
            tlvs_info (dict): EEPROM TLV types and values

        Returns:
            int: EEPROM CRC
        """
        encoded = [self.encoder((k,), tlvs_info[k]) for k in sorted(tlvs_info.keys())]
        tlvs = reduce(lambda x,y: x+y, encoded)

        # adding TLV_CODE_CRC_32 [type, len] for checksum calculation
        tlvs += bytearray([self._TLV_CODE_CRC_32]) + bytearray([4])

        # 4 extra bytes for checksum value that is calculated and added later
        tlvs_len = len(tlvs) + 4

        tlvs_len_header = bytearray([(tlvs_len >> 8) & 0xFF]) + bytearray([tlvs_len & 0xFF])
        header = self._TLV_INFO_ID_STRING + bytearray([self._TLV_INFO_VERSION]) + tlvs_len_header
        raw = header + tlvs
        checksum = self.calculate_checksum(raw)
        raw += self.encode_checksum(checksum)
        self._eeprom_raw = raw
        return checksum

    def _epprom_data_get(self):
        data = {}
        db_initialized = self._redis_hget('EEPROM_INFO|State', 'Initialized')
        if db_initialized == '1':
            tlvs = [self._TLV_CODE_PRODUCT_NAME,
                    self._TLV_CODE_PART_NUMBER,
                    self._TLV_CODE_SERIAL_NUMBER,
                    self._TLV_CODE_MAC_BASE,
                    self._TLV_CODE_LABEL_REVISION]
            for code in tlvs:
                value = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Value')
                if value:
                    data[code] = value
        else:
            data = {
                self._TLV_CODE_PRODUCT_NAME: self._vpd.get_product_name(),
                self._TLV_CODE_PART_NUMBER: self._vpd.get_part_number(),
                self._TLV_CODE_SERIAL_NUMBER: self._vpd.get_serial_number(),
                self._TLV_CODE_MAC_BASE: self._fwmngr.get_base_mac(),
                self._TLV_CODE_LABEL_REVISION: self._vpd.get_revision(),
            }
        return data

    def _eeprom_init(self):
        """Initializes the _eeprom_raw and _eeprom_info_dict via data retrieved from Vpd and FwManager

        """
        eeprom_data = self._epprom_data_get()
        checksum = self._eeprom_init_raw(eeprom_data)

        self._eeprom_info_dict = dict([(hex(k), v) for k,v in eeprom_data.items()])
        self._eeprom_info_dict[hex(self._TLV_CODE_CRC_32)] = checksum

    def read_eeprom(self):
        """Overrides the TlvInfoDecoder.read_eeprom so the EPPROM is read from _eeprom_raw instead of a file

        """
        if not self._eeprom_raw:
            self._eeprom_init()
        return self._eeprom_raw

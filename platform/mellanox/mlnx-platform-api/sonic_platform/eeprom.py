#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2019-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the eeprom information which are available in the platform
#
#############################################################################
import os
import subprocess

from sonic_py_common.logger import Logger
try:
    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

from .device_data import DeviceDataManager
from .utils import default_return, is_host, wait_until

logger = Logger()

#
# this is mlnx-specific
# should this be moved to chassis.py or here, which better?
#
EEPROM_SYMLINK = "/var/run/hw-management/eeprom/vpd_info"
platform_name = DeviceDataManager.get_platform_name()
platform_supporting_simx = ['x86_64-nvidia_sn4280_simx-r0', 'x86_64-mlnx_msn4700_simx-r0', 'x86_64-nvidia_sn5640_simx-r0', 'x86_64-nvidia_sn6600_simx-r0']
if platform_name and 'simx' in platform_name and not platform_name in platform_supporting_simx:
    if not os.path.exists(EEPROM_SYMLINK):
        if is_host():
            platform_path = os.path.join('/usr/share/sonic/device', platform_name)
        else:
            platform_path = '/usr/share/sonic/platform'
        if not os.path.exists(os.path.dirname(EEPROM_SYMLINK)):
            os.makedirs(os.path.dirname(EEPROM_SYMLINK))
        subprocess.check_call(['/usr/bin/xxd', '-r', '-p', 'syseeprom.hex', EEPROM_SYMLINK], cwd=platform_path)

WAIT_EEPROM_READY_SEC = 60


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        if not wait_until(predict=os.path.exists, timeout=WAIT_EEPROM_READY_SEC, path=EEPROM_SYMLINK):
            logger.log_error("Nowhere to read syseeprom from! No symlink found")
            raise RuntimeError("No syseeprom symlink found")

        self.eeprom_path = EEPROM_SYMLINK
        super(Eeprom, self).__init__(self.eeprom_path, 0, '', True)
        self._eeprom_info_dict = None

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

    @default_return({})
    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        if self._eeprom_info_dict is None:
            self._eeprom_info_dict = {}
            # Try get from DB first
            db_initialized = self._redis_hget('EEPROM_INFO|State', 'Initialized')
            if db_initialized == '1':
                code = self._TLV_CODE_PRODUCT_NAME
                while code <= self._TLV_CODE_SERVICE_TAG:
                    value = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Value')
                    if value:
                        self._eeprom_info_dict[hex(code)] = value
                    code += 1

                # Handle vendor extension TLV
                vendor_extension_tlv_code = hex(self._TLV_CODE_VENDOR_EXT)
                try:
                    vendor_extension_num = int(self._redis_hget('EEPROM_INFO|{}'.format(vendor_extension_tlv_code), 'Num_vendor_ext'))
                except (ValueError, TypeError):
                    vendor_extension_num = 0

                if vendor_extension_num != 0:
                    for i in range(vendor_extension_num):
                        value = self._redis_hget('EEPROM_INFO|{}'.format(vendor_extension_tlv_code), 'Value_{}'.format(i))
                        if value:
                            if vendor_extension_tlv_code not in self._eeprom_info_dict:
                                self._eeprom_info_dict[vendor_extension_tlv_code] = [value]
                            else:
                                self._eeprom_info_dict[vendor_extension_tlv_code].append(value)
            
                # Get CRC 
                value = self._redis_hget('EEPROM_INFO|{}'.format(hex(self._TLV_CODE_CRC_32)), 'Value')
                if value:
                    self._eeprom_info_dict[hex(self._TLV_CODE_CRC_32)] = value
            else:
                eeprom = self.read_eeprom()
                visitor = EepromContentVisitor(self._eeprom_info_dict)
                self.visit_eeprom(eeprom, visitor)
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


class EepromContentVisitor(eeprom_tlvinfo.EepromDefaultVisitor):
    def __init__(self, content):
        self.content = content

    def visit_tlv(self, name, code, length, value):
        if code != Eeprom._TLV_CODE_VENDOR_EXT:
            self.content[hex(code)] = value.rstrip('\0')
        else:
            if value:
                value = value.rstrip('\0')
                if value:
                    code = hex(code)
                    if code not in self.content:
                        self.content[code] = [value]
                    else:
                        self.content[code].append(value)


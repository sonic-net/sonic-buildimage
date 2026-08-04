#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import os
import pytest
import sys
from sonic_py_common import device_info
if sys.version_info.major == 3:
    from unittest.mock import MagicMock, patch
else:
    from mock import MagicMock, patch

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.chassis import Chassis
from sonic_platform.eeprom import Eeprom, EepromContentVisitor

class TestEeprom:
    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('os.path.islink', MagicMock(return_value=True))
    @patch('sonic_py_common.device_info.get_hwsku', MagicMock(return_value='MSN3420'))
    @patch('sonic_platform.eeprom.Eeprom.get_system_eeprom_info')
    @patch('sonic_platform.chassis.extract_RJ45_ports_index', MagicMock(return_value=[]))
    def test_chassis_eeprom(self, mock_eeprom_info):
        mock_eeprom_info.return_value = {
            hex(Eeprom._TLV_CODE_PRODUCT_NAME): 'MSN3420',
            hex(Eeprom._TLV_CODE_PART_NUMBER): 'MSN3420-CB2FO',
            hex(Eeprom._TLV_CODE_MAC_BASE): '1C:34:DA:1C:9F:00',
            hex(Eeprom._TLV_CODE_SERIAL_NUMBER): 'MT2019X13878'
        }
        chassis = Chassis()
        assert chassis.get_name() == 'MSN3420'
        assert chassis.get_model() == 'MSN3420-CB2FO'
        assert chassis.get_base_mac() == '1C:34:DA:1C:9F:00'
        assert chassis.get_serial() == 'MT2019X13878'
        assert chassis.get_system_eeprom_info() == mock_eeprom_info.return_value

    @patch('sonic_platform.eeprom.wait_until', MagicMock(return_value=False))
    def test_eeprom_init(self):
        # Test symlink not exist, there is an exception
        with pytest.raises(RuntimeError):
            Eeprom()

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('os.path.islink', MagicMock(return_value=True))
    def test_get_system_eeprom_info_from_db(self):
        return_values = {
            ('EEPROM_INFO|State', 'Initialized'): '1',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_PRODUCT_NAME)), 'Value'): 'MSN3420',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_PART_NUMBER)), 'Value'): 'MSN3420-CB2FO',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_MAC_BASE)), 'Value'): '1C:34:DA:1C:9F:00',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_SERIAL_NUMBER)), 'Value'): 'MT2019X13878',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_VENDOR_EXT)), 'Num_vendor_ext'): '2',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_VENDOR_EXT)), 'Value_0'): 'ext1',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_VENDOR_EXT)), 'Value_1'): 'ext2',
            ('EEPROM_INFO|{}'.format(hex(Eeprom._TLV_CODE_CRC_32)), 'Value'): 'CRC_VALUE',
        }
        def side_effect(key, field):
            return return_values.get((key, field))
        eeprom = Eeprom()
        eeprom._redis_hget = MagicMock(side_effect = side_effect)
        
        info = eeprom.get_system_eeprom_info()
        assert eeprom.get_product_name() == 'MSN3420'
        assert eeprom.get_part_number() == 'MSN3420-CB2FO'
        assert eeprom.get_base_mac() == '1C:34:DA:1C:9F:00'
        assert eeprom.get_serial_number() == 'MT2019X13878'
        assert info[hex(Eeprom._TLV_CODE_VENDOR_EXT)] == ['ext1', 'ext2']
        assert info[hex(Eeprom._TLV_CODE_CRC_32)] == 'CRC_VALUE'

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('os.path.islink', MagicMock(return_value=True))
    def test_get_system_eeprom_info_from_hardware(self):    
        eeprom = Eeprom()
        eeprom.p = os.path.join(test_path, 'mock_eeprom_data')
        eeprom._redis_hget = MagicMock()
        info = eeprom.get_system_eeprom_info()
        assert eeprom.get_product_name() == 'MSN3800'
        assert eeprom.get_part_number() == 'MSN3800-CS2FO'
        assert eeprom.get_base_mac() == 'B8:59:9F:A9:34:00'
        assert eeprom.get_serial_number() == 'MT1937X00537'
        assert info[hex(Eeprom._TLV_CODE_CRC_32)] == '0x9EFF0119'

    def test_eeprom_content_visitor(self):
        content = {}
        v = EepromContentVisitor(content)
        v.visit_tlv('tlv1', Eeprom._TLV_CODE_PRODUCT_NAME, 7, 'MSN3420')
        v.visit_tlv('tlv2', Eeprom._TLV_CODE_VENDOR_EXT, 4, 'ext1')
        v.visit_tlv('tlv3', Eeprom._TLV_CODE_VENDOR_EXT, 4, 'ext2')
        assert content[hex(Eeprom._TLV_CODE_PRODUCT_NAME)] == 'MSN3420'
        assert content[hex(Eeprom._TLV_CODE_VENDOR_EXT)] == ['ext1', 'ext2']

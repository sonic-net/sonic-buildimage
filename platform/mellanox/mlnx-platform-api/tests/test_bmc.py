#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
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
import json
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.bmc import BMC
from sonic_platform_base.bmc_base import BMCBase
from sonic_platform_base.redfish_client import RedfishClient


class MockBMCComponent:
    def get_firmware_id(self):
        return 'MGX_FW_BMC_0'

    def get_name(self):
        return 'BMC'


class TestBMC:
    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._get_eeprom_info')
    def test_bmc_get_eeprom(self, mock_get_eeprom_info):
        """Test get_eeprom method with successful EEPROM retrieval"""
        eeprom_dict_file_path = os.path.join(test_path, 'mock_parsed_bmc_eeprom_dict')
        with open(eeprom_dict_file_path, 'r') as f:
            data = f.read()
            expected_eeprom_data = json.loads(data)
        mock_get_eeprom_info.return_value = (RedfishClient.ERR_CODE_OK, expected_eeprom_data)
        bmc = BMC.get_instance()
        result = bmc.get_eeprom()
        assert result == expected_eeprom_data
        mock_get_eeprom_info.assert_called_once_with(BMC.BMC_EEPROM_ID)

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._get_firmware_version')
    def test_bmc_get_version(self, mock_get_firmware_version):
        """Test get_version method with successful version retrieval"""
        expected_version = '88.0002.1252'
        mock_get_firmware_version.return_value = (RedfishClient.ERR_CODE_OK, expected_version)
        bmc = BMC.get_instance()
        result = bmc.get_version()
        assert result == expected_version
        mock_get_firmware_version.assert_called_once_with(BMC.BMC_FIRMWARE_ID)

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_change_login_password')
    def test_bmc_reset_password(self, mock_change_password):
        """Test reset_password method with successful password reset"""
        mock_change_password.return_value = (RedfishClient.ERR_CODE_OK, 'Password changed successfully')
        bmc = BMC.get_instance()
        ret, msg = bmc.reset_root_password()
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == 'Password changed successfully'
        mock_change_password.assert_called_once_with('testpass', BMCBase.ROOT_ACCOUNT)

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_trigger_bmc_debug_log_dump')
    def test_bmc_trigger_bmc_debug_log_dump(self, mock_trigger_debug_log_dump):
        """Test trigger_bmc_debug_log_dump method with successful debug log dump"""
        expected_msg = 'Success'
        task_id = '0'
        mock_trigger_debug_log_dump.return_value = (RedfishClient.ERR_CODE_OK, (task_id, expected_msg))
        bmc = BMC.get_instance()
        (ret, (ret_task_id, msg)) = bmc.trigger_bmc_debug_log_dump()
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == expected_msg
        assert task_id == ret_task_id

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_bmc_debug_log_dump')
    def test_bmc_get_bmc_debug_log_dump(self, mock_get_debug_log_dump):
        """Test get_bmc_debug_log_dump method with successful debug log dump"""
        expected_msg = 'Success'
        mock_get_debug_log_dump.return_value = (RedfishClient.ERR_CODE_OK, expected_msg)
        bmc = BMC.get_instance()
        (ret, msg) = bmc.get_bmc_debug_log_dump('0', '/tmp', 'file.txt')
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == expected_msg

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_update_firmware')
    def test_bmc_update_firmware(self, mock_update_fw):
        """Test update_firmware method with successful update"""
        mock_update_fw.return_value = (RedfishClient.ERR_CODE_OK, 'Update successful', [BMC.BMC_FIRMWARE_ID])
        bmc = BMC.get_instance()
        ret, (msg, updated_components) = bmc.update_firmware('fake_image.fwpkg')
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == 'Update successful'
        assert updated_components == [BMC.BMC_FIRMWARE_ID]
        mock_update_fw.assert_called_once_with('fake_image.fwpkg', fw_ids=[BMC.BMC_FIRMWARE_ID])

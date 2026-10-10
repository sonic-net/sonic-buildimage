#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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


# What the Redfish client reports for a firmware ID the BMC does not carry. The
# BMC answers 404 with a Redfish error body, and curl exits 0 on that, so the
# client reports ERR_CODE_OK and is left with the 'N/A' it started from - the
# missing version, not the return code, is what marks the ID as not this BMC's.
MISSING_INVENTORY_MEMBER = (RedfishClient.ERR_CODE_OK, 'N/A')


@mock.patch('sonic_platform.device_data.DeviceDataManager.is_platform_with_bmc',
            mock.MagicMock(return_value=True))
class TestBMC:
    # BMC is a singleton and caches the resolved firmware ID on the instance,
    # so drop it between tests to keep them independent.
    @pytest.fixture(autouse=True)
    def reset_bmc_singleton(self):
        BMC._instance = None
        yield
        BMC._instance = None

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
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    @mock.patch('sonic_platform.bmc.BMC._get_firmware_version')
    def test_bmc_get_version_inventory_unreadable(self, mock_get_firmware_version, mock_redfish_get_version):
        """Test get_version method when the firmware inventory cannot be read"""
        mock_redfish_get_version.return_value = (RedfishClient.ERR_CODE_SERVER_UNREACHABLE, 'N/A')
        mock_get_firmware_version.return_value = (RedfishClient.ERR_CODE_OK, 'must-not-be-used')
        bmc = BMC.get_instance()
        assert bmc.get_version() == 'N/A'
        # both known IDs are tried before giving up
        assert mock_redfish_get_version.call_count == 2

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_set_min_password_length')
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_change_login_password')
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_min_password_length')
    def test_bmc_reset_password(self, mock_get_min_length, mock_change_password, mock_set_min_length):
        """Test reset_password method with successful password reset"""
        mock_get_min_length.return_value = (RedfishClient.ERR_CODE_OK, 12)
        mock_set_min_length.return_value = (RedfishClient.ERR_CODE_OK, '')
        mock_change_password.return_value = (RedfishClient.ERR_CODE_OK, '')
        bmc = BMC.get_instance()
        ret, msg = bmc.reset_root_password()
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == ''
        mock_get_min_length.assert_called_once()
        assert mock_set_min_length.call_args_list == [mock.call(8), mock.call(12)]
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
    @mock.patch('sonic_platform.bmc.BMC.get_firmware_id', mock.MagicMock(return_value='FW_BMC_0'))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_update_firmware')
    def test_bmc_update_firmware(self, mock_update_fw):
        """Test update_firmware method with successful update"""
        mock_update_fw.return_value = (RedfishClient.ERR_CODE_OK, 'Update successful', ['FW_BMC_0'])
        bmc = BMC.get_instance()
        ret, (msg, updated_components) = bmc.update_firmware('fake_image.fwpkg')
        assert ret == RedfishClient.ERR_CODE_OK
        assert msg == 'Update successful'
        assert updated_components == ['FW_BMC_0']
        mock_update_fw.assert_called_once_with('fake_image.fwpkg', fw_ids=['FW_BMC_0'])

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    @mock.patch('sonic_platform.bmc.BMC._get_firmware_version')
    def test_bmc_get_version_new_firmware_naming(self, mock_get_firmware_version, mock_redfish_get_version):
        """BMC FW 88.0060.2303 dropped the 'MGX_' prefix from its Redfish firmware IDs"""
        expected_version = '88.0060.2303'

        def fake_get_version(fw_id):
            if fw_id == 'FW_BMC_0':
                return (RedfishClient.ERR_CODE_OK, expected_version)
            return MISSING_INVENTORY_MEMBER

        mock_redfish_get_version.side_effect = fake_get_version
        mock_get_firmware_version.return_value = (RedfishClient.ERR_CODE_OK, 'must-not-be-used')
        bmc = BMC.get_instance()
        assert bmc.get_version() == expected_version
        # the new (preferred) ID is queried first and answers immediately, no
        # need to also try the legacy ID or fall back to a second query
        mock_redfish_get_version.assert_called_once_with('FW_BMC_0')
        mock_get_firmware_version.assert_not_called()

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    @mock.patch('sonic_platform.bmc.BMC._get_firmware_version')
    def test_bmc_get_version_legacy_firmware_naming(self, mock_get_firmware_version, mock_redfish_get_version):
        """BMC FW 88.0060.2112 and older prefix their Redfish firmware IDs with 'MGX_'"""
        expected_version = '88.0060.2112'

        def fake_get_version(fw_id):
            if fw_id == 'MGX_FW_BMC_0':
                return (RedfishClient.ERR_CODE_OK, expected_version)
            return MISSING_INVENTORY_MEMBER

        mock_redfish_get_version.side_effect = fake_get_version
        mock_get_firmware_version.return_value = (RedfishClient.ERR_CODE_OK, 'must-not-be-used')
        bmc = BMC.get_instance()
        assert bmc.get_version() == expected_version
        # the new (preferred) ID is tried first and misses, then the legacy ID hits
        assert mock_redfish_get_version.call_args_list == [
            mock.call('FW_BMC_0'), mock.call('MGX_FW_BMC_0')]
        mock_get_firmware_version.assert_not_called()

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_get_version_unrecognized_inventory(self, mock_redfish_get_version):
        """The BMC's inventory is readable, but neither known ID is present on it.

        Distinct from an unreachable/unreadable inventory: this is the case the
        two-ID resolution exists to catch - a future rename (or an unexpected
        device) that this code does not yet know about.
        """
        mock_redfish_get_version.return_value = MISSING_INVENTORY_MEMBER
        bmc = BMC.get_instance()
        assert bmc.get_version() == 'N/A'
        assert mock_redfish_get_version.call_args_list == [
            mock.call('FW_BMC_0'), mock.call('MGX_FW_BMC_0')]

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_get_firmware_id_queries_the_bmc_once(self, mock_redfish_get_version):
        """The firmware ID does not change while the BMC runs, so resolve it once"""
        mock_redfish_get_version.return_value = (RedfishClient.ERR_CODE_OK, '88.0060.2303')
        bmc = BMC.get_instance()
        assert bmc.get_firmware_id() == 'FW_BMC_0'
        assert bmc.get_firmware_id() == 'FW_BMC_0'
        mock_redfish_get_version.assert_called_once_with('FW_BMC_0')

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_get_firmware_id_legacy_firmware_naming(self, mock_redfish_get_version):
        """A legacy BMC must resolve to the legacy ID, not just display the right version.

        get_firmware_id() feeds the fw_ids of the update request, so settling on
        the current-naming ID here would aim the update at an inventory member
        the BMC does not have.
        """
        def fake_get_version(fw_id):
            if fw_id == 'MGX_FW_BMC_0':
                return (RedfishClient.ERR_CODE_OK, '88.0060.2112')
            return MISSING_INVENTORY_MEMBER

        mock_redfish_get_version.side_effect = fake_get_version
        bmc = BMC.get_instance()
        assert bmc.get_firmware_id() == 'MGX_FW_BMC_0'

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_get_firmware_id_inventory_unreachable(self, mock_redfish_get_version):
        """An unreadable inventory falls back to the current naming and is retried next time"""
        mock_redfish_get_version.return_value = (RedfishClient.ERR_CODE_SERVER_UNREACHABLE, 'N/A')
        bmc = BMC.get_instance()
        assert bmc.get_firmware_id() == 'FW_BMC_0'
        assert bmc.get_firmware_id() == 'FW_BMC_0'
        # each unresolved call retries both known IDs: 2 (first call) + 2 (second call)
        assert mock_redfish_get_version.call_count == 4

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform.bmc.BMC._login', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.bmc_base.BMCBase._logout', mock.MagicMock(return_value=RedfishClient.ERR_CODE_OK))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_get_version_does_not_cache_the_firmware_id(self, mock_redfish_get_version):
        """Only the firmware update flow caches the ID, so get_version() stays stateless"""
        mock_redfish_get_version.return_value = (RedfishClient.ERR_CODE_OK, '88.0060.2303')
        bmc = BMC.get_instance()
        assert bmc.get_version() == '88.0060.2303'
        assert bmc.get_firmware_id() == 'FW_BMC_0'
        # get_version() (1 call) + get_firmware_id() resolving independently (1 call)
        assert mock_redfish_get_version.call_count == 2

    @mock.patch('sonic_py_common.device_info.get_bmc_build_config', \
                mock.MagicMock(return_value={'bmc_nos_account_username': 'testuser', 'bmc_root_account_default_password': 'testpass'}))
    @mock.patch('sonic_py_common.device_info.get_bmc_data', \
                mock.MagicMock(return_value={'bmc_addr': '169.254.0.1'}))
    @mock.patch('sonic_platform.bmc.BMC._get_tpm_password', mock.MagicMock(return_value=''))
    @mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_get_firmware_version')
    def test_bmc_update_firmware_keeps_the_session_while_resolving_the_id(self, mock_redfish_get_version):
        """update_firmware() resolves the ID inside its own session - that must not log it out"""
        # model the real client: a session is opened by login, dropped by logout,
        # and every other request is refused while there is none
        session = {'open': False}

        def fake_login(self, *args, **kwargs):
            session['open'] = True
            return RedfishClient.ERR_CODE_OK

        def fake_logout(self, *args, **kwargs):
            session['open'] = False
            return RedfishClient.ERR_CODE_OK

        def fake_update(self, fw_image, fw_ids=None, **kwargs):
            if not session['open']:
                return (RedfishClient.ERR_CODE_NOT_LOGIN, 'Not login', [])
            return (RedfishClient.ERR_CODE_OK, 'Update successful', list(fw_ids))

        mock_redfish_get_version.return_value = (RedfishClient.ERR_CODE_OK, '88.0060.2303')
        with mock.patch('sonic_platform_base.redfish_client.RedfishClient.login', fake_login), \
             mock.patch('sonic_platform_base.redfish_client.RedfishClient.logout', fake_logout), \
             mock.patch('sonic_platform_base.redfish_client.RedfishClient.has_login',
                        lambda self: session['open']), \
             mock.patch('sonic_platform_base.redfish_client.RedfishClient.redfish_api_update_firmware',
                        fake_update):
            bmc = BMC.get_instance()
            ret, (msg, updated_components) = bmc.update_firmware('fake_image.fwpkg')
        assert ret == RedfishClient.ERR_CODE_OK
        assert updated_components == ['FW_BMC_0']

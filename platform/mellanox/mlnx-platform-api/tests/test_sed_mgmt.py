#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from sonic_platform.sed_mgmt import SedMgmt, READ_DEFAULT_SED_PW_SCRIPT
from sonic_platform_base.sed_mgmt_base import SedMgmtBase


class TestSedMgmt:
    """Tests for Mellanox SedMgmt."""

    def test_get_min_sed_password_len(self):
        """Test get_min_sed_password_len returns 8."""
        sed = SedMgmt.get_instance()
        assert sed.get_min_sed_password_len() == 8

    def test_get_max_sed_password_len(self):
        """Test get_max_sed_password_len returns 124."""
        sed = SedMgmt.get_instance()
        assert sed.get_max_sed_password_len() == 124

    @mock.patch('sonic_platform_base.sed_mgmt_base._read_sed_config_value')
    def test_get_tpm_bank_a_address(self, mock_read):
        """Test get_tpm_bank_a_address reads from config."""
        mock_read.return_value = '0x81010001'
        sed = SedMgmt.get_instance()
        assert sed.get_tpm_bank_a_address() == '0x81010001'
        mock_read.assert_called_with('tpm_bank_a')

    @mock.patch('sonic_platform_base.sed_mgmt_base._read_sed_config_value')
    def test_get_tpm_bank_b_address(self, mock_read):
        """Test get_tpm_bank_b_address reads from config."""
        mock_read.return_value = '0x81010002'
        sed = SedMgmt.get_instance()
        assert sed.get_tpm_bank_b_address() == '0x81010002'
        mock_read.assert_called_with('tpm_bank_b')

    @mock.patch('subprocess.run')
    def test_get_default_sed_password_success(self, mock_run):
        """Test get_default_sed_password returns script output on success."""
        mock_run.return_value = mock.MagicMock(returncode=0, stdout='default_secret\n')
        sed = SedMgmt.get_instance()
        result = sed.get_default_sed_password()
        assert result == 'default_secret'
        mock_run.assert_called_once()
        call_args = mock_run.call_args
        assert call_args[0][0] == [READ_DEFAULT_SED_PW_SCRIPT]
        assert call_args[1]['capture_output'] is True
        assert call_args[1]['text'] is True
        assert call_args[1]['timeout'] == 10

    @mock.patch('subprocess.run')
    def test_get_default_sed_password_script_fails(self, mock_run):
        """Test get_default_sed_password returns None when script fails."""
        mock_run.return_value = mock.MagicMock(returncode=1, stdout='')
        sed = SedMgmt.get_instance()
        assert sed.get_default_sed_password() is None

    @mock.patch('subprocess.run')
    def test_get_default_sed_password_empty_stdout(self, mock_run):
        """Test get_default_sed_password returns None when stdout is empty."""
        mock_run.return_value = mock.MagicMock(returncode=0, stdout='')
        sed = SedMgmt.get_instance()
        assert sed.get_default_sed_password() is None

    @mock.patch('subprocess.run')
    def test_get_default_sed_password_exception(self, mock_run):
        """Test get_default_sed_password returns None on exception."""
        mock_run.side_effect = Exception('subprocess error')
        sed = SedMgmt.get_instance()
        assert sed.get_default_sed_password() is None

    @mock.patch('sonic_platform_base.sed_mgmt_base._read_sed_config_value')
    @mock.patch('subprocess.check_call')
    def test_change_sed_password_success(self, mock_check_call, mock_read):
        """Test change_sed_password calls script with correct args."""
        mock_read.side_effect = lambda k: {'tpm_bank_a': '0x81010001', 'tpm_bank_b': '0x81010002'}.get(k)
        sed = SedMgmt.get_instance()
        result = sed.change_sed_password('new_password123')
        assert result is True
        mock_check_call.assert_called_once()
        call_args = mock_check_call.call_args[0][0]
        assert call_args[0] == SedMgmtBase.SED_PW_CHANGE_SCRIPT
        assert call_args[call_args.index('-a') + 1] == '0x81010001'
        assert call_args[call_args.index('-b') + 1] == '0x81010002'
        assert call_args[call_args.index('-p') + 1] == 'new_password123'

    @mock.patch('sonic_platform_base.sed_mgmt_base._read_sed_config_value')
    def test_change_sed_password_missing_config(self, mock_read):
        """Test change_sed_password returns False when config missing."""
        mock_read.return_value = None
        sed = SedMgmt.get_instance()
        assert sed.change_sed_password('new_password123') is False

    @mock.patch('sonic_platform_base.sed_mgmt_base._read_sed_config_value')
    @mock.patch('subprocess.check_call')
    def test_reset_sed_password_success(self, mock_check_call, mock_read):
        """Test reset_sed_password calls script with default password."""
        mock_read.side_effect = lambda k: {'tpm_bank_a': '0x81010001', 'tpm_bank_b': '0x81010002'}.get(k)
        with mock.patch.object(SedMgmt, 'get_default_sed_password', return_value='default_secret'):
            sed = SedMgmt.get_instance()
            result = sed.reset_sed_password()
        assert result is True
        mock_check_call.assert_called_once()
        call_args = mock_check_call.call_args[0][0]
        assert call_args[0] == SedMgmtBase.SED_PW_RESET_SCRIPT
        assert call_args[call_args.index('-p') + 1] == 'default_secret'

    @mock.patch.object(SedMgmt, 'get_default_sed_password', return_value=None)
    def test_reset_sed_password_no_default(self, mock_get_default):
        """Test reset_sed_password returns False when default password unavailable."""
        sed = SedMgmt.get_instance()
        assert sed.reset_sed_password() is False

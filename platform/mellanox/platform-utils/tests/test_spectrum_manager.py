#!/usr/bin/env python3
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

"""
Comprehensive unit tests for SpectrumFirmwareManager
"""

from mellanox_fw_manager.firmware_base import FW_ALREADY_UPDATED_FAILURE
from mellanox_fw_manager.spectrum_manager import SpectrumFirmwareManager
import os
import sys
import unittest
from unittest.mock import patch, MagicMock
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))


class TestSpectrumFirmwareManager(unittest.TestCase):
    """Test cases for SpectrumFirmwareManager methods"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def _create_manager(self, **kwargs):
        """Helper to create SpectrumFirmwareManager with mocked initialization"""
        defaults = {
            'asic_index': 0,
            'pci_id': "01:00.0",
            'fw_bin_path': self.temp_dir,
            'verbose': False,
            'clear_semaphore': False,
            'asic_type': 'SPC3',
            'status_queue': None
        }
        defaults.update(kwargs)

        with patch.object(SpectrumFirmwareManager, '_initialize_asic'):
            manager = SpectrumFirmwareManager(**defaults)
            manager.fw_file = f"{self.temp_dir}/fw-SPC3.mfa"
            manager.current_version = "30.2016.1036"
            manager.available_version = "30.2016.1040"
            return manager

    def test_get_mst_device_type(self):
        """Test _get_mst_device_type returns correct device type"""
        manager = self._create_manager()
        result = manager._get_mst_device_type()
        self.assertEqual(result, "Spectrum")

    def test_get_firmware_filename(self):
        """Test get_firmware_filename returns correct Spectrum firmware filename"""
        # Test SPC3
        manager = self._create_manager(asic_type='SPC3')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC3.mfa')

        # Test SPC
        manager = self._create_manager(asic_type='SPC')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC.mfa')

        # Test SPC5
        manager = self._create_manager(asic_type='SPC5')
        self.assertEqual(manager.get_firmware_filename(), 'fw-SPC5.mfa')

    def test_unsupported_asic_type(self):
        """Test that unsupported ASIC type raises error during initialization"""
        from mellanox_fw_manager.firmware_base import FirmwareManagerError
        with self.assertRaises(FirmwareManagerError) as context:
            SpectrumFirmwareManager(
                asic_index=0, pci_id="01:00.0", fw_bin_path=self.temp_dir,
                verbose=False, clear_semaphore=False, asic_type='unknown'
            )
        self.assertIn("Unsupported ASIC type", str(context.exception))

    def test_get_asic_type_map(self):
        """Test get_asic_type_map returns correct mapping"""
        result = SpectrumFirmwareManager.get_asic_type_map()

        expected = {
            '15b3:cb84': 'SPC',
            '15b3:cf6c': 'SPC2',
            '15b3:cf70': 'SPC3',
            '15b3:cf80': 'SPC4',
            '15b3:cf82': 'SPC5',
        }

        self.assertEqual(result, expected)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_success(self, mock_run_cmd):
        """Test _get_available_firmware_version when successful"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = f"""
Device Info:
{psid} field1 field2 30.2016.1040 field4
MT_0000001188 field1 field2 30.2016.1050 field4
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertEqual(result, "30.2016.1040")
        mock_run_cmd.assert_called_once()
        call_args = mock_run_cmd.call_args[0][0]
        self.assertIn('mlxfwmanager', call_args)
        self.assertIn('--list-content', call_args)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_command_failure(self, mock_run_cmd):
        """Test _get_available_firmware_version when mlxfwmanager command fails"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_run_cmd.return_value = MagicMock(
            returncode=1,
            stdout="",
            stderr="Error: Device not found"
        )

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_psid_not_found(self, mock_run_cmd):
        """Test _get_available_firmware_version when PSID not found in output"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = """
Device Info:
MT_0000001188 field1 field2 30.2016.1050 field4
MT_0000001189 field1 field2 30.2016.1060 field4
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_insufficient_fields(self, mock_run_cmd):
        """Test _get_available_firmware_version when line has insufficient fields"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_output = f"""
Device Info:
{psid} field1 field2
"""
        mock_run_cmd.return_value = MagicMock(returncode=0, stdout=mock_output)

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_get_available_firmware_version_exception(self, mock_run_cmd):
        """Test _get_available_firmware_version when exception occurs"""
        manager = self._create_manager()
        psid = "MT_0000001187"

        mock_run_cmd.side_effect = Exception("Subprocess execution failed")

        result = manager._get_available_firmware_version(psid)

        self.assertIsNone(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_success(self, mock_run_cmd):
        """Test run_firmware_update when successful"""
        manager = self._create_manager()

        mock_run_cmd.return_value = MagicMock(
            returncode=0,
            stdout="Firmware update completed successfully",
            stderr=""
        )

        result = manager.run_firmware_update()

        self.assertTrue(result)
        mock_run_cmd.assert_called_once()
        call_args = mock_run_cmd.call_args[0][0]
        self.assertIn('mlxfwmanager', call_args)
        self.assertIn('-u', call_args)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_failure(self, mock_run_cmd):
        """Test run_firmware_update when firmware update fails"""
        manager = self._create_manager()

        mock_run_cmd.return_value = MagicMock(
            returncode=1,
            stdout="",
            stderr="Error: Failed to update firmware - device busy"
        )

        result = manager.run_firmware_update()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_required(self, mock_run_cmd):
        """Test run_firmware_update when reactivation is required"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=0, stdout="Reactivation successful", stderr=""),
            MagicMock(returncode=0, stdout="Firmware update completed", stderr="")
        ]

        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

        reactivate_call = mock_run_cmd.call_args_list[1][0][0]
        self.assertIn('flint', reactivate_call)
        self.assertIn('ir', reactivate_call)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_fails_but_continues(self, mock_run_cmd):
        """Test run_firmware_update when reactivation fails but continues with retry"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=1, stdout="", stderr="Reactivation failed"),
            MagicMock(returncode=0, stdout="Firmware update completed", stderr="")
        ]

        result = manager.run_firmware_update()

        self.assertTrue(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_reactivation_required_retry_fails(self, mock_run_cmd):
        """Test run_firmware_update when reactivation required but retry fails"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = [
            MagicMock(returncode=FW_ALREADY_UPDATED_FAILURE, stdout="", stderr=""),
            MagicMock(returncode=0, stdout="Reactivation successful", stderr=""),
            MagicMock(returncode=1, stdout="", stderr="Retry failed")
        ]

        result = manager.run_firmware_update()

        self.assertFalse(result)
        self.assertEqual(mock_run_cmd.call_count, 3)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_exception(self, mock_run_cmd):
        """Test run_firmware_update when exception occurs"""
        manager = self._create_manager()

        mock_run_cmd.side_effect = Exception("Subprocess execution failed")

        result = manager.run_firmware_update()

        self.assertFalse(result)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_with_verbose_mode(self, mock_run_cmd):
        """Test run_firmware_update passes correct environment in verbose mode"""
        manager = self._create_manager(verbose=True)

        mock_run_cmd.return_value = MagicMock(
            returncode=0,
            stdout="Firmware update completed",
            stderr=""
        )

        result = manager.run_firmware_update()

        self.assertTrue(result)

        call_kwargs = mock_run_cmd.call_args[1]
        self.assertIn('env', call_kwargs)
        env = call_kwargs['env']
        self.assertIn('FLASH_ACCESS_DEBUG', env)
        self.assertIn('FW_COMPS_DEBUG', env)

    @patch('mellanox_fw_manager.spectrum_manager.SpectrumFirmwareManager._run_command')
    def test_run_firmware_update_failure_logs_parsed_detail(self, mock_run_cmd):
        """Failure log should surface parsed stdout detail when stderr is empty"""
        manager = self._create_manager()

        mock_run_cmd.return_value = MagicMock(
            returncode=2,
            stdout=(
                "-W- BME is not set, DMA access is not supported.\n"
                "MCC ERROR: 0x5\n"
                "Fail : Rejected authentication\n"
            ),
            stderr="",
        )

        with patch.object(manager, 'logger') as mock_logger:
            result = manager.run_firmware_update()

        self.assertFalse(result)
        mock_logger.error.assert_called_once()
        logged = mock_logger.error.call_args[0][0]
        self.assertIn("return code 2", logged)
        self.assertIn("BME is not set", logged)
        self.assertIn("MCC ERROR: 0x5", logged)
        self.assertIn("Rejected authentication", logged)


class TestMlxfwmanagerDetail(unittest.TestCase):
    """Test cases for SpectrumFirmwareManager._mlxfwmanager_detail parsing helper"""

    @staticmethod
    def _result(stdout="", stderr=""):
        return MagicMock(stdout=stdout, stderr=stderr)

    def _detail(self, stdout="", stderr="", **kwargs):
        return SpectrumFirmwareManager._mlxfwmanager_detail(
            self._result(stdout=stdout, stderr=stderr), **kwargs)

    def test_prefers_stderr_when_present(self):
        """Non-empty stderr is returned verbatim and stdout is ignored"""
        detail = self._detail(stdout="-E- error on stdout", stderr="real stderr reason")
        self.assertEqual(detail, "real stderr reason")

    def test_stderr_is_stripped(self):
        """Surrounding whitespace/newlines are stripped from stderr"""
        detail = self._detail(stderr="  device busy\n")
        self.assertEqual(detail, "device busy")

    def test_strips_ansi_and_cursor_control_from_stderr(self):
        """ANSI escapes, backspaces and carriage returns are removed from stderr"""
        detail = self._detail(stderr="\x1b[31m\rdevice busy\x08\x1b[0m")
        self.assertEqual(detail, "device busy")

    def test_empty_stderr_and_stdout_returns_empty(self):
        """Both streams empty (or None) yields an empty string"""
        self.assertEqual(self._detail(stdout="", stderr=""), "")
        self.assertEqual(self._detail(stdout=None, stderr=None), "")

    def test_extracts_error_lines_from_stdout(self):
        """Relevant error/warning lines are picked from stdout and joined with '|'"""
        stdout = (
            "Querying device ...\n"
            "-W- BME is not set, DMA access is not supported.\n"
            "MCC ERROR: 0x5\n"
            "Fail : Rejected authentication\n"
            "Done\n"
        )
        detail = self._detail(stdout=stdout)
        self.assertEqual(
            detail,
            "-W- BME is not set, DMA access is not supported. | "
            "MCC ERROR: 0x5 | Fail : Rejected authentication",
        )

    def test_matches_generic_error_and_fail_keywords(self):
        """Generic ERROR / FAIL* keywords are matched case-insensitively"""
        stdout = "informational line\nSomething Failed badly\nan Error occurred\nok\n"
        detail = self._detail(stdout=stdout)
        self.assertEqual(detail, "Something Failed badly | an Error occurred")

    def test_suppresses_noise_lines(self):
        """Debug/source-banner/flash-access noise is suppressed even if it matches errors"""
        stdout = (
            "-D- debug chatter\n"
            "user/mlxfwmanager.cpp:123\n"
            "[FLASH_ACCESS_DEBUG] error reading flash\n"
            "controlFsm : ERROR state\n"
            "-E- real error here\n"
        )
        detail = self._detail(stdout=stdout)
        self.assertEqual(detail, "-E- real error here")

    def test_deduplicates_repeated_lines(self):
        """Identical error lines appear only once, preserving first-seen order"""
        stdout = "-E- same error\n-E- same error\n-E- other error\n-E- same error\n"
        detail = self._detail(stdout=stdout)
        self.assertEqual(detail, "-E- same error | -E- other error")

    def test_strips_ansi_from_stdout_before_matching(self):
        """ANSI/cursor-control bytes in stdout are removed before line matching"""
        stdout = "\x1b[2J\x1b[H-E- corrupt MFA file\x1b[0m\n\rprogress\x08\x08\n"
        detail = self._detail(stdout=stdout)
        self.assertEqual(detail, "-E- corrupt MFA file")

    def test_falls_back_to_full_stdout_when_no_error_line(self):
        """With no recognizable error line, the full cleaned stdout is returned"""
        stdout = "just some neutral progress output\nmore output\n"
        detail = self._detail(stdout=stdout)
        self.assertEqual(detail, "just some neutral progress output\nmore output")

    def test_picked_detail_truncated_to_max_len(self):
        """Joined error detail longer than max_len is truncated with a marker"""
        stdout = "\n".join(f"-E- error number {i} with some padding text" for i in range(200))
        detail = self._detail(stdout=stdout, max_len=128)
        self.assertTrue(detail.endswith(" ...[truncated]"))
        self.assertLessEqual(len(detail), 128 + len(" ...[truncated]"))
        self.assertTrue(detail.startswith("-E- error number 0"))

    def test_stdout_fallback_truncated_keeps_tail(self):
        """Oversized fallback stdout is truncated keeping the tail with a leading marker"""
        stdout = "x" * 5000  # no error markers -> fallback path
        detail = self._detail(stdout=stdout, max_len=100)
        self.assertTrue(detail.startswith("...[truncated]"))
        self.assertEqual(len(detail), len("...[truncated]") + 100)
        self.assertTrue(detail.endswith("x" * 100))


if __name__ == '__main__':
    unittest.main()

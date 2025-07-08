#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

"""Tests for dpuctlplat Platform API Wrapper"""
import os
import sys
import time
import pytest
import subprocess
from unittest.mock import MagicMock, patch, Mock, call

from sonic_platform.dpuctlplat import (
    DpuCtlPlat, BootProgEnum, PCI_DEV_BASE, OperationType,
    WAIT_FOR_SHTDN, WAIT_FOR_DPU_READY
)

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)
scripts_path = os.path.join(modules_path, "scripts")

# Test data
TEST_DPU_LIST = ['dpu0', 'dpu1', 'dpu2', 'dpu3']
TEST_PCI_PATH = os.path.join(PCI_DEV_BASE, "0000:08:00.0")
TEST_RSHIM_PCI_PATH = os.path.join(PCI_DEV_BASE, "0000:08:00.1")
TEST_PCI_REMOVE_PATH = os.path.join(TEST_PCI_PATH, "remove")
TEST_RSHIM_PCI_REMOVE_PATH = os.path.join(TEST_RSHIM_PCI_PATH, "remove")

@pytest.fixture
def dpuctl_obj():
    """Fixture to create a DpuCtlPlat object for testing"""
    obj = DpuCtlPlat('dpu0')
    obj.setup_logger(True)
    obj.pci_dev_path = [TEST_PCI_PATH, TEST_RSHIM_PCI_PATH]
    return obj

class TestDpuCtlPlatInit:
    """Tests for DpuCtlPlat initialization"""

    def test_init(self, dpuctl_obj):
        """Test initialization of DpuCtlPlat object"""
        assert dpuctl_obj.dpu_name == 'dpu0'
        assert dpuctl_obj.dpu_id == 0
        assert dpuctl_obj._name == 'dpu1'  # hwmgmt name is dpu index + 1
        assert dpuctl_obj.verbosity is False
        assert isinstance(dpuctl_obj.boot_prog_map, dict)
        assert len(dpuctl_obj.boot_prog_map) > 0
        assert len(dpuctl_obj.pci_dev_path) == 2  # Both PCI and RSHIM paths

    def test_setup_logger(self, dpuctl_obj):
        """Test logger setup"""
        # Test with print mode
        dpuctl_obj.setup_logger(True)
        # Test that the logger functions add timestamps
        with patch('time.strftime') as mock_time:
            mock_time.return_value = "2024-01-01 12:00:00"
            with patch('builtins.print') as mock_print:
                dpuctl_obj.logger_info("test message")
                mock_print.assert_called_once_with("[2024-01-01 12:00:00] test message")

        # Test with syslogger mode
        dpuctl_obj.setup_logger(False)
        assert dpuctl_obj.logger_info != print
        assert dpuctl_obj.logger_error != print
        assert dpuctl_obj.logger_debug != print

    def test_get_pci_dev_path(self, dpuctl_obj):
        """Test PCI device path retrieval"""
        # Test with both PCI and RSHIM paths
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = ["0000:08:00.0", "0000:08:00.1"]
            paths = dpuctl_obj.get_pci_dev_path()
            assert len(paths) == 2
            assert paths[0].endswith("0000:08:00.0")
            assert paths[1].endswith("0000:08:00.1")

        # Test with missing PCI path
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = [None, "0000:08:00.1"]
            dpuctl_obj.pci_dev_path = []
            with pytest.raises(RuntimeError) as exc:
                dpuctl_obj.get_pci_dev_path()
            assert "Unable to obtain PCI device IDs" in str(exc.value)

        # Test with missing RSHIM path
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = ["0000:08:00.0", None]
            with pytest.raises(RuntimeError) as exc:
                dpuctl_obj.get_pci_dev_path()
            assert "Unable to obtain PCI device IDs" in str(exc.value)

class TestDpuCtlPlatPCI:
    """Tests for PCI-related functionality"""

    def test_pci_operations(self, dpuctl_obj):
        """Test PCI remove and scan operations"""
        written_data = []
        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name, "data": content_towrite})
            return True

        # Test PCI remove - should remove both devices
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', return_value=True):
            assert dpuctl_obj.dpu_pci_remove()
            assert len(written_data) == 2
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[0]["data"] == "1"
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[1]["data"] == "1"

        # Test PCI scan - should scan devices
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file):
            assert dpuctl_obj.dpu_pci_scan()
            assert written_data[0]["file"].endswith("rescan")
            assert written_data[0]["data"] == "1"

class TestDpuCtlPlatPower:
    """Tests for power management functionality"""

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    def test_power_off(self, mock_inotify_init, mock_wait_watch, dpuctl_obj):
        """Test power off functionality"""
        mock_inotify_init.return_value = None
        mock_wait_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name, "data": content_towrite})
            return True

        # Test force power off
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_power_off(True)
            assert len(written_data) == 4  # Both PCI and RSHIM removals + rst + pwr_force
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[0]["data"] == "1"
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[1]["data"] == "1"
            assert written_data[2]["data"] == "0"  # rst
            assert written_data[3]["data"] == "0"  # pwr_force

        # Test normal power off
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_power_off(False)
            assert len(written_data) == 4  # Both PCI and RSHIM removals + rst + pwr
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[2]["file"].endswith("_rst")
            assert written_data[3]["file"].endswith("_pwr")

        # Test power off when already off
        with patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'log_info') as mock_log:
            assert dpuctl_obj.dpu_power_off(False)
            assert "Skipping DPU power off as DPU is already powered off" in mock_log.call_args_list[-1].args[0]

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    def test_power_on(self, mock_inotify_init, mock_wait_watch, dpuctl_obj):
        """Test power on functionality"""
        mock_inotify_init.return_value = None
        mock_wait_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name, "data": content_towrite})
            return True

        # Test force power on
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1):
            assert dpuctl_obj.dpu_power_on(True)
            assert len(written_data) == 3  # pwr_force + rst + rescan
            assert written_data[0]["file"].endswith("_pwr_force")
            assert written_data[0]["data"] == "1"
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[1]["data"] == "1"
            assert written_data[2]["file"].endswith("rescan")
            assert written_data[2]["data"] == "1"

        # Test normal power on
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1):
            assert dpuctl_obj.dpu_power_on(False)
            assert len(written_data) == 3  # pwr + rst + rescan
            assert written_data[0]["file"].endswith("_pwr")
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"].endswith("rescan")

class TestDpuCtlPlatReboot:
    """Tests for reboot functionality"""

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    def test_reboot(self, mock_inotify_init, mock_wait_watch, dpuctl_obj):
        """Test reboot functionality"""
        mock_inotify_init.return_value = None
        mock_wait_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name, "data": content_towrite})
            return True

        # Test normal reboot
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(False)
            assert len(written_data) == 5  # Both PCI removals + rst + rst + rescan
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[2]["file"].endswith("_rst")
            assert written_data[3]["file"].endswith("_rst")
            assert written_data[4]["file"].endswith("rescan")

        # Test force reboot
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(True)
            assert len(written_data) == 7  # Both PCI removals + rst + pwr_force + pwr_force + rst + rescan
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[2]["file"].endswith("_rst")
            assert written_data[3]["file"].endswith("_pwr_force")
            assert written_data[4]["file"].endswith("_pwr_force")
            assert written_data[5]["file"].endswith("_rst")
            assert written_data[6]["file"].endswith("rescan")

        # Test no-wait reboot
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(no_wait=True)
            assert len(written_data) == 4  # Both PCI removals + rst + rst
            assert written_data[0]["file"] == TEST_PCI_REMOVE_PATH
            assert written_data[1]["file"] == TEST_RSHIM_PCI_REMOVE_PATH
            assert written_data[2]["file"].endswith("_rst")
            assert written_data[3]["file"].endswith("_rst")

        # Test reboot with skip_pre_post=True
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(skip_pre_post=True)
            assert len(written_data) == 2  # Only rst operations
            assert all("_rst" in data["file"] for data in written_data)
            assert not any("remove" in data["file"] for data in written_data)
            assert not any("rescan" in data["file"] for data in written_data)

class TestDpuCtlPlatUtils:
    """Tests for utility functions"""

    def test_run_cmd_output(self, dpuctl_obj):
        """Test command execution and error handling"""
        # Test successful command
        with patch('subprocess.check_output') as mock_cmd:
            mock_cmd.return_value = b"success\n"
            assert dpuctl_obj.run_cmd_output(["test"]) == "success"

        # Test failed command with exception
        with patch('subprocess.check_output') as mock_cmd:
            mock_cmd.side_effect = subprocess.CalledProcessError(1, "test")
            with pytest.raises(subprocess.CalledProcessError):
                dpuctl_obj.run_cmd_output(["test"])

        # Test failed command without exception
        with patch('subprocess.check_output') as mock_cmd:
            mock_cmd.side_effect = subprocess.CalledProcessError(1, "test")
            assert dpuctl_obj.run_cmd_output(["test"], raise_exception=False) is None

    def test_write_file(self, dpuctl_obj):
        """Test file writing functionality"""
        with patch('sonic_platform.utils.write_file') as mock_write:
            mock_write.return_value = True
            assert dpuctl_obj.write_file("test_file", "test_content")
            mock_write.assert_called_once_with("test_file", "test_content", raise_exception=True)

            mock_write.side_effect = Exception("Write error")
            with pytest.raises(Exception) as exc:
                dpuctl_obj.write_file("test_file", "test_content")
            assert "Write error" in str(exc.value)

    def test_get_hwmgmt_name(self, dpuctl_obj):
        """Test hardware management name generation"""
        assert dpuctl_obj.get_hwmgmt_name() == "dpu1"  # dpu0 -> dpu1
        dpuctl_obj.dpu_name = "dpu1"
        dpuctl_obj.dpu_id = 1
        assert dpuctl_obj.get_hwmgmt_name() == "dpu2"  # dpu1 -> dpu2

class TestDpuCtlPlatStatus:
    """Tests for status monitoring functionality"""

    def test_boot_progress(self, dpuctl_obj):
        """Test boot progress monitoring"""
        dpuctl_obj.boot_prog_path = os.path.join(test_path, 'mock_dpu_boot_prog')

        class DummyPoller:
            def poll(self):
                return True

        with patch("sonic_platform.utils.read_int_from_file") as mock_read:
            # Test known boot progress states
            for state in BootProgEnum:
                mock_read.return_value = state.value
                dpuctl_obj.update_boot_prog_once(DummyPoller())
                assert dpuctl_obj.boot_prog_state == state.value
                assert dpuctl_obj.boot_prog_indication == f"{state.value} - {dpuctl_obj.boot_prog_map[state.value]}"

            # Test unknown boot progress state
            mock_read.return_value = 99
            dpuctl_obj.update_boot_prog_once(DummyPoller())
            assert dpuctl_obj.boot_prog_state == 99
            assert dpuctl_obj.boot_prog_indication == "99 - N/A"

    def test_status_updates(self, dpuctl_obj):
        """Test DPU status updates"""
        with patch("sonic_platform.utils.read_int_from_file") as mock_read:
            # Test ready state
            mock_read.return_value = 1
            dpuctl_obj.dpu_ready_update()
            assert dpuctl_obj.dpu_ready_state == 1
            assert dpuctl_obj.dpu_ready_indication == "True"

            # Test shutdown ready state
            mock_read.return_value = 1
            dpuctl_obj.dpu_shtdn_ready_update()
            assert dpuctl_obj.dpu_shtdn_ready_state == 1
            assert dpuctl_obj.dpu_shtdn_ready_indication == "True"

            # Test invalid states
            mock_read.return_value = 25
            dpuctl_obj.dpu_ready_update()
            assert dpuctl_obj.dpu_ready_indication == "25 - N/A"
            dpuctl_obj.dpu_shtdn_ready_update()
            assert dpuctl_obj.dpu_shtdn_ready_indication == "25 - N/A"

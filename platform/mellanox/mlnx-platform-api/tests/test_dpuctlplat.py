#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
import errno
import os
import sys
import time
import pytest
import subprocess
from unittest.mock import MagicMock, patch, Mock, call

from sonic_platform.dpuctlplat import (
    DpuCtlPlat, BootProgEnum, PCI_DEV_BASE, OperationType,
    WAIT_FOR_SHTDN, WAIT_FOR_DPU_READY,
    MLX5_CORE_BIND_PATH, MLX5_CORE_UNBIND_PATH,
)
from sonic_platform.device_data import DpuInterfaceEnum

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)
scripts_path = os.path.join(modules_path, "scripts")

# Test data
TEST_DPU_LIST = ['dpu0', 'dpu1', 'dpu2', 'dpu3']
TEST_PCI_BDF = "0000:08:00.0"
TEST_RSHIM_PCI_BDF = "0000:08:00.1"
TEST_PCI_PATH = os.path.join(PCI_DEV_BASE, TEST_PCI_BDF)
TEST_RSHIM_PCI_PATH = os.path.join(PCI_DEV_BASE, TEST_RSHIM_PCI_BDF)

@pytest.fixture
def dpuctl_obj():
    """Fixture to create a DpuCtlPlat object for testing"""
    obj = DpuCtlPlat('dpu0')
    obj.setup_logger(True)
    obj.pci_dev_path = [TEST_PCI_PATH, TEST_RSHIM_PCI_PATH]
    obj.pci_dev_path_map = {
        DpuInterfaceEnum.PCIE_INT: TEST_PCI_PATH,
        DpuInterfaceEnum.RSHIM_PCIE_INT: TEST_RSHIM_PCI_PATH,
    }
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
        # Test that the logger functions add timestamps. Patch logger so it doesn't
        # run in test (avoids socket errors and extra print calls from errors).
        with patch('time.strftime') as mock_time:
            mock_time.return_value = "2024-01-01 12:00:00"
            with patch('builtins.print') as mock_print, \
                 patch('sonic_platform.dpuctlplat.logger.log_notice'):
                dpuctl_obj.logger_info("test message")
                mock_print.assert_called_once_with("[2024-01-01 12:00:00] test message")

        # Test with syslogger mode
        dpuctl_obj.setup_logger(False)
        assert dpuctl_obj.logger_info != print
        assert dpuctl_obj.logger_error != print
        assert dpuctl_obj.logger_debug != print
        assert dpuctl_obj.logger_warning != print

    def test_get_pci_dev_path(self, dpuctl_obj):
        """Test PCI device path retrieval"""
        # Reset both caches so the resolver actually runs.
        dpuctl_obj.pci_dev_path = []
        dpuctl_obj.pci_dev_path_map = {}

        # Test with both PCI and RSHIM paths
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = [TEST_PCI_BDF, TEST_RSHIM_PCI_BDF]
            paths = dpuctl_obj.get_pci_dev_path()
            assert len(paths) == 2
            assert paths[0].endswith(TEST_PCI_BDF)
            assert paths[1].endswith(TEST_RSHIM_PCI_BDF)

        # Test with missing PCIE_INT path: error names the missing interface.
        dpuctl_obj.pci_dev_path = []
        dpuctl_obj.pci_dev_path_map = {}
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = [None, TEST_RSHIM_PCI_BDF]
            with pytest.raises(RuntimeError) as exc:
                dpuctl_obj.get_pci_dev_path()
            assert "Unable to obtain PCI device ID" in str(exc.value)
            assert DpuInterfaceEnum.PCIE_INT.value in str(exc.value)

        # Test with missing RSHIM_PCIE_INT path: error names the missing interface.
        dpuctl_obj.pci_dev_path = []
        dpuctl_obj.pci_dev_path_map = {}
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = [TEST_PCI_BDF, None]
            with pytest.raises(RuntimeError) as exc:
                dpuctl_obj.get_pci_dev_path()
            assert "Unable to obtain PCI device ID" in str(exc.value)
            assert DpuInterfaceEnum.RSHIM_PCIE_INT.value in str(exc.value)

    def test_get_pci_dev_path_map(self, dpuctl_obj):
        """Test deterministic name-keyed map of DPU PCI device paths."""
        dpuctl_obj.pci_dev_path = []
        dpuctl_obj.pci_dev_path_map = {}

        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            mock_get.side_effect = [TEST_PCI_BDF, TEST_RSHIM_PCI_BDF]
            path_map = dpuctl_obj.get_pci_dev_path_map()
            assert set(path_map.keys()) == {
                DpuInterfaceEnum.PCIE_INT,
                DpuInterfaceEnum.RSHIM_PCIE_INT,
            }
            assert path_map[DpuInterfaceEnum.PCIE_INT].endswith(TEST_PCI_BDF)
            assert path_map[DpuInterfaceEnum.RSHIM_PCIE_INT].endswith(TEST_RSHIM_PCI_BDF)

        # Result is cached; a second call shouldn't re-query DeviceDataManager.
        with patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_get:
            same_map = dpuctl_obj.get_pci_dev_path_map()
            assert same_map is path_map
            mock_get.assert_not_called()

        # get_pci_dev_path() should be derived from the same source of truth
        # in the order declared by PCI_DEV_INTERFACES.
        dpuctl_obj.pci_dev_path = []
        derived = dpuctl_obj.get_pci_dev_path()
        assert derived == [
            path_map[DpuCtlPlat.PCI_DEV_INTERFACES[0]],
            path_map[DpuCtlPlat.PCI_DEV_INTERFACES[1]],
        ]

class TestDpuCtlPlatPCI:
    """Tests for PCI-related functionality"""

    def test_pci_operations(self, dpuctl_obj):
        """Test PCI unbind/bind operations.

        ``dpu_pci_remove`` unbinds PCIE_INT from mlx5_core and intentionally
        skips the RSHIM/SoC device. ``dpu_pci_scan`` binds PCIE_INT
        back, or logs a skip message when the driver is already bound.
        """
        written_data = []
        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name, "data": content_towrite})
            return True

        # PCI remove with driver bound: writes BDF to mlx5_core/unbind.
        # No write is performed for RSHIM_PCIE_INT (skipped intentionally).
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', return_value=True):
            assert dpuctl_obj.dpu_pci_remove()
            assert len(written_data) == 1
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[0]["data"] == TEST_PCI_BDF

        # PCI remove no-op when the driver isn't bound: no writes, still True.
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', return_value=False), \
             patch.object(dpuctl_obj, 'log_debug') as mock_dbg:
            assert dpuctl_obj.dpu_pci_remove()
            assert written_data == []
            assert any("Skipping unbind" in c.args[0] for c in mock_dbg.call_args_list)

        # PCI scan when the driver is already bound: skip bind, log info,
        # no write happens.
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', return_value=True), \
             patch.object(dpuctl_obj, 'log_info') as mock_log:
            assert dpuctl_obj.dpu_pci_scan()
            assert written_data == []
            assert any("skip bind" in c.args[0] for c in mock_log.call_args_list)

        # PCI scan when the driver isn't bound: writes BDF to mlx5_core/bind.
        written_data.clear()

        def exists_for_bind(path):
            # Driver symlink absent; device + bind path present.
            return not path.endswith("/driver")

        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', side_effect=exists_for_bind):
            assert dpuctl_obj.dpu_pci_scan()
            assert len(written_data) == 1
            assert written_data[0]["file"] == MLX5_CORE_BIND_PATH
            assert written_data[0]["data"] == TEST_PCI_BDF

        # PCI scan when the PCIE_INT device itself is missing from the bus:
        # warning logged, no bind write performed.
        written_data.clear()

        def exists_missing_pci(path):
            if path == TEST_PCI_PATH:
                return False
            if path.endswith("/driver"):
                return False
            return True

        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', side_effect=exists_missing_pci), \
             patch.object(dpuctl_obj, 'log_warning') as mock_warn:
            assert dpuctl_obj.dpu_pci_scan()
            assert written_data == []
            assert any(TEST_PCI_PATH in c.args[0] for c in mock_warn.call_args_list)

        # PCI scan when the PCIE_INT device is present but mlx5_core bind path
        # is missing: warning logged about the bind path, no bind write performed.
        written_data.clear()

        def exists_no_bindpath(path):
            if path == TEST_PCI_PATH:
                return True
            if path.endswith("/driver"):
                return False
            if path == MLX5_CORE_BIND_PATH:
                return False
            return True

        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', side_effect=exists_no_bindpath), \
             patch.object(dpuctl_obj, 'log_warning') as mock_warn:
            assert dpuctl_obj.dpu_pci_scan()
            assert written_data == []
            assert any(
                MLX5_CORE_BIND_PATH in c.args[0]
                for c in mock_warn.call_args_list
            )

        # PCI scan when RSHIM/SoC device hasn't reappeared: warning logged.
        written_data.clear()

        def exists_missing_rshim(path):
            if path == TEST_RSHIM_PCI_PATH:
                return False
            return True  # driver_link present, so bind is skipped (log only)

        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', side_effect=exists_missing_rshim), \
             patch.object(dpuctl_obj, 'log_warning') as mock_warn:
            assert dpuctl_obj.dpu_pci_scan()
            assert written_data == []
            assert any(TEST_RSHIM_PCI_PATH in c.args[0] for c in mock_warn.call_args_list)

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
            # 1 unbind (PCIE_INT, RSHIM intentionally skipped) + rst + pwr_force
            assert len(written_data) == 3
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[0]["data"] == TEST_PCI_BDF
            assert written_data[1]["data"] == "0"  # rst
            assert written_data[2]["data"] == "0"  # pwr_force

        # Test normal power off
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_power_off(False)
            # 1 unbind + rst (from dpu_go_down) + pwr
            assert len(written_data) == 3
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[0]["data"] == TEST_PCI_BDF
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"].endswith("_pwr")

        # Test power off when already off
        with patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'log_info') as mock_log:
            assert dpuctl_obj.dpu_power_off(False)
            assert "Skipping DPU power off as DPU is already powered off" in mock_log.call_args_list[-1].args[0]

        # Test power off with skip_pre_post=True
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_power_off(False, skip_pre_post=True)
            assert len(written_data) == 2  # Only rst and pwr operations
            # Pre-shutdown skipped, so no mlx5_core unbind happens.
            assert not any(d["file"] == MLX5_CORE_UNBIND_PATH for d in written_data)
            assert written_data[0]["file"].endswith("_rst")
            assert written_data[1]["file"].endswith("_pwr")

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

        # Test force power on. With os.path.exists mocked True, dpu_pci_scan
        # sees the driver as already bound and logs a skip message instead
        # of writing to mlx5_core/bind.
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1), \
             patch.object(dpuctl_obj, 'log_info') as mock_log:
            assert dpuctl_obj.dpu_power_on(True)
            assert len(written_data) == 2  # pwr_force + rst (scan skipped: already bound)
            assert written_data[0]["file"].endswith("_pwr_force")
            assert written_data[0]["data"] == "1"
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[1]["data"] == "1"
            assert any("skip bind" in c.args[0] for c in mock_log.call_args_list)
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)

        # Test normal power on
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1):
            assert dpuctl_obj.dpu_power_on(False)
            assert len(written_data) == 2  # pwr + rst (scan skipped: already bound)
            assert written_data[0]["file"].endswith("_pwr")
            assert written_data[1]["file"].endswith("_rst")
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)

        # Test normal power on when driver isn't bound: dpu_post_startup
        # should actually bind PCIE_INT via mlx5_core/bind.
        written_data.clear()

        def exists_for_bind(path):
            return not path.endswith("/driver")

        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch('os.path.exists', side_effect=exists_for_bind), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1):
            assert dpuctl_obj.dpu_power_on(False)
            assert len(written_data) == 3  # pwr + rst + bind
            assert written_data[0]["file"].endswith("_pwr")
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"] == MLX5_CORE_BIND_PATH
            assert written_data[2]["data"] == TEST_PCI_BDF

        # Test power on with skip_pre_post=True
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.RST.value), \
             patch.object(dpuctl_obj, 'read_force_power_path', return_value=1):
            assert dpuctl_obj.dpu_power_on(False, skip_pre_post=True)
            assert len(written_data) == 2  # Only pwr and rst operations
            # Post-startup skipped, so no mlx5_core bind happens.
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)
            assert written_data[0]["file"].endswith("_pwr")
            assert written_data[1]["file"].endswith("_rst")

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
            # 1 unbind + rst (from dpu_go_down) + rst (from _reboot)
            # Scan skipped: driver appears bound under the os.path.exists=True mock.
            assert len(written_data) == 3
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[0]["data"] == TEST_PCI_BDF
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"].endswith("_rst")
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)

        # Test force reboot
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(True)
            # 1 unbind + rst + pwr_force (off) + pwr_force + rst (on); scan skipped.
            assert len(written_data) == 5
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"].endswith("_pwr_force")
            assert written_data[3]["file"].endswith("_pwr_force")
            assert written_data[4]["file"].endswith("_rst")
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)

        # Test no-wait reboot
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(no_wait=True)
            # 1 unbind + rst (from dpu_go_down) + rst (from _reboot); post-startup skipped on no_wait.
            assert len(written_data) == 3
            assert written_data[0]["file"] == MLX5_CORE_UNBIND_PATH
            assert written_data[1]["file"].endswith("_rst")
            assert written_data[2]["file"].endswith("_rst")

        # Test reboot with skip_pre_post=True
        written_data.clear()
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', return_value=BootProgEnum.OS_RUN.value):
            assert dpuctl_obj.dpu_reboot(skip_pre_post=True)
            assert len(written_data) == 2  # Only rst operations
            assert all("_rst" in data["file"] for data in written_data)
            # Neither pre-shutdown (unbind) nor post-startup (bind) runs.
            assert not any(d["file"] == MLX5_CORE_UNBIND_PATH for d in written_data)
            assert not any(d["file"] == MLX5_CORE_BIND_PATH for d in written_data)

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

    def test_read_boot_prog_retries_on_enxio(self, dpuctl_obj):
        """read_boot_prog retries twice on ENXIO with 1s delay, then returns value."""
        dpuctl_obj.boot_prog_path = os.path.join(test_path, "mock_dpu_boot_prog")
        enxio = OSError(errno.ENXIO, os.strerror(errno.ENXIO))

        with patch("sonic_platform.utils.read_int_from_file") as mock_read, \
                patch("sonic_platform.dpuctlplat.time.sleep") as mock_sleep:
            mock_read.side_effect = [enxio, enxio, 5]
            assert dpuctl_obj.read_boot_prog() == 5
            assert mock_read.call_count == 3
            mock_sleep.assert_has_calls([call(1), call(1)])

    def test_read_boot_prog_raises_after_enxio_retries_exhausted(self, dpuctl_obj):
        """After three ENXIO failures, the last error is propagated."""
        dpuctl_obj.boot_prog_path = os.path.join(test_path, "mock_dpu_boot_prog")
        enxio = OSError(errno.ENXIO, os.strerror(errno.ENXIO))

        with patch("sonic_platform.utils.read_int_from_file") as mock_read, \
                patch("sonic_platform.dpuctlplat.time.sleep") as mock_sleep:
            mock_read.side_effect = [enxio, enxio, enxio]
            with pytest.raises(OSError) as excinfo:
                dpuctl_obj.read_boot_prog()
            assert excinfo.value.errno == errno.ENXIO
            assert mock_read.call_count == 3
            mock_sleep.assert_has_calls([call(1), call(1)])

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

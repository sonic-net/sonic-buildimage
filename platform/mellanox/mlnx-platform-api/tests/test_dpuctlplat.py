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

"""dpuctlplat Tests Implementation"""
import os
import sys
import pytest
from sonic_platform.dpuctlplat import DpuCtlPlat, BootProgEnum, PCI_DEV_BASE, OperationType

from unittest.mock import MagicMock, patch, Mock, call

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)
scripts_path = os.path.join(modules_path, "scripts")


def create_dpu_list():
    """Create dpu object list for Function calls"""
    existing_dpu_list = ['dpu0', 'dpu1', 'dpu2', 'dpu3']
    dpuctl_list = []
    for dpu_name in existing_dpu_list:
        dpuctl_obj = DpuCtlPlat(dpu_name)
        dpuctl_obj.setup_logger(True)
        dpuctl_list.append(dpuctl_obj)
    context = {
        "dpuctl_list": dpuctl_list,
    }
    return context


obj = create_dpu_list()

rshim_interface = "rshim@0"
pci_dev_path = os.path.join(PCI_DEV_BASE, "0000:08:00.0", 'remove')


class TestDpuClass:
    """Tests for dpuctl Platform API Wrapper"""
    @classmethod
    def setup_class(cls):
        """Setup function for all tests for dpuctl implementation"""
        os.environ["PATH"] += os.pathsep + scripts_path
        os.environ["MLNX_PLATFORM_API_DPUCTL_UNIT_TESTING"] = "2"
        dpuctl_obj = obj["dpuctl_list"][0]
        dpuctl_obj.rshim_interface = rshim_interface
        dpuctl_obj.pci_dev_path = pci_dev_path

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('multiprocessing.Process.start', MagicMock(return_value=True))
    @patch('multiprocessing.Process.is_alive', MagicMock(return_value=False))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    @patch('subprocess.check_output', MagicMock(return_value=True))
    def test_power_off(self, mock_inotify, mock_add_watch):
        """Tests for Per DPU Power Off function"""
        dpuctl_obj = obj["dpuctl_list"][0]
        mock_inotify.return_value = None
        mock_add_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name,
                                 "data": content_towrite})
            return True
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', MagicMock(return_value=BootProgEnum.OS_RUN.value)):
            assert dpuctl_obj.dpu_power_off(True)
            print(f"{written_data}")
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert "0" == written_data[1]["data"]
            assert written_data[1]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[2]["data"]
            assert written_data[2]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            written_data = []
            assert dpuctl_obj.dpu_power_off(False)
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_shtdn_ready")
            assert written_data[0]["file"].endswith(
                f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr")
            assert "0" == written_data[2]["data"]
            written_data = []
            mock_add_watch.return_value = None
            assert dpuctl_obj.dpu_power_off(False)
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_shtdn_ready")
            assert written_data[0]["file"].endswith(
                f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[2]["data"]
            assert written_data[3]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "0" == written_data[3]["data"]
        # Test whether value of boot_progress skips power off
        with patch.object(dpuctl_obj, 'read_boot_prog') as mock_boot_prog, \
             patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, '_power_off_force') as mock_power_off_force, \
             patch.object(dpuctl_obj, '_power_off') as mock_power_off, \
             patch.object(dpuctl_obj, 'log_info') as mock_obj:
            mock_boot_prog.return_value = BootProgEnum.RST.value
            mock_add_watch.return_value = True
            assert dpuctl_obj.dpu_power_off(False)
            assert mock_obj.call_args_list[1].args[0] == "Skipping DPU power off as DPU is already powered off"

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('multiprocessing.Process.start', MagicMock(return_value=True))
    @patch('multiprocessing.Process.is_alive', MagicMock(return_value=False))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    @patch('subprocess.check_output', MagicMock(return_value=True))
    def test_power_on(self, mock_inotify, mock_add_watch):
        """Tests for Per DPU Power On function"""
        dpuctl_obj = obj["dpuctl_list"][0]
        mock_inotify.return_value = None
        mock_add_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name,
                                 "data": content_towrite})
            return True
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'wait_for_pci', wraps=MagicMock(return_value=None)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control', wraps=MagicMock(return_value=None)), \
             patch.object(dpuctl_obj, 'read_boot_prog', wraps=MagicMock(return_value=BootProgEnum.RST.value)), \
             patch.object(dpuctl_obj, 'read_force_power_path') as mock_pwr_force_read:
            mock_pwr_force_read.return_value = 1
            assert dpuctl_obj.dpu_power_on(True)
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_ready")
            assert written_data[0]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[1]["data"]
            written_data = []
            assert dpuctl_obj.dpu_power_on(False)
            assert written_data[0]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"rescan")
            assert "1" == written_data[2]["data"]
            written_data = []
            mock_add_watch.return_value = None
            assert not dpuctl_obj.dpu_power_on(False)
            assert len(written_data) == 19
            assert written_data[0]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[1]["data"]
            for i in range(4):
                assert written_data[2 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "0" == written_data[2 + 4 * i]["data"]
                assert written_data[3 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "0" == written_data[3 + 4 * i]["data"]
                assert written_data[4 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "1" == written_data[4 + 4 * i]["data"]
                assert written_data[5 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "1" == written_data[5 + 4 * i]["data"]
            assert written_data[18]["file"].endswith(f"rescan")
            assert "1" == written_data[18]["data"]
            written_data = []
            mock_add_watch.return_value = True
            mock_pwr_force_read.return_value = 0
            mock_inotify.reset_mock()
            assert dpuctl_obj.dpu_power_on(False)
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_ready")
            assert written_data[0]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[1]["data"]

    @patch('os.path.exists', MagicMock(return_value=True))
    @patch('multiprocessing.Process.start', MagicMock(return_value=None))
    @patch('multiprocessing.Process.is_alive', MagicMock(return_value=False))
    @patch('sonic_platform.inotify_helper.InotifyHelper.wait_watch')
    @patch('sonic_platform.inotify_helper.InotifyHelper.__init__')
    def test_dpu_reset(self, mock_inotify, mock_add_watch):
        """Tests for Per DPU Reset function"""
        dpuctl_obj = obj["dpuctl_list"][0]
        mock_inotify.return_value = None
        mock_add_watch.return_value = True
        written_data = []

        def mock_write_file(file_name, content_towrite):
            written_data.append({"file": file_name,
                                 "data": content_towrite})
            return True
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', MagicMock(return_value=BootProgEnum.OS_RUN.value)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control', wraps=MagicMock(return_value=None)):
            dpuctl_obj.write_file = mock_write_file
            assert dpuctl_obj.dpu_reboot(False)
            assert len(written_data) == 4
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[2]["data"]
            assert written_data[3]["file"].endswith(f"rescan")
            assert "1" == written_data[3]["data"]
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_ready")
            mock_add_watch.return_value = None
            written_data = []
            assert not dpuctl_obj.dpu_reboot()
            assert len(written_data) == 22
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[2]["data"]
            assert written_data[3]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "0" == written_data[3]["data"]
            assert written_data[4]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[4]["data"]
            for i in range(4):
                assert written_data[5 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "0" == written_data[5 + 4 * i]["data"]
                assert written_data[6 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "0" == written_data[6 + 4 * i]["data"]
                assert written_data[7 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "1" == written_data[7 + 4 * i]["data"]
                assert written_data[8 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "1" == written_data[8 + 4 * i]["data"]
            assert written_data[21]["file"].endswith(f"rescan")
            assert "1" == written_data[21]["data"]
        # Force Reboot
        mock_inotify.reset_mock()
        mock_add_watch.return_value = True
        mock_inotify.return_value = None
        written_data = []
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', MagicMock(return_value=BootProgEnum.OS_RUN.value)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control', wraps=MagicMock(return_value=None)):
            dpuctl_obj.write_file = mock_write_file
            assert dpuctl_obj.dpu_reboot(True)
            mock_add_watch.return_value = None
            assert len(written_data) == 6
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "0" == written_data[2]["data"]
            assert written_data[3]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "1" == written_data[3]["data"]
            assert written_data[4]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[4]["data"]
            assert written_data[5]["file"].endswith(f"rescan")
            assert "1" == written_data[5]["data"]
            assert mock_inotify.call_args.args[0].endswith(
                f"{dpuctl_obj.get_hwmgmt_name()}_ready")
            mock_add_watch.return_value = None
            written_data = []
            assert not dpuctl_obj.dpu_reboot(True)
            assert len(written_data) == 18
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "0" == written_data[2]["data"]
            assert written_data[3]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
            assert "1" == written_data[3]["data"]
            assert written_data[4]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[4]["data"]
            for i in range(3):
                assert written_data[5 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "0" == written_data[5 + 4 * i]["data"]
                assert written_data[6 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "0" == written_data[6 + 4 * i]["data"]
                assert written_data[7 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_pwr_force")
                assert "1" == written_data[7 + 4 * i]["data"]
                assert written_data[8 + 4 * i]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
                assert "1" == written_data[8 + 4 * i]["data"]
            assert written_data[17]["file"].endswith(f"rescan")
            assert "1" == written_data[17]["data"]
        written_data = []
        mock_inotify.reset_mock()
        mock_add_watch.reset_mock()
        mock_inotify.return_value = None
        mock_add_watch.return_value = True
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', MagicMock(return_value=BootProgEnum.OS_RUN.value)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control') as mock_rshim:
            assert dpuctl_obj.dpu_reboot(forced=False, no_wait=True)
            # Rshim service is only stopped and not started
            mock_rshim.assert_called_once()
            mock_rshim.call_args.args[0] == "stop"
            assert written_data[0]["file"].endswith(f"{pci_dev_path}")
            assert "1" == written_data[0]["data"]
            assert written_data[1]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[1]["data"]
            assert written_data[2]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "1" == written_data[2]["data"]
            mock_inotify.called_once()
            mock_add_watch.called_once()
        # Skip pre startup and post shutdown
        written_data = []
        with patch.object(dpuctl_obj, 'write_file', wraps=mock_write_file), \
             patch.object(dpuctl_obj, 'read_boot_prog', MagicMock(return_value=BootProgEnum.OS_START.value)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control') as mock_rshim:
            assert dpuctl_obj.dpu_reboot(skip_pre_post=True)
            mock_rshim.assert_not_called()
            # We skip writing PCI data
            assert written_data[0]["file"].endswith(f"{dpuctl_obj.get_hwmgmt_name()}_rst")
            assert "0" == written_data[0]["data"]
            assert not written_data[-1]["file"].endswith("rescan")

    def test_prog_update(self):
        dpuctl_obj = obj["dpuctl_list"][0]
        dpuctl_obj.boot_prog_path = os.path.join(test_path, 'mock_dpu_boot_prog')

        class Dummy:
            def poll(self):
                return True
        dummy_obj = Dummy()
        mock_file_path = "mock_dpu_boot_prog"
        mock_val = 0
        boot_prog_map = dpuctl_obj.boot_prog_map

        def mock_read_int_from_file(file_path, default=0, raise_exception=False, log_func=None):
            if file_path.endswith(mock_file_path):
                return mock_val
            else:
                return 0
        with patch("sonic_platform.utils.read_int_from_file", wraps=mock_read_int_from_file), \
             patch.object(dpuctl_obj, 'wait_for_pci', wraps=MagicMock(return_value=None)), \
             patch.object(dpuctl_obj, 'dpu_rshim_service_control', wraps=MagicMock(return_value=None)):
            for key_val in boot_prog_map.keys():
                mock_val = key_val
                dpuctl_obj.update_boot_prog_once(dummy_obj)
                assert dpuctl_obj.boot_prog_state == key_val
                assert dpuctl_obj.boot_prog_indication == f"{key_val} - {boot_prog_map.get(key_val)}"
            mock_val = 25
            dpuctl_obj.update_boot_prog_once(dummy_obj)
            assert dpuctl_obj.boot_prog_state == 25
            assert dpuctl_obj.boot_prog_indication == "25 - N/A"
            mock_val = 36
            dpuctl_obj.update_boot_prog_once(dummy_obj)
            assert dpuctl_obj.boot_prog_state == 36
            assert dpuctl_obj.boot_prog_indication == "36 - N/A"
            mock_file_path = "dpu1_ready"
            mock_val = 1
            dpuctl_obj.dpu_status_update()
            assert dpuctl_obj.boot_prog_state == 0
            assert dpuctl_obj.boot_prog_indication == f"0 - {boot_prog_map.get(0)}"
            assert dpuctl_obj.dpu_ready_state == 1
            assert dpuctl_obj.dpu_ready_indication == f"True"
            assert dpuctl_obj.dpu_shtdn_ready_state == 0
            assert dpuctl_obj.dpu_shtdn_ready_indication == f"False"
            mock_file_path = "dpu1_shtdn_ready"
            dpuctl_obj.dpu_status_update()
            assert dpuctl_obj.boot_prog_state == 0
            assert dpuctl_obj.boot_prog_indication == f"0 - {boot_prog_map.get(0)}"
            assert dpuctl_obj.dpu_ready_state == 0
            assert dpuctl_obj.dpu_ready_indication == "False"
            assert dpuctl_obj.dpu_shtdn_ready_state == 1
            assert dpuctl_obj.dpu_shtdn_ready_indication == "True"
            mock_file_path = "dpu1_shtdn_ready"
            mock_val = 25
            dpuctl_obj.dpu_status_update()
            assert dpuctl_obj.dpu_shtdn_ready_indication == "25 - N/A"
            mock_file_path = "dpu1_ready"
            mock_val = 50
            dpuctl_obj.dpu_status_update()
            assert dpuctl_obj.dpu_ready_indication == "50 - N/A"

    @patch('os.path.exists')
    @patch('os.open', MagicMock(return_value=-1))
    @patch('os.close', MagicMock(return_value=None))
    @patch('sonic_platform.dpuctlplat.poll')
    def test_pci_func(self, m1, mock_exists):
        dpuctl_obj = obj["dpuctl_list"][0]
        mock_exists.return_value = False
        mock_obj = Mock()
        mock_obj.register.return_value = None
        mock_obj.poll.return_value = None
        m1.return_value = mock_obj
        timeout_val = 45

        def mock_time_diff():
            mock_time_diff.counter += 1
            return mock_time_diff.counter * timeout_val
        mock_time_diff.counter = 0
        with patch("time.monotonic", wraps=mock_time_diff):
            # PCI Device is not recognized
            assert not dpuctl_obj.wait_for_pci()
            pci_parent_path = os.path.dirname(pci_dev_path)
            assert pci_parent_path == mock_exists.call_args.args[0]
            mock_obj.register.assert_called_once()
            mock_obj.poll.assert_called_once()
            # PCI device is recognized immediately
            mock_obj.reset_mock()
            mock_exists.reset_mock()
            mock_exists.return_value = True
            assert dpuctl_obj.wait_for_pci()
            assert pci_parent_path == mock_exists.call_args.args[0]
            mock_obj.register.assert_not_called()
            mock_obj.poll.assert_not_called()
            # PCI device is added later (Detected in Loop)
            timeout_val = 20
            mock_exists.reset_mock()
            mock_obj.reset_mock()
            mock_exists.side_effect = [False, True]
            mock_obj.poll.return_value = True
            assert dpuctl_obj.wait_for_pci()
            mock_obj.register.assert_called_once()
            mock_obj.poll.assert_called_once()
            # PCI device is added later (Detected at the end)
            timeout_val = 80
            mock_exists.reset_mock()
            mock_obj.reset_mock()
            mock_exists.side_effect = [False, True]
            assert dpuctl_obj.wait_for_pci()
            mock_obj.register.assert_called_once()
            mock_obj.poll.assert_not_called()
        with patch.object(dpuctl_obj, 'pci_dev_path', None), \
             patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_int,\
             patch.object(dpuctl_obj, 'log_error') as mock_obj:
            mock_int.return_value = None
            dpuctl_obj.wait_for_pci()
            mock_obj.assert_called_once_with("Unable to wait for PCI device:Unable to obtain pci device id for dpu0 from platform.json")
            new_pci_dev_id = "0000:05:00.0"
            mock_int.return_value = new_pci_dev_id
            dpuctl_obj.wait_for_pci()
            assert dpuctl_obj.pci_dev_path.endswith(f"{new_pci_dev_id}/remove")
            # pci dev_path is cached
            mock_int.reset_mock()
            mock_int.return_value = "None"
            dpuctl_obj.wait_for_pci()
            mock_int.assert_not_called()
            assert dpuctl_obj.pci_dev_path.endswith(f"{new_pci_dev_id}/remove")

    def test_rshim_service(self):
        dpuctl_obj = obj["dpuctl_list"][0]
        with patch.object(dpuctl_obj, 'run_cmd_output') as mock_method:
            dpuctl_obj.dpu_rshim_service_control('start')
            mock_method.assert_called_once()
            cmd_string = ' '.join(mock_method.call_args.args[0])
            service_name = rshim_interface
            operation = "Start"
            assert (operation in cmd_string) and (service_name in cmd_string)
            mock_method.reset_mock()
            operation = "Stop"
            dpuctl_obj.dpu_rshim_service_control('stop')
            cmd_string = ' '.join(mock_method.call_args.args[0])
            assert (operation in cmd_string) and (service_name in cmd_string)
            mock_method.assert_called_once()
            with pytest.raises(TypeError):
                dpuctl_obj.dpu_rshim_service_control()
            with patch.object(dpuctl_obj, 'rshim_interface', None), \
                 patch('sonic_platform.device_data.DeviceDataManager.get_dpu_interface') as mock_int,\
                 patch.object(dpuctl_obj, 'log_error') as mock_obj:
                mock_int.return_value = None
                dpuctl_obj.dpu_rshim_service_control('start')
                mock_obj.assert_called_once_with("Failed to start rshim!: Unable to Parse rshim information for dpu0 from Platform.json")
                mock_int.return_value = "rshim1"
                dpuctl_obj.dpu_rshim_service_control('start')
                assert dpuctl_obj.rshim_interface == "rshim@1"
                mock_int.reset_mock()
                mock_int.return_value = "rshim20"
                dpuctl_obj.dpu_rshim_service_control('start')
                # Rshim name is cached
                mock_int.assert_not_called()
                assert dpuctl_obj.rshim_interface == "rshim@1"

    def test_pre_and_post(self):
        dpuctl_obj = obj["dpuctl_list"][0]
        with patch.object(dpuctl_obj, 'dpu_rshim_service_control') as mock_rshim, patch.object(dpuctl_obj, 'write_file') as mock_write:
            manager_mock = Mock()
            manager_mock.attach_mock(mock_rshim, 'rshim')
            manager_mock.attach_mock(mock_write, 'write')
            mock_rshim.return_value = True
            mock_write.return_value = True
            assert dpuctl_obj.dpu_pre_shutdown()
            mock_rshim.assert_called_once()
            mock_write.assert_called_once()
            # Confirm the order of calls and the parameters
            manager_mock.mock_calls[0] == call.rshim('stop')
            manager_mock.mock_calls[1] == call.rshim(dpuctl_obj.pci_dev_path, '1')
            mock_rshim.return_value = False
            assert not dpuctl_obj.dpu_pre_shutdown()
            mock_rshim.return_value = True
            # Test post startup
            mock_rshim.reset_mock()
            mock_write.reset_mock()
            manager_mock.reset_mock()
            with patch.object(dpuctl_obj, 'wait_for_pci') as mock_pci:
                manager_mock.attach_mock(mock_rshim, 'rshim')
                manager_mock.attach_mock(mock_write, 'write')
                manager_mock.attach_mock(mock_pci, 'pci')
                dpuctl_obj.dpu_post_startup()
                mock_rshim.assert_called_once()
                mock_write.assert_called_once()
                mock_pci.assert_called_once()
                # Confirm the order of calls and the parameters
                manager_mock.mock_calls[0] == call.rshim('/sys/bus/pci/rescan', '1')
                manager_mock.mock_calls[1] == call.pci()
                manager_mock.mock_calls[2] == call.rshim('start')
                mock_rshim.return_value = False
                assert not dpuctl_obj.dpu_post_startup()
        with patch.object(dpuctl_obj, 'write_file', side_effect=Exception("Mock")), \
             patch.object(dpuctl_obj, 'run_cmd_output', MagicMock(return_value=True)):
            assert not dpuctl_obj.dpu_pre_shutdown()
        with patch.object(dpuctl_obj, 'run_cmd_output', side_effect=Exception("Mock")), \
             patch.object(dpuctl_obj, 'dpu_pci_remove', MagicMock(return_value=True)):
            assert not dpuctl_obj.dpu_pre_shutdown()
        with patch.object(dpuctl_obj, 'write_file', side_effect=Exception("Mock")), \
             patch.object(dpuctl_obj, 'wait_for_pci', MagicMock(return_value=True)), \
             patch.object(dpuctl_obj, 'run_cmd_output', MagicMock(return_value=True)):
            assert not dpuctl_obj.dpu_post_startup()
        with patch.object(dpuctl_obj, 'run_cmd_output', side_effect=Exception("Mock")), \
             patch.object(dpuctl_obj, 'wait_for_pci', MagicMock(return_value=True)), \
             patch.object(dpuctl_obj, 'dpu_pci_scan', MagicMock(return_value=True)):
            assert not dpuctl_obj.dpu_post_startup()

    @classmethod
    def teardown_class(cls):
        """Teardown function for all tests for dpuctl implementation"""
        os.environ["MLNX_PLATFORM_API_DPUCTL_UNIT_TESTING"] = "0"
        os.environ["PATH"] = os.pathsep.join(
            os.environ["PATH"].split(os.pathsep)[:-1])

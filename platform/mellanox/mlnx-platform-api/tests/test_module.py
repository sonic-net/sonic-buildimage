#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

import os
import sys
import pytest
import sonic_platform.utils
import subprocess
from mock import patch
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

import sonic_platform.chassis
from sonic_platform import utils
from sonic_platform.chassis import ModularChassis, SmartSwitchChassis
from sonic_platform.device_data import DeviceDataManager
from sonic_platform.module import Module
from sonic_platform_base.module_base import ModuleBase
from sonic_platform_base.chassis_base import ChassisBase



class TestModule:
    @classmethod
    def setup_class(cls):
        DeviceDataManager.get_linecard_sfp_count = mock.MagicMock(return_value=2)
        DeviceDataManager.get_linecard_count = mock.MagicMock(return_value=2)
        sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[])
        DeviceDataManager.get_dpu_count = mock.MagicMock(return_value=4)

    def test_chassis_get_num_sfp(self):
        chassis = ModularChassis()
        assert chassis.get_num_sfps() == 4

    def test_chassis_get_all_sfps(self):
        utils.read_int_from_file = mock.MagicMock(return_value=1)
        chassis = ModularChassis()
        assert len(chassis.get_all_sfps()) == 4

    @patch('sonic_platform.module.SonicV2Connector', mock.MagicMock())
    @patch('sonic_platform.module.ConfigDBConnector', mock.MagicMock())
    def test_chassis_get_num_modules(self):
        chassis = SmartSwitchChassis()
        assert chassis.get_num_modules() == 4

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_linecard_max_port_count', mock.MagicMock(return_value=16))
    def test_chassis_get_sfp(self):
        utils.read_int_from_file = mock.MagicMock(return_value=1)
        index = 1
        chassis = ModularChassis()
        sfp = chassis.get_sfp(index)
        assert sfp

    def test_thermal(self):
        from sonic_platform.thermal import THERMAL_NAMING_RULE
        DeviceDataManager.get_gearbox_count = mock.MagicMock(return_value=2)
        utils.read_int_from_file = mock.MagicMock(return_value=1)
        m = Module(1)
        assert m.get_num_thermals() == 2
        assert len(m._thermal_list) == 0

        thermals = m.get_all_thermals()
        assert len(thermals) == 2

        rule = THERMAL_NAMING_RULE['linecard thermals']
        start_index = rule.get('start_index', 1)
        for i, thermal in enumerate(thermals):
            assert rule['name'].format(i + start_index) in thermal.get_name()
            assert rule['temperature'].format(i + start_index) in thermal.temperature
            assert rule['high_threshold'].format(i + start_index) in thermal.high_threshold
            assert rule['high_critical_threshold'].format(i + start_index) in thermal.high_critical_threshold
            assert thermal.get_position_in_parent() == i + 1

        thermal = m.get_thermal(1)
        assert thermal
        assert thermal.get_position_in_parent() == 2

    def get_sfp(self):
        DeviceDataManager.get_linecard_sfp_count = mock.MagicMock(return_value=3)
        utils.read_int_from_file = mock.MagicMock(return_value=1)
    
        # Test get_num_sfps, it should not create any SFP objects
        m = Module(1)
        assert m.get_num_sfps() == 3
        assert len(m._sfp_list) == 0

        # Index out of bound, return None
        sfp = m.get_sfp(3)
        assert sfp is None
        assert len(m._sfp_list) == 0

        # Get one SFP, other SFP list should be initialized to None
        sfp = m.get_sfp(0)
        assert sfp is not None
        assert len(m._sfp_list) == 3
        assert m._sfp_list[1] is None
        assert m._sfp_list[2] is None
        assert m.sfp_initialized_count == 1

        # Get the SFP again, no new SFP created
        sfp1 = m.get_sfp(0)
        assert id(sfp) == id(sfp1)

        # Get another SFP, sfp_initialized_count increase
        sfp2 = m.get_sfp(1)
        assert sfp2 is not None
        assert m._sfp_list[2] is None
        assert m.sfp_initialized_count == 2

        # Get all SFPs, but there are SFP already created, only None SFP created
        sfp_list = m.get_all_sfps()
        assert len(sfp_list) == 3
        assert m.sfp_initialized_count == 3
        assert filter(lambda x: x is not None, sfp_list)
        assert id(sfp1) == id(sfp_list[0])
        assert id(sfp2) == id(sfp_list[1])

        # Get all SFPs, no SFP yet, all SFP created
        m._sfp_list = []
        m.sfp_initialized_count = 0
        sfp_list = m.get_all_sfps()
        assert len(sfp_list) == 3
        assert m.sfp_initialized_count == 3

    def test_check_state(self):
        utils.read_int_from_file = mock.MagicMock(return_value=0)
        m = Module(1)
        m._sfp_list.append(1)
        m._thermal_list.append(1)
        m._get_seq_no = mock.MagicMock(return_value=0)
        # both seq number and state no change, do not re-init module 
        m._check_state()
        assert len(m._sfp_list) > 0
        assert len(m._thermal_list) > 0

        # seq number changes, but state keeps deactivated, no need re-init module 
        m._get_seq_no = mock.MagicMock(return_value=1)
        m._check_state()
        assert len(m._sfp_list) > 0
        assert len(m._thermal_list) > 0

        # seq number not change, state changes from deactivated to activated, need re-init module
        utils.read_int_from_file = mock.MagicMock(return_value=1)
        m._check_state()
        assert len(m._sfp_list) == 0
        assert len(m._thermal_list) == 0

        # seq number changes, state keeps activated, which means the module has been replaced, need re-init module
        m._sfp_list.append(1)
        m._thermal_list.append(1)
        m._get_seq_no = mock.MagicMock(return_value=2)
        m._check_state()
        assert len(m._sfp_list) == 0
        assert len(m._thermal_list) == 0

        # seq number not change, state changes from activated to deactivated, need re-init module
        m._sfp_list.append(1)
        m._thermal_list.append(1)
        utils.read_int_from_file = mock.MagicMock(return_value=0)
        m._check_state()
        assert len(m._sfp_list) == 0
        assert len(m._thermal_list) == 0

    @patch('sonic_platform.module.SonicV2Connector', mock.MagicMock())
    @patch('sonic_platform.module.ConfigDBConnector', mock.MagicMock())
    def test_module_vpd(self):
        m = Module(1)
        m.vpd_parser.vpd_file = os.path.join(test_path, 'mock_psu_vpd')

        assert m.get_model() == 'MTEF-PSF-AC-C'
        assert m.get_serial() == 'MT1946X07684'
        assert m.get_revision() == 'A3'

        m.vpd_parser.vpd_file = 'not exists'
        assert m.get_model() == 'N/A'
        assert m.get_serial() == 'N/A'
        assert m.get_revision() == 'N/A'

        m.vpd_parser.vpd_file_last_mtime = None
        m.vpd_parser.vpd_file = os.path.join(test_path, 'mock_psu_vpd')
        assert m.get_model() == 'MTEF-PSF-AC-C'
        assert m.get_serial() == 'MT1946X07684'
        assert m.get_revision() == 'A3'

        from sonic_platform.module import DpuModule
        dm = DpuModule(2)
        dm.dpu_vpd_parser.vpd_file_last_mtime = None
        dm.dpu_vpd_parser.vpd_file = os.path.join(test_path, 'mock_psu_vpd_dpu')
        # dpu2 in pmon = DPU3 in eeprom (1 based indexing is used in HW)
        dpu_data = {
            "DPU3_SN": "MT4431X26022",
            "DPU3_PN": "SN4280BF3DPU2",
            "DPU3_REV": "A0",
            "DPU3_BASE_MAC": "90:0A:84:C6:00:B1"
        }
        dm.dpu_vpd_parser.vpd_data = dpu_data
        with patch.object(dm.dpu_vpd_parser, '_get_data', wraps=mock.MagicMock(return_value=True)):
            assert dm.get_base_mac() == "90:0A:84:C6:00:B1"
            assert dm.get_model() == "SN4280BF3DPU2"
            assert dm.get_serial() == "MT4431X26022"
            assert dm.get_revision() == "A0"

        dm.dpu_vpd_parser = None
        with pytest.raises(AttributeError):
            dm.get_base_mac()
            dm.get_model()
            dm.get_serial()
            dm.get_revision()

        dm = DpuModule(3)
        # DPU4 is not present in the VPD Parser output
        dm.dpu_vpd_parser.vpd_data = dpu_data
        assert dm.get_base_mac() == "N/A"
        assert dm.get_model() == "N/A"
        assert dm.get_serial() == "N/A"
        assert dm.get_revision() == "N/A"

    @patch('sonic_platform.module.SonicV2Connector', mock.MagicMock())
    @patch('swsscommon.swsscommon.ConfigDBConnector.connect', mock.MagicMock())
    @mock.patch('swsscommon.swsscommon.ConfigDBConnector.get')
    @mock.patch('subprocess.call')
    def test_dpu_module(self, mock_call, mock_get):
        from sonic_platform.module import DpuModule
        m = DpuModule(3)
        assert m.get_type() == ModuleBase.MODULE_TYPE_DPU
        assert m.get_name() == "DPU3"
        assert m.get_description() == "NVIDIA BlueField-3 DPU"
        assert m.get_dpu_id() == 3
        with patch.object(m.dpuctl_obj, "dpu_reboot") as mock_obj:
            mock_obj.return_value = True
            assert m.reboot() is True
            mock_obj.assert_called_once_with(skip_pre_post=True)
            mock_obj.reset_mock()
            m.reboot(reboot_type=ModuleBase.MODULE_REBOOT_SMARTSWITCH)
            mock_obj.assert_called_once_with(no_wait=True, skip_pre_post=True)
            with pytest.raises(RuntimeError):
                m.reboot("None")
        with patch('sonic_py_common.syslogger.SysLogger.log_error') as mock_method:
            m.dpuctl_obj.dpu_power_on = mock.MagicMock(return_value=True)
            assert m.set_admin_state(True)
            mock_method.assert_not_called()
            m.dpuctl_obj.dpu_power_on = mock.MagicMock(return_value=False)
            assert not m.set_admin_state(True)
            mock_method.assert_called_once_with("Failed to set the admin state for DPU3")
        m.dpuctl_obj.dpu_power_off = mock.MagicMock(return_value=True)
        assert m.set_admin_state(False)
        m1 = DpuModule(2)
        assert m.get_midplane_ip() == "169.254.200.4"
        assert m1.get_midplane_ip() == "169.254.200.3"
        with patch.object(m, '_is_midplane_up', ) as mock_midplane_m, \
             patch.object(m1, '_is_midplane_up',) as mock_midplane_m1:
            mock_midplane_m.return_value = True
            mock_midplane_m1.return_value = True
            command = ['ping', '-c', '1', '-W', '1', "169.254.200.4"]
            mock_call.return_value = 0
            assert m.is_midplane_reachable()
            mock_call.assert_called_with(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            mock_call.return_value = 1
            assert not m.is_midplane_reachable()
            mock_call.assert_called_with(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            mock_call.side_effect = subprocess.CalledProcessError(1, command)
            assert not m.is_midplane_reachable()
            assert not m1.is_midplane_reachable()
            mock_call.reset_mock()
            mock_call.side_effect = None
            mock_midplane_m.return_value = False
            mock_midplane_m1.return_value = False
            assert not m.is_midplane_reachable()
            assert not m1.is_midplane_reachable()
            mock_call.assert_not_called()

        m.fault_state = False
        test_file_path = ""
        pl_data = {
            "dpu0": {
                "interface": {"Ethernet224": "Ethernet0"},
                "midplane_interface": "dpu0_mid",
                "bus_info": "0000:08:0.0"
            },
            "dpu1": {
                "interface": {"Ethernet232": "Ethernet1"},
                "midplane_interface": "dpu1_mid",
                "bus_info": "0000:08:0.0"
            },
            "dpu2": {
                "interface": {"Ethernet236": "Ethernet2"},
                "midplane_interface": "dpu2_mid",
                "bus_info": "0000:08:0.0"
            },
        }
        DeviceDataManager.get_platform_dpus_data = mock.MagicMock(return_value=pl_data)

        def mock_read_int_from_file(file_path, default=0, raise_exception=False, log_func=None):
            if file_path.endswith(test_file_path):
                return 1
            else:
                return 0
        file_name_list = ['reset_aux_pwr_or_reload', 'reset_comex_pwr_fail', 'reset_from_main_board', 'reset_dpu_thermal', 'reset_pwr_off', 'None']
        reboot_cause_list = [
            (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'power auxiliary outage or reload'),
            (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'Power failed to comex module'),
            (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Reset from Main board'),
            (ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, 'Thermal shutdown of the DPU'),
            (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Reset due to Power off'),
            (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, ''),
        ]
        with patch("sonic_platform.utils.read_int_from_file", wraps=mock_read_int_from_file):
            for index, file_name in enumerate(file_name_list):
                test_file_path = file_name
                assert m.get_reboot_cause() == reboot_cause_list[index]
        m1 = DpuModule(0)
        m2 = DpuModule(1)
        m3 = DpuModule(2)
        m4 = DpuModule(3)
        # DPU0 in PMON = dpu1 in hw-mgmt
        m1.get_hw_mgmt_id() == 1
        m2.get_hw_mgmt_id() == 2
        m3.get_hw_mgmt_id() == 3
        m4.get_hw_mgmt_id() == 4
        assert not m1.midplane_interface
        with patch("sonic_platform.utils.read_int_from_file", wraps=mock.MagicMock(return_value=1)):
            assert m1._is_midplane_up()
            assert m2._is_midplane_up()
            assert m3._is_midplane_up()
            
            # Test get_pci_bus_info function
            with patch.object(m1.dpuctl_obj, "get_pci_dev_path") as mock_get_pci_dev_path:
                mock_get_pci_dev_path.return_value = ["0000:08:00.0", "0000:09:00.0"]
                # First call should get the bus info from dpuctl_obj
                assert m1.get_pci_bus_info() == ["0000:08:00.0", "0000:09:00.0"]
                mock_get_pci_dev_path.assert_called_once()
                # Second call should use cached value
                assert m1.get_pci_bus_info() == ["0000:08:00.0", "0000:09:00.0"]
                # Should not call get_pci_dev_path again
                mock_get_pci_dev_path.assert_called_once()

                # Test with a different module
                with patch.object(m2.dpuctl_obj, "get_pci_dev_path") as mock_get_pci_dev_path2:
                    mock_get_pci_dev_path2.return_value = ["0000:09:00.0", "0000:0A:00.0"]
                    assert m2.get_pci_bus_info() == ["0000:09:00.0", "0000:0A:00.0"]
                    mock_get_pci_dev_path2.assert_called_once()

            with pytest.raises(RuntimeError):
                m4._is_midplane_up()
                m4.get_pci_bus_info()
            assert m1.midplane_interface == "dpu0_mid"
            assert m2.midplane_interface == "dpu1_mid"
            assert m3.midplane_interface == "dpu2_mid"
        with patch("sonic_platform.utils.read_str_from_file", wraps=mock.MagicMock(return_value=0)):
            assert not m1._is_midplane_up()
            assert not m2._is_midplane_up()
            assert not m3._is_midplane_up()

        with patch.object(m1.dpuctl_obj, "read_boot_prog") as mock_obj:
            mock_obj.return_value = 5
            m1.get_oper_status() == ModuleBase.MODULE_STATUS_ONLINE
            mock_obj.return_value = 0
            m1.get_oper_status() == ModuleBase.MODULE_STATUS_OFFLINE
            mock_obj.return_value = 2
            m1.get_oper_status() == ModuleBase.MODULE_STATUS_OFFLINE
            mock_obj.return_value = 4
            m1.get_oper_status() == ModuleBase.MODULE_STATUS_OFFLINE

        temp_data = {
            f"TEMPERATURE_INFO_{m.get_dpu_id()}|DDR": {"temperature": "45.0", "high_threshold":"90", "critical_high_threshold": "100"},
            f"TEMPERATURE_INFO_{m.get_dpu_id()}|NVME": {"temperature": "100.0", "high_threshold":"85", "critical_high_threshold": "110"},
            f"TEMPERATURE_INFO_{m.get_dpu_id()}|CPU": {"temperature": "75.0", "high_threshold":"80", "critical_high_threshold": "95"}
        }
        def new_get_all(db_name, table_name):
            return temp_data[table_name]
        
        with patch.object(m.chassis_state_db, 'get_all', wraps=new_get_all):
            output_dict = m.get_temperature_dict()
            assert output_dict['DDR'] == temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|DDR"]
            assert output_dict['CPU'] == temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|CPU"]
            assert output_dict['NVME'] == temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|NVME"]
            temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|CPU"] = {}
            output_dict = m.get_temperature_dict()
            assert output_dict['DDR'] == temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|DDR"]
            assert output_dict['CPU'] == {}
            assert output_dict['NVME'] == temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|NVME"]
            del temp_data[f"TEMPERATURE_INFO_{m.get_dpu_id()}|CPU"]
            assert m.get_temperature_dict() == {}


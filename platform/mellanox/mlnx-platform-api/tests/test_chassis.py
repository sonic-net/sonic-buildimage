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
import random
import sys
import subprocess
import threading
import pytest

from mock import MagicMock
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

import sonic_platform.chassis
from sonic_platform_base.sfp_base import SfpBase
from sonic_platform.chassis import Chassis, SmartSwitchChassis
from sonic_platform.device_data import DeviceDataManager

sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[])
sonic_platform.chassis.extract_cpo_ports_index = mock.MagicMock(return_value=[])

class TestChassis:
    """Test class to test chassis.py. The test cases covers:
        1. PSU related API
        2. Fan drawer related API
        3. SFP related API (Except modular chassis SFP related API)
        4. Reboot cause related API

    Thermal, Eeprom, Watchdog, Component, System LED related API will be tested in seperate class
    """
    @classmethod
    def setup_class(cls):
        os.environ["MLNX_PLATFORM_API_UNIT_TESTING"] = "1"

    def test_psu(self):
        from sonic_platform.psu import Psu, FixedPsu
        # Test creating hot swapable PSU
        DeviceDataManager.get_psu_count = mock.MagicMock(return_value=2)
        DeviceDataManager.is_psu_hotswapable = mock.MagicMock(return_value=True)
        chassis = Chassis()
        chassis.initialize_psu()
        assert len(chassis._psu_list) == 2
        assert len(list(filter(lambda x: isinstance(x, Psu) ,chassis._psu_list))) == 2

        # Test creating fixed PSU
        DeviceDataManager.get_psu_count = mock.MagicMock(return_value=3)
        DeviceDataManager.is_psu_hotswapable = mock.MagicMock(return_value=False)
        chassis._psu_list = []
        chassis.initialize_psu()
        assert len(chassis._psu_list) == 3
        assert len(list(filter(lambda x: isinstance(x, FixedPsu) ,chassis._psu_list))) == 3

        # Test chassis.get_all_psus
        chassis._psu_list = []
        psu_list = chassis.get_all_psus()
        assert len(psu_list) == 3

        # Test chassis.get_psu
        chassis._psu_list = []
        psu = chassis.get_psu(0)
        assert psu and isinstance(psu, FixedPsu)
        psu = chassis.get_psu(3)
        assert psu is None

        # Test chassis.get_num_psus
        chassis._psu_list = []
        assert chassis.get_num_psus() == 3

    @mock.patch('sonic_platform.device_data.DeviceDataManager.get_fan_drawer_sysfs_count')
    def test_fan(self, mock_sysfs_count):
        from sonic_platform.fan_drawer import RealDrawer, VirtualDrawer

        # Test creating fixed fan
        mock_sysfs_count.return_value = 4
        DeviceDataManager.is_fan_hotswapable = mock.MagicMock(return_value=False)
        assert DeviceDataManager.get_fan_drawer_count() == 1
        DeviceDataManager.get_fan_count = mock.MagicMock(return_value=4)
        chassis = Chassis()
        chassis.initialize_fan()
        assert len(chassis._fan_drawer_list) == 1
        assert len(list(filter(lambda x: isinstance(x, VirtualDrawer) ,chassis._fan_drawer_list))) == 1
        assert chassis.get_fan_drawer(0).get_num_fans() == 4

        # Test creating hot swapable fan
        DeviceDataManager.get_fan_drawer_count = mock.MagicMock(return_value=2)
        DeviceDataManager.get_fan_count = mock.MagicMock(return_value=4)
        DeviceDataManager.is_fan_hotswapable = mock.MagicMock(return_value=True)
        chassis._fan_drawer_list = []
        chassis.initialize_fan()
        assert len(chassis._fan_drawer_list) == 2
        assert len(list(filter(lambda x: isinstance(x, RealDrawer) ,chassis._fan_drawer_list))) == 2
        assert chassis.get_fan_drawer(0).get_num_fans() == 2
        assert chassis.get_fan_drawer(1).get_num_fans() == 2

        # Test chassis.get_all_fan_drawers
        chassis._fan_drawer_list = []
        assert len(chassis.get_all_fan_drawers()) == 2

        # Test chassis.get_fan_drawer
        chassis._fan_drawer_list = []
        fan_drawer = chassis.get_fan_drawer(0)
        assert fan_drawer and isinstance(fan_drawer, RealDrawer)
        fan_drawer = chassis.get_fan_drawer(2)
        assert fan_drawer is None

        # Test chassis.get_num_fan_drawers
        chassis._fan_drawer_list = []
        assert chassis.get_num_fan_drawers() == 2

    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=False))
    def test_sfp(self):
        # Test get_num_sfps, it should not create any SFP objects
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=3)
        chassis = Chassis()
        assert chassis.get_num_sfps() == 3
        assert len(chassis._sfp_list) == 0

        # Index out of bound, return None
        sfp = chassis.get_sfp(4)
        assert sfp is None
        assert len(chassis._sfp_list) == 0

        # Get one SFP, other SFP list should be initialized to None
        sfp = chassis.get_sfp(1)
        assert sfp is not None
        assert len(chassis._sfp_list) == 3
        assert chassis._sfp_list[1] is None
        assert chassis._sfp_list[2] is None
        assert chassis.sfp_initialized_count == 1

        # Get the SFP again, no new SFP created
        sfp1 = chassis.get_sfp(1)
        assert id(sfp) == id(sfp1)

        # Get another SFP, sfp_initialized_count increase
        sfp2 = chassis.get_sfp(2)
        assert sfp2 is not None
        assert chassis._sfp_list[2] is None
        assert chassis.sfp_initialized_count == 2

        # Get all SFPs, but there are SFP already created, only None SFP created
        sfp_list = chassis.get_all_sfps()
        assert len(sfp_list) == 3
        assert chassis.sfp_initialized_count == 3
        assert list(filter(lambda x: x is not None, sfp_list))
        assert id(sfp1) == id(sfp_list[0])
        assert id(sfp2) == id(sfp_list[1])

        # Get all SFPs, no SFP yet, all SFP created
        chassis._sfp_list = []
        chassis.sfp_initialized_count = 0
        sfp_list = chassis.get_all_sfps()
        assert len(sfp_list) == 3
        assert chassis.sfp_initialized_count == 3

        # Get all SFPs, with RJ45 ports
        sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[0,1,2])
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=3)
        chassis = Chassis()
        assert chassis.get_num_sfps() == 6
        sonic_platform.chassis.extract_RJ45_ports_index = mock.MagicMock(return_value=[])

        # Get all SFPs, with CPO ports
        sonic_platform.chassis.extract_cpo_ports_index = mock.MagicMock(return_value=[3, 4])
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=3)
        chassis = Chassis()
        assert chassis.get_num_sfps() == 5
        sonic_platform.chassis.extract_cpo_ports_index = mock.MagicMock(return_value=[])

    @mock.patch('sonic_platform.device_data.DeviceDataManager.is_module_host_management_mode', mock.MagicMock(return_value=False))
    def test_create_sfp_in_multi_thread(self):
        DeviceDataManager.get_sfp_count = mock.MagicMock(return_value=3)

        iteration_num = 100
        while iteration_num > 0:
            chassis = Chassis()
            assert chassis.sfp_initialized_count == 0
            t1 = threading.Thread(target=lambda: chassis.get_sfp(1))
            t2 = threading.Thread(target=lambda: chassis.get_sfp(1))
            t3 = threading.Thread(target=lambda: chassis.get_all_sfps())
            t4 = threading.Thread(target=lambda: chassis.get_all_sfps())
            threads = [t1, t2, t3, t4]
            random.shuffle(threads)
            for t in threads:
                t.start()
            for t in threads:
                t.join()
            assert len(chassis.get_all_sfps()) == 3
            assert chassis.sfp_initialized_count == 3
            for index, s in enumerate(chassis.get_all_sfps()):
                assert s.sdk_index == index
            iteration_num -= 1

    @mock.patch('sonic_platform.chassis.Chassis._wait_reboot_cause_ready', MagicMock(return_value=True))
    def test_reboot_cause(self):
        from sonic_platform import utils
        from sonic_platform.chassis import REBOOT_CAUSE_ROOT
        chassis = Chassis()
        major, minor = chassis.get_reboot_cause()
        assert major == chassis.REBOOT_CAUSE_NON_HARDWARE
        assert minor == ''

        mock_file_content = {}
        def read_int_from_file(file_path, *args, **kwargs):
            return mock_file_content[file_path]

        utils.read_int_from_file = read_int_from_file

        for key, value in chassis.reboot_major_cause_dict.items():
            file_path = os.path.join(REBOOT_CAUSE_ROOT, key)
            mock_file_content[file_path] = 1
            major, minor = chassis.get_reboot_cause()
            assert major == value
            assert minor == ''
            mock_file_content[file_path] = 0

        for key, value in chassis.reboot_minor_cause_dict.items():
            file_path = os.path.join(REBOOT_CAUSE_ROOT, key)
            mock_file_content[file_path] = 1
            major, minor = chassis.get_reboot_cause()
            assert major == chassis.REBOOT_CAUSE_HARDWARE_OTHER
            assert minor == value
            mock_file_content[file_path] = 0

        utils.is_host = mock.MagicMock(return_value=True)
        chassis._parse_warmfast_reboot_from_proc_cmdline = mock.MagicMock(return_value='warm-reboot')
        for key, value in chassis.reboot_major_cause_dict.items():
            file_path = os.path.join(REBOOT_CAUSE_ROOT, key)
            mock_file_content[file_path] = 1
            major, minor = chassis.get_reboot_cause()
            assert major == chassis.REBOOT_CAUSE_NON_HARDWARE
            assert minor == ''
            mock_file_content[file_path] = 0

        for key, value in chassis.reboot_minor_cause_dict.items():
            file_path = os.path.join(REBOOT_CAUSE_ROOT, key)
            mock_file_content[file_path] = 1
            major, minor = chassis.get_reboot_cause()
            assert major == chassis.REBOOT_CAUSE_NON_HARDWARE
            assert minor == value
            mock_file_content[file_path] = 0

    @mock.patch('sonic_platform.chassis.Chassis._wait_reboot_cause_ready', MagicMock(return_value=False))
    def test_reboot_cause_timeout(self):
        chassis = Chassis()
        major, minor = chassis.get_reboot_cause()
        assert major == chassis.REBOOT_CAUSE_NON_HARDWARE
        assert minor == ''

    @mock.patch('sonic_platform.utils.read_int_from_file')
    @mock.patch('sonic_platform.chassis.time.sleep', mock.MagicMock())
    def test_wait_reboot_cause_ready(self, mock_read_int):
        mock_read_int.return_value = 1
        chassis = Chassis()
        assert chassis._wait_reboot_cause_ready()
        mock_read_int.return_value = 0
        assert not chassis._wait_reboot_cause_ready()

    def test_parse_warmfast_reboot_from_proc_cmdline(self):
        chassis = Chassis()
        with mock.patch("builtins.open", mock.mock_open(read_data="SONIC_BOOT_TYPE=warm")):
            assert chassis._parse_warmfast_reboot_from_proc_cmdline() == "warm-reboot"

        with mock.patch("builtins.open", mock.mock_open(read_data="SONIC_BOOT_TYPE=fast")):
            assert chassis._parse_warmfast_reboot_from_proc_cmdline() == "fast-reboot"

        with mock.patch("builtins.open", mock.mock_open(read_data="SONIC_BOOT_TYPE=None")):
            assert chassis._parse_warmfast_reboot_from_proc_cmdline() == None

    def test_module(self):
        from sonic_platform.chassis import ModularChassis
        # Test get_num_modules, it should not create any SFP objects
        DeviceDataManager.get_linecard_count = mock.MagicMock(return_value=3)
        chassis = ModularChassis()
        assert chassis.is_modular_chassis()
        assert chassis.get_num_modules() == 3
        assert len(chassis._module_list) == 0

        # Index out of bound, return None
        m = chassis.get_module(3)
        assert m is None
        assert len(chassis._module_list) == 0

        # Get one Module, other Module in list should be initialized to None
        m = chassis.get_module(0)
        assert m is not None
        assert len(chassis._module_list) == 3
        assert chassis._module_list[1] is None
        assert chassis._module_list[2] is None
        assert chassis.module_initialized_count == 1

        # Get the Module again, no new Module created
        m1 = chassis.get_module(0)
        assert id(m) == id(m1)

        # Get another Module, module_initialized_count increase
        m2 = chassis.get_module(1)
        assert m2 is not None
        assert chassis._module_list[2] is None
        assert chassis.module_initialized_count == 2

        # Get all SFPs, but there are SFP already created, only None SFP created
        module_list = chassis.get_all_modules()
        assert len(module_list) == 3
        assert chassis.module_initialized_count == 3
        assert list(filter(lambda x: x is not None, module_list))
        assert id(m1) == id(module_list[0])
        assert id(m2) == id(module_list[1])

        # Get all SFPs, no SFP yet, all SFP created
        chassis._module_list = []
        chassis.module_initialized_count = 0
        module_list = chassis.get_all_modules()
        assert len(module_list) == 3
        assert chassis.module_initialized_count == 3

    def test_get_port_or_cage_type(self):
        chassis = Chassis()
        chassis._RJ45_port_inited = True
        chassis._RJ45_port_list = [0]
        assert SfpBase.SFP_PORT_TYPE_BIT_RJ45 == chassis.get_port_or_cage_type(1)

        exceptionRaised = False
        try:
            chassis.get_port_or_cage_type(2)
        except NotImplementedError:
            exceptionRaised = True

        assert exceptionRaised

    def test_parse_vpd(self):
        chassis = Chassis()
        content = chassis._parse_vpd_data(os.path.join(test_path, 'vpd_data_file'))
        assert content.get('REV') == 'A7'

    @mock.patch('sonic_platform.module.SonicV2Connector', mock.MagicMock())
    @mock.patch('sonic_platform.module.ConfigDBConnector', mock.MagicMock())
    def test_smartswitch(self):
        orig_dpu_count = DeviceDataManager.get_dpu_count
        DeviceDataManager.get_dpu_count = mock.MagicMock(return_value=4)
        chassis = SmartSwitchChassis()

        assert not chassis.is_modular_chassis()
        assert chassis.is_smartswitch()
        assert chassis.init_midplane_switch()

        chassis._module_list = None
        chassis.module_initialized_count = 0
        chassis.module_name_index_map = {}
        with pytest.raises(RuntimeError, match="Invalid index = -1 for module"
                           " initialization with total module count = 4"):
            chassis.initialize_single_module(-1)
            chassis.get_module(-1)
        with pytest.raises(KeyError):
            chassis.get_module_index('DPU1')
            chassis.get_module_index('DPU2')
            chassis.get_dpu_id("DPU1")
            chassis.get_dpu_id("DPU2")
            chassis.get_dpu_id("DPU3")

        DeviceDataManager.get_dpu_count = mock.MagicMock(return_value=0)
        assert chassis.get_num_modules() == 0
        with pytest.raises(TypeError):
            chassis.get_module(0)
        chassis.initialize_modules()
        assert chassis.get_all_modules() is None

        DeviceDataManager.get_dpu_count = mock.MagicMock(return_value=4)
        from sonic_platform.module import DpuModule
        assert isinstance(chassis.get_module(0), DpuModule)
        assert chassis.get_module(4) is None

        chassis.initialize_modules()
        assert chassis.get_module_index('DPU0') == 0
        assert chassis.get_module_index('DPU3') == 3
        with pytest.raises(KeyError):
            chassis.get_module_index('DPU10')
            chassis.get_module_index('ABC')

        assert chassis.get_num_modules() == 4
        module_list = chassis.get_all_modules()
        assert len(module_list) == 4
        pl_data = {
            "dpu0": {
                "interface": {"Ethernet224": "Ethernet0"}
            },
            "dpu1": {
                "interface": {"Ethernet232": "Ethernet0"}
            },
            "dpu2": {
                "interface": {"EthernetX": "EthernetY"}
            }
        }
        orig_dpus_data = DeviceDataManager.get_platform_dpus_data
        DeviceDataManager.get_platform_dpus_data = mock.MagicMock(return_value=pl_data)
        chassis.get_module_dpu_data_port(0) == str({"Ethernet232": "Ethernet0"})
        with pytest.raises(IndexError):
            assert chassis.get_module_dpu_data_port(5)
            assert chassis.get_module_dpu_data_port(-1)

        assert chassis.get_dpu_id("DPU1") == 1
        assert chassis.get_dpu_id("DPU3") == 3
        assert chassis.get_dpu_id("DPU2") == 2
        with pytest.raises(KeyError):
            chassis.get_dpu_id('DPU15')
            chassis.get_dpu_id('ABC')
        DeviceDataManager.get_platform_dpus_data = orig_dpus_data
        DeviceDataManager.get_dpu_count = orig_dpu_count

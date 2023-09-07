#
# Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES.
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

import glob
import os

from . import utils

DEVICE_DATA = {
    'x86_64-mlnx_msn2700-r0': {
        'thermal': {
            "capability": {
                "comex_amb": False
            }
        }
    },
    'x86_64-mlnx_msn2740-r0': {
        'thermal': {
            "capability": {
                "cpu_pack": False,
                "comex_amb": False
            }
        }
    },
    'x86_64-mlnx_msn2100-r0': {
        'thermal': {
            "capability": {
                "cpu_pack": False,
                "comex_amb": False
            }
        }
    },
    'x86_64-mlnx_msn2410-r0': {
        'thermal': {
            "capability": {
                "comex_amb": False
            }
        }
    },
    'x86_64-mlnx_msn2010-r0': {
        'thermal': {
            "capability": {
                "cpu_pack": False,
                "comex_amb": False
            }
        }
    },
    'x86_64-mlnx_msn3700-r0': {
    },
    'x86_64-mlnx_msn3700c-r0': {
    },
    'x86_64-mlnx_msn3800-r0': {
    },
    'x86_64-mlnx_msn4700-r0': {
    },
    'x86_64-mlnx_msn4410-r0': {
    },
    'x86_64-mlnx_msn3420-r0': {
    },
    'x86_64-mlnx_msn4600c-r0': {
    },
    'x86_64-mlnx_msn4600-r0': {
    },
    'x86_64-nvidia_sn4800-r0': {
        'thermal': {
            "capability": {
                "comex_amb": False
            }
        },
        'sfp': {
            'max_port_per_line_card': 16
        }
    },
    'x86_64-nvidia_sn2201-r0': {
        'thermal': {
            "capability": {
                "comex_amb": False,
                "cpu_amb": True
            }
        }
    },
    'x86_64-nvidia_sn5600-r0': {
        'thermal': {
            "capability": {
                "comex_amb": False,
                "pch_temp": True
            }
        }
    }
}


class DeviceDataManager:
    @classmethod
    @utils.read_only_cache()
    def get_platform_name(cls):
        from sonic_py_common import device_info
        return device_info.get_platform()

    @classmethod
    @utils.read_only_cache()
    def is_simx_platform(cls):
        platform_name = cls.get_platform_name()
        return platform_name and 'simx' in platform_name

    @classmethod
    @utils.read_only_cache()
    def get_fan_drawer_count(cls):
        # Here we don't read from /run/hw-management/config/hotplug_fans because the value in it is not
        # always correct.
        return len(glob.glob('/run/hw-management/thermal/fan*_status')) if cls.is_fan_hotswapable() else 1

    @classmethod
    @utils.read_only_cache()
    def get_fan_count(cls):
        return len(glob.glob('/run/hw-management/thermal/fan*_speed_get'))

    @classmethod
    @utils.read_only_cache()
    def is_fan_hotswapable(cls):
        return utils.read_int_from_file('/run/hw-management/config/hotplug_fans') > 0

    @classmethod
    @utils.read_only_cache()
    def get_psu_count(cls):
        psu_count = utils.read_int_from_file('/run/hw-management/config/hotplug_psus')
        # If psu_count == 0, the platform has fixed PSU
        return psu_count if psu_count > 0 else len(glob.glob('/run/hw-management/config/psu*_i2c_addr'))

    @classmethod
    @utils.read_only_cache()
    def is_psu_hotswapable(cls):
        return utils.read_int_from_file('/run/hw-management/config/hotplug_psus') > 0

    @classmethod
    @utils.read_only_cache()
    def get_sfp_count(cls):
        return utils.read_int_from_file('/run/hw-management/config/sfp_counter')

    @classmethod
    def get_linecard_sfp_count(cls, lc_index):
        return utils.read_int_from_file('/run/hw-management/lc{}/config/module_counter'.format(lc_index), log_func=None)

    @classmethod
    def get_gearbox_count(cls, sysfs_folder):
        return utils.read_int_from_file(os.path.join(sysfs_folder, 'gearbox_counter'), log_func=None)

    @classmethod
    @utils.read_only_cache()
    def get_cpu_thermal_count(cls):
        return len(glob.glob('run/hw-management/thermal/cpu_core[!_]'))

    @classmethod
    @utils.read_only_cache()
    def get_sodimm_thermal_count(cls):
        return len(glob.glob('/run/hw-management/thermal/sodimm*_temp_input'))

    @classmethod
    @utils.read_only_cache()
    def get_thermal_capability(cls):
        platform_data = DEVICE_DATA.get(cls.get_platform_name(), None)
        if not platform_data:
            return None

        thermal_data = platform_data.get('thermal', None)
        if not thermal_data:
            return None

        return thermal_data.get('capability', None)

    @classmethod
    @utils.read_only_cache()
    def get_linecard_count(cls):
        return utils.read_int_from_file('/run/hw-management/config/hotplug_linecards', log_func=None)

    @classmethod
    @utils.read_only_cache()
    def get_linecard_max_port_count(cls):
        platform_data = DEVICE_DATA.get(cls.get_platform_name(), None)
        if not platform_data:
            return 0

        sfp_data = platform_data.get('sfp', None)
        if not sfp_data:
            return 0
        return sfp_data.get('max_port_per_line_card', 0)

    @classmethod
    def get_bios_component(cls):
        from .component import ComponentBIOS, ComponentBIOSSN2201
        if cls.get_platform_name() in ['x86_64-nvidia_sn2201-r0']:
            # For SN2201, special chass is required for handle BIOS
            # Currently, only fetching BIOS version is supported
            return ComponentBIOSSN2201()
        return ComponentBIOS()

    @classmethod
    def get_cpld_component_list(cls):
        from .component import ComponentCPLD, ComponentCPLDSN2201
        if cls.get_platform_name() in ['x86_64-nvidia_sn2201-r0']:
            # For SN2201, special chass is required for handle BIOS
            # Currently, only fetching BIOS version is supported
            return ComponentCPLDSN2201.get_component_list()
        return ComponentCPLD.get_component_list()

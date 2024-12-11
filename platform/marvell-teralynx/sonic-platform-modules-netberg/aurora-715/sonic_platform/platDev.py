#!/usr/bin/env python

try:
    import os
    import copy
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger("platDev")
PLATFORM_NAME = "aurora-715"
MAX_FAN_MODULE = 5
MAX_FAN = 2

THERMAL_SENSOR_LIST = [
    {
        'name': "pch_haswell",
        'temp_index': [1],
        'sysfile_path': "/sys/class/hwmon/hwmon0/",
        'support_mask': 0x81,
        'ext_sysfile_list': None
    },
    {
        'name': "CPU core temp",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon1/",
        'support_mask': 0x8B,
        'ext_sysfile_list': None
    },
    {
        'name': "NCT7511Y(U73)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA715_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_r_b_f', 'temp_r_b_f_max', 'temp_r_b_f_min', 'temp_r_b_f_crit', 'temp_r_b_f_lcrit'],
                             2:['temp_r_b_b', 'temp_r_b_b_max', 'temp_r_b_b_min', 'temp_r_b_b_crit', 'temp_r_b_b_lcrit']}
    },
    {
        'name': "G781(U94)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA715_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_l_b_f', 'temp_l_b_f_max', 'temp_l_b_f_min', 'temp_l_b_f_crit', 'temp_l_b_f_lcrit'],
                             2:['temp_l_b_b', 'temp_l_b_b_max', 'temp_l_b_b_min', 'temp_l_b_b_crit', 'temp_l_b_b_lcrit']}
    },
    {
        'name': "G781(U34)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA715_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_r_t_f', 'temp_r_t_f_max', 'temp_r_t_f_min', 'temp_r_t_f_crit', 'temp_r_t_f_lcrit'],
                             2:['temp_r_t_b', 'temp_r_t_b_max', 'temp_r_t_b_min', 'temp_r_t_b_crit', 'temp_r_t_b_lcrit']}
    },
    {
        'name': "G781(U4)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA715_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_l_t_f', 'temp_l_t_f_max', 'temp_l_t_f_min', 'temp_l_t_f_crit', 'temp_l_t_f_lcrit'],
                             2:['temp_l_t_b', 'temp_l_t_b_max', 'temp_l_t_b_min', 'temp_l_t_b_crit', 'temp_l_t_b_lcrit']}
    }
]

# PSU LIST
# ext_sysfile_list
#  [0] : sysfile path for present
#  [1] : sysfile path for status
#
PSU_LIST = ['PSU1', 'PSU2']

PSU_INFO = {
    'PSU1': {
        'attr_path': "/sys/class/hwmon/hwmon2/device/NBA715_POWER/",
        'status_path': "/sys/class/hwmon/hwmon2/device/NBA715_POWER/"
    },
    'PSU2': {
        'attr_path': "/sys/class/hwmon/hwmon2/device/NBA715_POWER/",
        'status_path': "/sys/class/hwmon/hwmon2/device/NBA715_POWER/"
    }
}

FAN_LIST = ['FAN1', 'FAN2', 'FAN3', 'FAN4', 'FAN5']

FAN_INFO = {
    'FAN1': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA715_FAN/'
    },
    'FAN2': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA715_FAN/'
    },
    'FAN3': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA715_FAN/'
    },
    'FAN4': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA715_FAN/'
    },
    'FAN5': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA715_FAN/'
    }
}


# SFP LIST
#
#
# 0: QSFP
# 1: SFP
# sfp not port on 715 platform

SFP_EXT_SYSFILE_LIST = ["/sys/class/hwmon/hwmon2/device/NBA715_QSFP/", ""]

SFP_GROUP_LIST = ['SFP-G01', 'SFP-G02', 'SFP-G03', 'SFP-G04']

PORT_NUM = 32

# SFP-eeprom paths /sys/bus/i2c/devices/XX-0050

I2C_DEVICES = {
    # NCT7511Y sensor & fan control
    'NCT7511Y(U73)': {
        'parent': 'viaBMC',
        'parent_ch': 0,
        'driver': 'nct7511',
        'i2caddr': '0x2e',
        'path': ' ',
        'status': 'NOTINST'
    },
    # G781 sensors
    'G781(U94)': {
        'parent': 'viaBMC',
        'parent_ch': 1,
        'driver': 'g781',
        'i2caddr': '0x4c',
        'path': ' ',
        'status': 'NOTINST'
    },
    'G781(U4)': {
        'parent': 'viaBMC',
        'parent_ch': 2,
        'driver': 'g781',
        'i2caddr': '0x4c',
        'path': ' ',
        'status': 'NOTINST'
    },
    'G781(U34)': {
        'parent': 'viaBMC',
        'parent_ch': 3,
        'driver': 'g781',
        'i2caddr': '0x4c',
        'path': ' ',
        'status': 'NOTINST'
    },
    # PSU
    'PSU1': {
        'parent': 'viaBMC',
        'parent_ch': 4,
        'driver': 'zrh2800k2',
        'i2caddr': '0x58',
        'path': ' ',
        'status': 'NOTINST'
    },
    'PSU2': {
        'parent': 'viaBMC',
        'parent_ch': 4,
        'driver': 'zrh2800k2',
        'i2caddr': '0x59',
        'path': ' ',
        'status': 'NOTINST'
    },
    'TPS53681(0x6C)': {
        'parent': 'viaBMC',
        'parent_ch': 5,
        'driver': 'tps53679',
        'i2caddr': '0x6c',
        'path': ' ',
        'status': 'NOTINST'
    },
    'TPS53681(0x6E)': {
        'parent': 'viaBMC',
        'parent_ch': 5,
        'driver': 'tps53679',
        'i2caddr': '0x6e',
        'path': ' ',
        'status': 'NOTINST'
    },
    'TPS53681(0x70)': {
        'parent': 'viaBMC',
        'parent_ch': 5,
        'driver': 'tps53679',
        'i2caddr': '0x70',
        'path': ' ',
        'status': 'NOTINST'
    }
}


SFP_GROUPS = {
    'SFP-G01': {
        "type": "QSFP28",
        'number': 8,
        'parent': 'PCA9548_0x71_1',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe1',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/9-0050", "/sys/bus/i2c/devices/10-0050",
                  "/sys/bus/i2c/devices/11-0050", "/sys/bus/i2c/devices/12-0050",
                  "/sys/bus/i2c/devices/13-0050", "/sys/bus/i2c/devices/14-0050",
                  "/sys/bus/i2c/devices/15-0050", "/sys/bus/i2c/devices/16-0050"],
        'status': 'NOTINST'
    },
    'SFP-G02': {
        "type": "QSFP28",
        'number': 8,
        'parent': 'PCA9548_0x71_2',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe1',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/17-0050", "/sys/bus/i2c/devices/18-0050",
                  "/sys/bus/i2c/devices/19-0050", "/sys/bus/i2c/devices/20-0050",
                  "/sys/bus/i2c/devices/21-0050", "/sys/bus/i2c/devices/22-0050",
                  "/sys/bus/i2c/devices/23-0050", "/sys/bus/i2c/devices/24-0050"],
        'status': 'NOTINST'
    },
    'SFP-G03': {
        "type": "QSFP28",
        'number': 8,
        'parent': 'PCA9548_0x71_3',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe1',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/25-0050", "/sys/bus/i2c/devices/26-0050",
                  "/sys/bus/i2c/devices/27-0050", "/sys/bus/i2c/devices/28-0050",
                  "/sys/bus/i2c/devices/29-0050", "/sys/bus/i2c/devices/30-0050",
                  "/sys/bus/i2c/devices/31-0050", "/sys/bus/i2c/devices/32-0050"],
        'status': 'NOTINST'
    },
    'SFP-G04': {
        "type": "QSFP28",
        'number': 8,
        'parent': 'PCA9548_0x71_4',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe1',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/33-0050", "/sys/bus/i2c/devices/34-0050",
                  "/sys/bus/i2c/devices/35-0050", "/sys/bus/i2c/devices/36-0050",
                  "/sys/bus/i2c/devices/37-0050", "/sys/bus/i2c/devices/38-0050",
                  "/sys/bus/i2c/devices/39-0050", "/sys/bus/i2c/devices/40-0050"],
        'status': 'NOTINST'
    }
}

# Component
# ["Master-CPLD", ("Used for managing Fan, PSU, system LEDs, QSFP "
#                  "modules (1-16)")],
# ["Slave-CPLD", "Used for managing QSFP modules (17-32)"]

CHASSIS_COMPONENTS = [
    ["BIOS", ("Performs initialization of hardware components during "
              "booting")],
    ["System-CPLD", "Used for managing CPU board devices and power"]
]


class PlatDev():
    def __init__(self):
        self.plat_name = PLATFORM_NAME
        self.psu_info = copy.deepcopy(PSU_INFO)
        self.thermal_info = []
        self.fan_info = copy.deepcopy(FAN_INFO)
        self.sfp_info = copy.deepcopy(SFP_GROUPS)

        # get install info
        # self.sfp_install_info = SFP_GROUPS
        self.device_install_info = I2C_DEVICES

        # update path info with install info
        # Item 1/2 not changed, append directly
        self.thermal_info.append(THERMAL_SENSOR_LIST[0])
        self.thermal_info.append(THERMAL_SENSOR_LIST[1])

    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath, 'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except FileNotFoundError:
            logger.log_error(f"File {filepath} not found.  Aborting")
        except OSError as ex:
            logger.log_error("Cannot open - {}: {}".format(filepath, repr(ex)))

        return None

    def bmc_is_exist(self):
        bmc_filePath = '/sys/class/hwmon/hwmon2/device/NBA715_SYS/bmc_present'
        if os.path.exists(bmc_filePath):
            value = self.__read_attr_file(bmc_filePath)
            if int(value) == 1:
                return True
            else:
                return False
        else:
            return False
    ######Componet method #####

    def get_component_count(self):
        return len(CHASSIS_COMPONENTS)

    def get_component_name(self, idx):
        return CHASSIS_COMPONENTS[idx][0]

    def get_component_descript(self, idx):
        return CHASSIS_COMPONENTS[idx][1]

    ###### PSU method ######
    def get_psu_list(self):
        return PSU_LIST

    def get_psu_info_all(self):
        return self.psu_info

    def get_psu_info_by_name(self, name):
        return self.psu_info.get(name)

    def get_psu_attr_path_by_name(self, name):
        return self.psu_info[name].get('attr_path')

    def get_psu_status_path_by_name(self, name):
        return self.psu_info[name].get('status_path')

    ###### Thermal method ######
    def get_thermal_dev_info_all(self):
        return self.thermal_info

    def get_thermal_dev_name_by_idx(self, index):
        return self.thermal_info[index].get('name')

    def get_thermal_dev_tempidx_by_idx(self, index):
        return self.thermal_info[index].get('temp_index')

    def get_thermal_dev_sysfile_path_by_idx(self, index):
        return self.thermal_info[index].get('sysfile_path')

    def get_thermal_dev_support_mask_by_idx(self, index):
        return self.thermal_info[index].get('support_mask')

    def get_thermal_dev_ext_sysfile_list_by_idx(self, index):
        return self.thermal_info[index].get('ext_sysfile_list')

    ###### Fan method ######
    def get_fan_support(self):
        return True

    def get_fan_list(self):
        return FAN_LIST

    def get_fan_info_all(self):
        return self.fan_info

    def get_fan_info_by_name(self, name):
        return self.fan_info.get(name)

    def get_fan_sysfile_path_by_name(self, name):
        return self.fan_info[name].get('attr_path')

    def get_fan_is_draw_by_name(self, name):
        return self.fan_info[name].get('isdraw')

    def get_fan_num_by_name(self, name):
        return self.fan_info[name].get('fan_num')

    ###### SFP method ######
    def get_sfp_num(self):
        return PORT_NUM

    def get_sfp_group_list(self):
        return SFP_GROUP_LIST

    def get_sfp_group_info(self):
        return self.sfp_info

    def get_sfp_group_info_by_name(self, name):
        return self.sfp_info.get(name)

    def get_sfp_group_type_by_name(self, name):
        return self.sfp_info[name].get('type')

    def get_sfp_group_path_by_name(self, name):
        return self.sfp_info[name].get('paths')

    def get_sfp_group_number_by_name(self, name):
        return self.sfp_info[name].get('number')

    def get_sfp_ext_sysfile_list(self):
        return SFP_EXT_SYSFILE_LIST

#!/usr/bin/env python

try:
    import copy
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger("platDev")
MAX_FAN_MODULE = 5
MAX_FAN = 2

# THERMAL_SENSOR_LIST
# index is used to indicate the default temp{}_* under sysfile_path
# support_mask:  1:support  0:not support
#   bit 0 : temperature (always 1)
#   bit 1 : high threshold
#   bit 2 : low threshold
#   bit 3 : high critical threshold
#   bit 4 : low critical threshold
#   bit 7 :  cpu internal sensor
# ext_sysfile_list: each specified path of each supported function,
#   which not follows the (default) genernal naming rule
#   [0] ext_temp_file : temperature
#   [1] ext_high_thr : high threshold
#   [2] ext_low_thr : low threshold
#   [3] ext_high_cri_thr : high critical threshold
#   [4] ext_low_cri_thr : low critical threshold

# ['Sensor 1    Top', 'temp_th0_t', 'temp_th0_t_max', 'temp_th0_t_min', 'temp_th0_t_crit', 'temp_th0_t_lcrit'],
# ['Sensor 1 Bottom', 'temp_th0_b', 'temp_th0_b_max', 'temp_th0_b_min', 'temp_th0_b_crit', 'temp_th0_b_lcrit'],
# ['Sensor 1 Remote', 'temp_th0_r', 'temp_th0_r_max', 'temp_th0_r_min', 'temp_th0_r_crit', 'temp_th0_r_lcrit'],
# ['Sensor 2    Top', 'temp_th1_t', 'temp_th1_t_max', 'temp_th1_t_min', 'temp_th1_t_crit', 'temp_th1_t_lcrit'],
# ['Sensor 2 Bottom', 'temp_th1_b', 'temp_th1_b_max', 'temp_th1_b_min', 'temp_th1_b_crit', 'temp_th1_b_lcrit'],
# ['Sensor 3    Top', 'temp_th2_t', 'temp_th2_t_max', 'temp_th2_t_min', 'temp_th2_t_crit', 'temp_th2_t_lcrit'],
# ['Sensor 3 Bottom', 'temp_th2_b', 'temp_th2_b_max', 'temp_th2_b_min', 'temp_th2_b_crit', 'temp_th2_b_lcrit'],
# ['Sensor 4    Top', 'temp_th3_t', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit'],
# ['Sensor 4 Bottom', 'temp_th3_b', 'temp_th3_b_max', 'temp_th3_b_min', 'temp_th3_b_crit', 'temp_th3_b_lcrit'],

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
        'name': "FrontRight",
        'temp_index': [1, 2, 4],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA615_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_th0_t', 'temp_th0_t_max', 'temp_th0_t_min', 'temp_th0_t_crit', 'temp_th0_t_lcrit'],
                             2:['temp_th0_b', 'temp_th0_b_max', 'temp_th0_b_min', 'temp_th0_b_crit', 'temp_th0_b_lcrit'],
                             4:['temp_th0_r', 'temp_th0_r_max', 'temp_th0_r_min', 'temp_th0_r_crit', 'temp_th0_r_lcrit']}
    },
    {
        'name': "FrontLeft",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA615_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_th1_t', 'temp_th1_t_max', 'temp_th1_t_min', 'temp_th1_t_crit', 'temp_th1_t_lcrit'],
                             2:['temp_th1_b', 'temp_th1_b_max', 'temp_th1_b_min', 'temp_th1_b_crit', 'temp_th1_b_lcrit']}
    },
    {
        'name': "Back",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA615_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_th2_t', 'temp_th2_t_max', 'temp_th2_t_min', 'temp_th2_t_crit', 'temp_th2_t_lcrit'],
                             2:['temp_th2_b', 'temp_th2_b_max', 'temp_th2_b_min', 'temp_th2_b_crit', 'temp_th2_b_lcrit']}
    },
    {
        'name': "ASIC",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon2/device/NBA615_THERMAL/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1: ['temp_th3_t', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit'],
                             2:['temp_th3_b', 'temp_th3_b_max', 'temp_th3_b_min', 'temp_th3_b_crit', 'temp_th3_b_lcrit']}
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
        'attr_path': "/sys/class/hwmon/hwmon2/device/NBA615_POWER/",
        'status_path': "/sys/class/hwmon/hwmon2/device/NBA615_POWER/"
    },
    'PSU2': {
        'attr_path': "/sys/class/hwmon/hwmon2/device/NBA615_POWER/",
        'status_path': "/sys/class/hwmon/hwmon2/device/NBA615_POWER/"
    }
}

# SFP LIST
#
#
FAN_LIST = ['FAN1', 'FAN2', 'FAN3', 'FAN4']

FAN_INFO = {
    'FAN1': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA615_FAN/'
    },
    'FAN2': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA615_FAN/'
    },
    'FAN3': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA615_FAN/'
    },
    'FAN4': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/NBA615_FAN/'
    },
}


# SFP LIST
#
#
SFP_EXT_SYSFILE_LIST = ["/sys/class/hwmon/hwmon2/device/NBA615_QSFP/",
                        "/sys/class/hwmon/hwmon2/device/NBA615_SFP/"]

SFP_GROUP_LIST = ['SFP-G01', 'SFP-G02', 'SFP-G03',
                  'SFP-G04', 'SFP-G05', 'SFP-G06', 'SFP-G07']

PORT_NUM = 56

# SFP-eeprom paths /sys/bus/i2c/devices/XX-0050
SFP_GROUPS = {
    'SFP-G01': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_1',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/9-0050", "/sys/bus/i2c/devices/10-0050",
                  "/sys/bus/i2c/devices/11-0050", "/sys/bus/i2c/devices/12-0050",
                  "/sys/bus/i2c/devices/13-0050", "/sys/bus/i2c/devices/14-0050",
                  "/sys/bus/i2c/devices/15-0050", "/sys/bus/i2c/devices/16-0050"],
        'status': 'NOTINST'
    },
    'SFP-G02': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_2',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/17-0050", "/sys/bus/i2c/devices/18-0050",
                  "/sys/bus/i2c/devices/19-0050", "/sys/bus/i2c/devices/20-0050",
                  "/sys/bus/i2c/devices/21-0050", "/sys/bus/i2c/devices/22-0050",
                  "/sys/bus/i2c/devices/23-0050", "/sys/bus/i2c/devices/24-0050"],
        'status': 'NOTINST'
    },
    'SFP-G03': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_3',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/25-0050", "/sys/bus/i2c/devices/26-0050",
                  "/sys/bus/i2c/devices/27-0050", "/sys/bus/i2c/devices/28-0050",
                  "/sys/bus/i2c/devices/29-0050", "/sys/bus/i2c/devices/30-0050",
                  "/sys/bus/i2c/devices/31-0050", "/sys/bus/i2c/devices/32-0050"],
        'status': 'NOTINST'
    },
    'SFP-G04': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_4',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/33-0050", "/sys/bus/i2c/devices/34-0050",
                  "/sys/bus/i2c/devices/35-0050", "/sys/bus/i2c/devices/36-0050",
                  "/sys/bus/i2c/devices/37-0050", "/sys/bus/i2c/devices/38-0050",
                  "/sys/bus/i2c/devices/39-0050", "/sys/bus/i2c/devices/40-0050"],

        'status': 'NOTINST'
    },
    'SFP-G05': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_5',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/41-0050", "/sys/bus/i2c/devices/42-0050",
                  "/sys/bus/i2c/devices/43-0050", "/sys/bus/i2c/devices/44-0050",
                  "/sys/bus/i2c/devices/45-0050", "/sys/bus/i2c/devices/46-0050",
                  "/sys/bus/i2c/devices/47-0050", "/sys/bus/i2c/devices/48-0050"],

        'status': 'NOTINST'
    },
    'SFP-G06': {
        "type": "SFP+",
        'number': 8,
        'parent': 'PCA9548_0x71_6',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe2',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/49-0050", "/sys/bus/i2c/devices/50-0050",
                  "/sys/bus/i2c/devices/51-0050", "/sys/bus/i2c/devices/52-0050",
                  "/sys/bus/i2c/devices/53-0050", "/sys/bus/i2c/devices/54-0050",
                  "/sys/bus/i2c/devices/55-0050", "/sys/bus/i2c/devices/56-0050"],

        'status': 'NOTINST'
    },
    'SFP-G07': {
        "type": "QSFP28",
        'number': 8,
        'parent': 'PCA9548_0x71_7',
        'channels': [0, 1, 2, 3, 4, 5, 6, 7],
        'driver': 'optoe1',
        'i2caddr': '0x50',
        'paths': ["/sys/bus/i2c/devices/57-0050", "/sys/bus/i2c/devices/58-0050",
                  "/sys/bus/i2c/devices/59-0050", "/sys/bus/i2c/devices/60-0050",
                  "/sys/bus/i2c/devices/61-0050", "/sys/bus/i2c/devices/62-0050",
                  "/sys/bus/i2c/devices/63-0050", "/sys/bus/i2c/devices/64-0050"],

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
        self.psu_info = copy.deepcopy(PSU_INFO)
        self.fan_info = copy.deepcopy(FAN_INFO)
        self.sfp_info = copy.deepcopy(SFP_GROUPS)
        self.thermal_info = copy.deepcopy(THERMAL_SENSOR_LIST)

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

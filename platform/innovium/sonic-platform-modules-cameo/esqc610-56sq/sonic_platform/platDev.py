try:
    import os
    import copy
    import json
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger("paltDev")
PLATFORM_INSTALL_INFO_FILE = "/etc/sonic/platform_install.json"
PLATFORM_NAME = "esqc610_56sq"
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
        'name': "NCT7511Y(U48)",
        'temp_index': [1, 2, 4],
        'sysfile_path': "/sys/class/hwmon/hwmon6/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1:['temp_th0_t', 'temp_th0_t_max', 'temp_th0_t_min', 'temp_th0_t_crit', 'temp_th0_t_lcrit'],
                             2:['temp_th0_b', 'temp_th0_b_max', 'temp_th0_b_min', 'temp_th0_b_crit', 'temp_th0_b_lcrit'],
                             4:['temp_th0_r', 'temp_th0_r_max', 'temp_th0_r_min', 'temp_th0_r_crit', 'temp_th0_r_lcrit']}
    },
    {
        'name': "G781(U44)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon3/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1:['temp_th1_t', 'temp_th1_t_max', 'temp_th1_t_min', 'temp_th1_t_crit', 'temp_th1_t_lcrit'],
                             2:['temp_th1_b', 'temp_th1_b_max', 'temp_th1_b_min', 'temp_th1_b_crit', 'temp_th1_b_lcrit']}
    },
    {
        'name': "G781(U45)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon4/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1:['temp_th2_t', 'temp_th2_t_max', 'temp_th2_t_min', 'temp_th2_t_crit', 'temp_th2_t_lcrit'],
                             2:['temp_th2_b', 'temp_th2_b_max', 'temp_th2_b_min', 'temp_th2_b_crit', 'temp_th2_b_lcrit']}
    },
    {
        'name': "G781(U47)",
        'temp_index': [1, 2],
        'sysfile_path': "/sys/class/hwmon/hwmon5/",
        'support_mask': 0x0F,
        'ext_sysfile_list': {1:['temp_th3_t', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit'],
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
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESQC610_POWER/"
    },
    'PSU2': {
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESQC610_POWER/"
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
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESQC610_FAN/'
    },
    'FAN2': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESQC610_FAN/'
    },    
    'FAN3': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESQC610_FAN/'
    },
    'FAN4': {
        'isdraw': True,
        'fan_num': 2,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESQC610_FAN/'
    },
}


# SFP LIST
# 
# 
SFP_EXT_SYSFILE_LIST = ["/sys/class/hwmon/hwmon2/device/ESQC610_QSFP/",
                        "/sys/class/hwmon/hwmon2/device/ESQC610_SFP/"]
                        
SFP_GROUP_LIST = ['SFP-G01', 'SFP-G02', 'SFP-G03', 'SFP-G04', 'SFP-G05', 'SFP-G06', 'SFP-G07']

PORT_NUM = 56

# SFP-eeprom paths /sys/bus/i2c/devices/XX-0050
SFP_GROUP_INFO = {
    "SFP-G01": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G02": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G03": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G04": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G05": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G06": {
        "type": "SFP+",
        "paths": [

        ],
        "number": 8,
    },
    "SFP-G07": {
        "type": "QSFP28",
        "paths": [

        ],
        "number": 8,
    }
}

#
#Component
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
        self.sfp_info = copy.deepcopy(SFP_GROUP_INFO)
        self.device_install_info = dict()
        self.sfp_install_info = dict()
        
        # get install info
        self.get_dev_install_info()
        # update path info with install info
        # Item 1/2 not changed, append directly
        self.thermal_info.append(THERMAL_SENSOR_LIST[0])
        self.thermal_info.append(THERMAL_SENSOR_LIST[1])

        for i in range(2, len(THERMAL_SENSOR_LIST)):
            install_info = self.device_install_info.get(THERMAL_SENSOR_LIST[i]['name'])
            if install_info :
                if self.bmc_is_exist():
                    THERMAL_SENSOR_LIST[i]['sysfile_path'] = '/sys/class/hwmon/hwmon2/device/ESQC610_THERMAL/'
                else:
                    THERMAL_SENSOR_LIST[i]['sysfile_path'] = install_info.get('hwmon_path')
                self.thermal_info.append(THERMAL_SENSOR_LIST[i])

        for psu_name in PSU_LIST:
            install_info = self.device_install_info.get(psu_name)
            if install_info:
                if self.bmc_is_exist():
                    self.psu_info[psu_name]['attr_path']= '/sys/class/hwmon/hwmon2/device/ESQC610_POWER/'
                else:
                    self.psu_info[psu_name]['attr_path'] = install_info.get('hwmon_path')+ '/device/'
        
        for sfp_group_name in SFP_GROUP_LIST:
            install_info = self.sfp_install_info.get(sfp_group_name)
            if install_info:
                self.sfp_info[sfp_group_name]['paths'] = install_info.get('paths')
        
    def get_dev_install_info(self):
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            self.sfp_install_info = install_info[2]
            self.device_install_info = install_info[1]
            
    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath,'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except Exception as ex:
            logger.log_error("Unable to open {} due to {}".format(filepath, repr(ex)))
        
        return None

    def bmc_is_exist(self):
        bmc_filePath = '/sys/class/hwmon/hwmon2/device/ESQC610_SYS/bmc_present'
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

    def get_component_name(self,idx):
        return CHASSIS_COMPONENTS[idx][0]

    def get_component_descript(self,idx):
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







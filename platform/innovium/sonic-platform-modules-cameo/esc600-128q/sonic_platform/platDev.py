try:
    import os
    import copy
    import json
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger("paltDev")
PLATFORM_INSTALL_INFO_FILE = "/etc/sonic/platform_install.json"
PLATFORM_NAME = "esc600_128q"

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
        'name': "pch_haswell",'temp_index': [1],'sysfile_path': "/sys/class/hwmon/hwmon0/",'support_mask': 0x81,
        'ext_sysfile_list': None
    },
    {
        'name': "CPU core temp", 'temp_index': [1, 2],'sysfile_path': "/sys/class/hwmon/hwmon1/",'support_mask': 0x8B,
        'ext_sysfile_list': None
    },
    {
        'name': "NCT7511Y(U2)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon6/",'support_mask': 0x01,
        'ext_sysfile_list': {1:['nct7511_temp', 'temp_th0_t_max', 'temp_th0_t_min', 'temp_th0_t_crit', 'temp_th0_t_lcrit']}
    },
    {
        'name': "G781(U49)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon3/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['left_bot_sb_temp', 'temp_th1_t_max', 'temp_th1_t_min', 'temp_th1_t_crit', 'temp_th1_t_lcrit']}
    },
    {
        'name': "G781(U1)",'temp_index': [1],'sysfile_path': "/sys/class/hwmon/hwmon4/",'support_mask': 0x01,
        'ext_sysfile_list': {1:['ctr_top_sb_temp', 'temp_th2_t_max', 'temp_th2_t_min', 'temp_th2_t_crit', 'temp_th2_t_lcrit']}
    },
    {
        'name': "G781(U21)",'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/",'support_mask': 0x01,
        'ext_sysfile_list': {1:['ctr_sb_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    },
    {
        'name': "G781(U1x)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['left_top_cb_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    },
    {
        'name': "G781(U11)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['ctr_cb_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    },
    {
        'name': "G781(U16)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['right_bot_cb_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    },
    {
        'name': "G781(U17)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['left_bot_cb_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    },
    {
        'name': "G781(U6)", 'temp_index': [1], 'sysfile_path': "/sys/class/hwmon/hwmon5/", 'support_mask': 0x01,
        'ext_sysfile_list': {1:['io_board_temp', 'temp_th3_t_max', 'temp_th3_t_min', 'temp_th3_t_crit', 'temp_th3_t_lcrit']}
    }
]

# PSU LIST
# ext_sysfile_list
#  [0] : sysfile path for present
#  [1] : sysfile path for status
#
PSU_LIST = ['PSU1', 'PSU2', 'PSU3', 'PSU4']

PSU_INFO = {
    'PSU1': {
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESC600_POWER/"
    },
    'PSU2': {
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESC600_POWER/"
    },
    'PSU3': {
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESC600_POWER/"
    },
    'PSU4': {
        'attr_path': "WILL BE RE-INIT",
        'status_path': "/sys/class/hwmon/hwmon2/device/ESC600_POWER/"
    }
}

# SFP LIST
# 
#
FAN_LIST = ['FAN1', 'FAN2', 'FAN3', 'FAN4']

FAN_INFO = {
    'FAN1': {
        'isdraw': True,
        'fan_num': 1,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESC600_FAN/'
    },
    'FAN2': {
        'isdraw': True,
        'fan_num': 1,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESC600_FAN/'
    },    
    'FAN3': {
        'isdraw': True,
        'fan_num': 1,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESC600_FAN/'
    },
    'FAN4': {
        'isdraw': True,
        'fan_num': 1,
        'attr_path': '/sys/class/hwmon/hwmon2/device/ESC600_FAN/'
    },
}


# SFP LIST
# 
# 
SFP_EXT_SYSFILE_LIST = []

PLATFORM_CARD_LIST = ['PHY_CPLD_1', 'PHY_CPLD_2', 'PHY_CPLD_3', 'PHY_CPLD_4', 'PHY_CPLD_5', 'PHY_CPLD_6', 'PHY_CPLD_7',
                      'PHY_CPLD_8']
SFP_GROUP_LIST = ['SFP-G11', 'SFP-G12', 'SFP-G21', 'SFP-G22', 'SFP-G31', 'SFP-G32', 'SFP-G41', 'SFP-G42',
                  'SFP-G51', 'SFP-G52', 'SFP-G61', 'SFP-G62', 'SFP-G71', 'SFP-G72', 'SFP-G81', 'SFP-G82']
PORT_NUM = 0

# SFP-eeprom paths /sys/bus/i2c/devices/XX-0050
SFP_GROUP_INFO = {
    "SFP-G11": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G12": {"type": "QSFP28","paths": [], "number": 8},
    "SFP-G21": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G22": {"type": "QSFP28","paths": [], "number": 8},
    "SFP-G31": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G32": {"type": "QSFP28","paths": [], "number": 8},
    "SFP-G41": {"type": "QSFP28", "paths": [],  "number": 8},"SFP-G42": {"type": "QSFP28","paths": [], "number": 8},
    "SFP-G51": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G52": {"type": "QSFP28", "paths": [], "number": 8},
    "SFP-G61": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G62": {"type": "QSFP28", "paths": [], "number": 8},
    "SFP-G71": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G72": {"type": "QSFP28", "paths": [], "number": 8},
    "SFP-G81": {"type": "QSFP28", "paths": [], "number": 8}, "SFP-G82": {"type": "QSFP28", "paths": [], "number": 8},
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
                    THERMAL_SENSOR_LIST[i]['sysfile_path'] = '/sys/class/hwmon/hwmon2/device/ESC600_THERMAL/'
                else:
                    if install_info.get('hwmon_path') is None:
                        continue
                    THERMAL_SENSOR_LIST[i]['sysfile_path'] = install_info.get('hwmon_path')
                self.thermal_info.append(THERMAL_SENSOR_LIST[i])

        for psu_name in PSU_LIST:
            install_info = self.device_install_info.get(psu_name)
            if install_info:
                if self.bmc_is_exist():
                    self.psu_info[psu_name]['attr_path']= '/sys/class/hwmon/hwmon2/device/ESC600_POWER/'
                else:
                    self.psu_info[psu_name]['attr_path'] = install_info.get('hwmon_path')+ '/device/'
        
        for sfp_group_name in SFP_GROUP_LIST:
            install_info = self.sfp_install_info.get(sfp_group_name)
            if install_info:
                self.sfp_info[sfp_group_name]['paths'] = install_info.get('paths')
                self.sfp_info[sfp_group_name]['number'] = install_info.get('number')
                # 400G line card
                if self.sfp_info[sfp_group_name]['number'] == 4:
                    self.sfp_info[sfp_group_name]['type'] = 'QSFP-DD'
        
    def get_dev_install_info(self):
        global SFP_EXT_SYSFILE_LIST
        global  PORT_NUM
        PORT_NUM =0
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            self.sfp_install_info = install_info[2]
            self.device_install_info = install_info[1]
            for card_name in PLATFORM_CARD_LIST:
                card = install_info[1][card_name]
                if card['portnum'] == 0:
                    continue
                for i in range(1,card['portnum']+1):
                    PORT_NUM = PORT_NUM+1
                    present_file = card['hwmon_path']+'/device/'+'QSFP_present_{}'.format(i)
                    lp_file = card['hwmon_path']+'/device/'+'QSFP_low_power_{}'.format(i)
                    reset_file = card['hwmon_path']+'/device/'+'QSFP_reset_{}'.format(i)
                    SFP_EXT_SYSFILE_LIST.append([present_file,lp_file,reset_file])

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
        bmc_filePath = '/sys/class/hwmon/hwmon2/device/ESC600_SYS/bmc_present'
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







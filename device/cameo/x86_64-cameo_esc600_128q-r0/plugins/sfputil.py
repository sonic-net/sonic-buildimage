#!/usr/bin/env python

try:
    import time
    import json
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PLATFORM_INSTALL_INFO_FILE = "/etc/sonic/platform_install.json"
PLATFORM_SFP_GROUPS = ['SFP-G11', 'SFP-G12', 'SFP-G21', 'SFP-G22', 'SFP-G31', 'SFP-G32', 'SFP-G41', 'SFP-G42',
                       'SFP-G51',
                       'SFP-G52', 'SFP-G61', 'SFP-G62', 'SFP-G71', 'SFP-G72', 'SFP-G81', 'SFP-G82']
PLATFORM_CARD_LIST = ['PHY_CPLD_1', 'PHY_CPLD_2', 'PHY_CPLD_3', 'PHY_CPLD_4', 'PHY_CPLD_5', 'PHY_CPLD_6', 'PHY_CPLD_7',
                      'PHY_CPLD_8']


class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    _port_start = 0
    _port_end = 127
    _port_to_eeprom_mapping = {}
    _global_port_pres_dict = {}
    _port_status_mapping = []

    def __init__(self):
        eeprom_path = "{}/eeprom"
        eeprom_path_list, status_path_list = self.get_sfp_path()
        self._port_end = len(status_path_list) - 1
        self._port_status_mapping = status_path_list
        for x in range(self._port_start, self._port_end + 1):
            port_eeprom_path = eeprom_path.format(eeprom_path_list[x])
            self._port_to_eeprom_mapping[x] = port_eeprom_path
        self.init_global_port_presence()
        SfpUtilBase.__init__(self)

    def get_attr_value(self, attr_path):
        retval = 'ERR'
        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            print("Unable to open ", attr_path, " file !")
            return retval
        retval = retval.rstrip('\r\n')
        fd.close()
        return retval

    def get_sfp_path(self):
        eeprom_path_list=[]
        status_path_list=[]
        with open(PLATFORM_INSTALL_INFO_FILE) as fd:
            install_info = json.load(fd)
            for sfp_group_name in PLATFORM_SFP_GROUPS:
                sfp_group = install_info[2][sfp_group_name]
                for i in range(0,sfp_group['number']):
                    eeprom_path_list.append(sfp_group['paths'][i])
            for card_name in PLATFORM_CARD_LIST:
                card = install_info[1][card_name]
                if card['portnum'] == 0:
                    continue
                for i in range(1,card['portnum']+1):
                    present_file = card['hwmon_path']+'/device/'+'QSFP_present_{}'.format(i)
                    lp_file = card['hwmon_path']+'/device/'+'QSFP_low_power_{}'.format(i)
                    reset_file = card['hwmon_path']+'/device/'+'QSFP_reset_{}'.format(i)
                    status_path_list.append([present_file,lp_file,reset_file])
        return eeprom_path_list, status_path_list
    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False
        pos = self._port_status_mapping[port_num]
        path = pos[2]
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        reg_file.write(str(port_num + 1))
        reg_file.close()
        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        pos = self._port_status_mapping[port_num]
        path = pos[1]
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        # the gpio pin is ACTIVE_HIGH
        if lpmode is True:
            val = "1"
        else:
            val = "0"
        # write value to gpio
        reg_file.seek(0)
        reg_file.write(val)
        reg_file.close()
        return True

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False
        pos = self._port_status_mapping[port_num]
        path = pos[1]
        res = self.get_attr_value(path)
        if res == 'ERR':
            return False
        if int(res) == 1:
            return True
        return False

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False
        pos = self._port_status_mapping[port_num]
        path = pos[0]
        res = self.get_attr_value(path)
        if res == 'ERR':
            return False
        if int(res) == 1:
            return True
        return False

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if (presence):
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        while True:
            for port_num in range(self.port_start, (self.port_end + 1)):
                presence = self.get_presence(port_num)
                if (presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif (not presence and self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'
                if (len(port_dict) > 0):
                    return True, port_dict
            time.sleep(1)

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def qsfp_ports(self):
        return range(0, self._port_end + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

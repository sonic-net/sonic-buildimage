#!/usr/bin/env python


import time

try:
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

ATTR_PATH = '/sys/class/hwmon/hwmon2/device/'

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

QSFP_RESET_FILE = 'NBA715_QSFP/qsfp{}_reset'
QSFP_LOWPOWER_FILE = 'NBA715_QSFP/qsfp{}_low_power'
QSFP_PRESENT_FILE = 'NBA715_QSFP/qsfp{}_present'


class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    _port_start = 0
    _port_end = 31
    _port_in_block = 32
    _port_to_eeprom_mapping = {}
    _global_port_pres_dict = {}

    def __init__(self):
        eeprom_path = "{}/eeprom"
        path_list = self.get_sfp_path()
        for port_n in range(self._port_start, self._port_end + 1):
            port_eeprom_path = eeprom_path.format(path_list[port_n])
            self._port_to_eeprom_mapping[port_n] = port_eeprom_path

        self.init_global_port_presence()
        SfpUtilBase.__init__(self)

    def get_sfp_path(self):
        paths = []
        for sfp_group in SFP_GROUPS:
            paths += SFP_GROUPS[sfp_group]['paths']
        return paths

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        path = ATTR_PATH+QSFP_RESET_FILE.format(port_num+1)
        try:
            reg_file = open(path, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_file.write('1')

        reg_file.close()
        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        path = ATTR_PATH+QSFP_LOWPOWER_FILE.format(port_num+1)
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

        path = ATTR_PATH+QSFP_LOWPOWER_FILE.format(port_num+1)
        try:
            reg_file = open(path, 'r')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        text = reg_file.read()
        reg_file.close()

        return int(text) == 1

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        path = ATTR_PATH+QSFP_PRESENT_FILE.format(port_num+1)
        try:
            reg_file = open(path, 'r')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        text = reg_file.read()
        reg_file.close()
        return int(text) == 1

    def init_global_port_presence(self):
        for port_num in range(self.port_start, (self.port_end + 1)):
            presence = self.get_presence(port_num)
            if presence:
                self._global_port_pres_dict[port_num] = '1'
            else:
                self._global_port_pres_dict[port_num] = '0'

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        while True:
            for port_num in range(self.port_start, (self.port_end + 1)):
                presence = self.get_presence(port_num)
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and
                     self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

                if len(port_dict) > 0:
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
        return range(0, self._port_in_block + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

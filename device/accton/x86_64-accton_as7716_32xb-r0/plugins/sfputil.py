# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    import sys
    import subprocess
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32

    BASE_OOM_PATH = "/sys/bus/i2c/devices/{0}-0050/"
    BASE_CPLD_PATH = "/sys/bus/i2c/devices/0-0060/"
    BASE_I2C_PATH = "/sys/bus/i2c/devices/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        0: [1, 29],
        1: [2, 30],
        2: [3, 31],
        3: [4, 32],
        4: [5, 34],
        5: [6, 33],
        6: [7, 36],
        7: [8, 35],
        8: [9, 25],
        9: [10, 26],
        10: [11, 27],
        11: [12, 28],
        12: [14, 37],
        13: [15, 38],
        14: [16, 39],
        15: [17, 40],
        16: [18, 41],
        17: [19, 42],
        18: [20, 43],
        19: [21, 44],
        20: [22, 53],
        21: [23, 54],
        22: [24, 55],
        23: [25, 56],
        24: [26, 45],
        25: [27, 46],
        26: [28, 47],
        27: [29, 48],
        28: [30, 49],
        29: [31, 50],
        30: [32, 51],
        31: [33, 52],
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        present_path = self.BASE_CPLD_PATH + "module_present_" + str(port_num+1)
        self.__port_to_is_present = present_path

        content = "0"
        try:
            val_file = open(self.__port_to_is_present)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def __init__(self):
        eeprom_path = self.BASE_I2C_PATH

        for x in range(0, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x][1]
            )
            if(x < 9):
                if(self.get_presence(x) == 1):
                    self.port_to_eeprom_mapping[x] = self.BASE_I2C_PATH + "0-000" + str(x+1) + "/eeprom"

            else:
                if(self.get_presence(x) == 1):
                    self.port_to_eeprom_mapping[x] = self.BASE_I2C_PATH + "0-00" + str(x+1) + "/eeprom"

        SfpUtilBase.__init__(self)

    def get_low_power_mode(self, port_num):
        raise NotImplementedError

    def set_low_power_mode(self, port_num, lpmode):
        raise NotImplementedError

    def reset(self, port_num):
        if port_num < self.port_start or port_num > self.port_end:
            return False

        mod_rst_cmd = "ipmitool raw 0x34 0x11 " + str(port_num+1) + " 0x11 0x1"
        subprocess.check_output(mod_rst_cmd, universal_newlines=True)
        return True

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

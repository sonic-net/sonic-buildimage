# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import string
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 31

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"

    _port_to_eeprom_mapping = {}
    _cpld_mapping = {
        0:  "20-0061",
        1:  "21-0062",
    }
    _port_to_i2c_mapping = {
        0:  25,
        1:  26,
        2:  27,
        3:  28,
        4:  29,
        5:  30,
        6:  31,
        7:  32,
        8:  33,
        9: 34,
        10: 35,
        11: 36,
        12: 37,
        13: 38,
        14: 39,
        15: 40,
        16: 41,
        17: 42,
        18: 43,
        19: 44,
        20: 45,
        21: 46,
        22: 47,
        23: 48,
        24: 49,
        25: 50,
        26: 51,
        27: 52,
        28: 53,
        29: 54,
        30: 55,
        31: 56
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_port_start(self):
        return self.PORT_START

    @property
    def qsfp_port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORT_END + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x])

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_ps = self._cpld_mapping[0]
        path = "/sys/bus/i2c/devices/{0}/module_present_{1}"
        port_ps = path.format(cpld_ps, port_num+1)

        content = "0"
        try:
            val_file = open(port_ps)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_ps = self._cpld_mapping[1]
        path = "/sys/bus/i2c/devices/{0}/module_lpmode_{1}"
        port_ps = path.format(cpld_ps, port_num+1)

        content = "0"
        try:
            val_file = open(port_ps)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.PORT_START or port_num > self.PORT_END:
            return False

        cpld_ps = self._cpld_mapping[1]
        path = "/sys/bus/i2c/devices/{0}/module_lpmode_{1}"
        port_ps = path.format(cpld_ps, port_num+1)
        try:
            reg_file = open(port_ps, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_value = '1' if lpmode is True else '0'
        reg_file.write(reg_value)
        reg_file.close()

        return True

    def reset(self, port_num):
        if port_num < self.PORT_START or port_num > self.PORT_END:
            return False

        cpld_ps = self._cpld_mapping[1]
        path = "/sys/bus/i2c/devices/{0}/module_reset_{1}"
        port_ps = path.format(cpld_ps, port_num+1)
        try:
            reg_file = open(port_ps, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_value = '1'
        reg_file.write(reg_value)
        time.sleep(0.1)
        reg_file.seek(0)
        reg_value = '0'
        reg_file.write(reg_value)

        reg_file.close()

        return True

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

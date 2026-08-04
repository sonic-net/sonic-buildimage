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


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 54
    PORTS_IN_BLOCK = 54
    QSFP_PORT_START = 49
    QSFP_PORT_END = 54

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _cpld_mapping = {
        0:  "4-0060",
        1:  "5-0062",
        2:  "6-0064",
    }
    _port_to_i2c_mapping = {
        1:  18,
        2:  19,
        3:  20,
        4:  21,
        5:  22,
        6:  23,
        7:  24,
        8:  25,
        9:  26,
        10: 27,
        11: 28,
        12: 29,
        13: 30,
        14: 31,
        15: 32,
        16: 33,
        17: 34,
        18: 35,
        19: 36,
        20: 37,
        21: 38,
        22: 39,
        23: 40,
        24: 41,
        25: 42,
        26: 43,
        27: 44,
        28: 45,
        29: 46,
        30: 47,
        31: 48,
        32: 49,
        33: 50,
        34: 51,
        35: 52,
        36: 53,
        37: 54,
        38: 55,
        39: 56,
        40: 57,
        41: 58,
        42: 59,
        43: 60,
        44: 61,
        45: 62,
        46: 63,
        47: 64,
        48: 65,
        49: 66,  # QSFP49
        50: 67,
        51: 68,
        52: 69,
        53: 70,
        54: 71,  # QSFP54
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    @property
    def qsfp_port_end(self):
        return self.QSFP_PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.QSFP_PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x])

        SfpUtilBase.__init__(self)

    def get_cpld_num(self, port_num):
        cpld_i = 1
        if (port_num > 24 and port_num < self.qsfp_port_start):
            cpld_i = 2

        if (port_num > 52):
            cpld_i = 2

        return cpld_i

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_i = self.get_cpld_num(port_num)

        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_present_{1}"
        port_ps = path.format(cpld_ps, port_num)

        content = "0"
        try:
            val_file = open(port_ps)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        # content is a string, either "0" or "1"
        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            eeprom = None

            if not self.get_presence(port_num):
                return False

            eeprom = open(self.port_to_eeprom_mapping[port_num], "rb")
            eeprom.seek(93)
            lpmode = ord(eeprom.read(1))

            if ((lpmode & 0x3) == 0x3):
                return True  # Low Power Mode if "Power override" bit is 1 and "Power set" bit is 1
            else:
                # High Power Mode if one of the following conditions is matched:
                # 1. "Power override" bit is 0
                # 2. "Power override" bit is 1 and "Power set" bit is 0
                return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            eeprom = None

            if not self.get_presence(port_num):
                return False  # Port is not present, unable to set the eeprom

            # Fill in write buffer
            regval = 0x3 if lpmode else 0x1  # 0x3:Low Power Mode, 0x1:High Power Mode
            buffer = create_string_buffer(1)
            buffer[0] = chr(regval)

            # Write to eeprom
            eeprom = open(self.port_to_eeprom_mapping[port_num], "r+b")
            eeprom.seek(93)
            eeprom.write(buffer[0])
            return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def reset(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        cpld_i = self.get_cpld_num(port_num)
        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_reset_{1}"
        port_ps = path.format(cpld_ps, port_num)
        try:
            reg_file = open(port_ps, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

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

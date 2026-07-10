# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 53
    PORTS_IN_BLOCK = 54
    QSFP_PORT_START = 48
    QSFP_PORT_END = 53

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        0: 10,
        1: 11,
        2: 12,
        3: 13,
        4: 14,
        5: 15,
        6: 16,
        7: 17,
        8: 18,
        9: 19,
        10: 20,
        11: 21,
        12: 22,
        13: 23,
        14: 24,
        15: 25,
        16: 26,
        17: 27,
        18: 28,
        19: 29,
        20: 30,
        21: 31,
        22: 32,
        23: 33,
        24: 34,
        25: 35,
        26: 36,
        27: 37,
        28: 38,
        29: 39,
        30: 40,
        31: 41,
        32: 42,
        33: 43,
        34: 44,
        35: 45,
        36: 46,
        37: 47,
        38: 48,
        39: 49,
        40: 50,
        41: 51,
        42: 52,
        43: 53,
        44: 54,
        45: 55,
        46: 56,
        47: 57,
        48: 58,
        49: 59,
        50: 60,
        51: 61,
        52: 62,
        53: 63
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
        eeprom_path = "/sys/bus/i2c/devices/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/present")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/lpmod")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            print("\nError:SFP's don't support this property")
            return False

        try:
            reg_file = open("/sys/class/swps/port"+str(port_num)+"/lpmod", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = int(reg_file.readline().rstrip())

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = 1
        else:
            reg_value = 0

        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def reset(self, port_num):
        QSFP_RESET_REGISTER_DEVICE_FILE = "/sys/class/swps/port"+str(port_num)+"/reset"
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            print("\nError:SFP's don't support this property")
            return False

        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 0
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 2 second to allow it to settle
        time.sleep(2)

        # Flip the value back write back to the register to take port out of reset
        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

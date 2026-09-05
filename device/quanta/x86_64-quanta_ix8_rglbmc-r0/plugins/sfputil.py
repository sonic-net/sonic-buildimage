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
    PORT_END = 56
    PORTS_IN_BLOCK = 56
    QSFP_PORT_START = 49
    QSFP_PORT_END = 56

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
         1 : 17,
         2 : 18,
         3 : 19,
         4 : 20,
         5 : 21,
         6 : 22,
         7 : 23,
         8 : 24,
         9 : 25,
        10 : 26,
        11 : 27,
        12 : 28,
        13 : 29,
        14 : 30,
        15 : 31,
        16 : 32,
        17 : 33,
        18 : 34,
        19 : 35,
        20 : 36,
        21 : 37,
        22 : 38,
        23 : 39,
        24 : 40,
        25 : 41,
        26 : 42,
        27 : 43,
        28 : 44,
        29 : 45,
        30 : 46,
        31 : 47,
        32 : 48,
        33 : 49,
        34 : 50,
        35 : 51,
        36 : 52,
        37 : 53,
        38 : 54,
        39 : 55,
        40 : 56,
        41 : 57,
        42 : 58,
        43 : 59,
        44 : 60,
        45 : 61,
        46 : 62,
        47 : 63,
        48 : 64,
        49 : 65,#QSFP49
        50 : 66,#QSFP50
        51 : 67,#QSFP51
        52 : 68,#QSFP52
        53 : 69,#QSFP53
        54 : 70,#QSFP54
        55 : 71,#QSFP55
        56 : 72,#QSFP56
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
            self.port_to_eeprom_mapping[x] = eeprom_path.format(self._port_to_i2c_mapping[x])
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            if port_num < self.qsfp_port_start:
                reg_file = open("/sys/class/cpld-sfp28/port-"+str(port_num)+"/pre_n")
            else:
                reg_file = open("/sys/class/gpio/gpio"+str((port_num-self.qsfp_port_start)*4+34)+"/value")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_file.readline().rstrip()
        if port_num < self.qsfp_port_start:
            if reg_value == '1':
                return True
        else:
            if reg_value == '0':
                return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            reg_file = open("/sys/class/gpio/gpio"+str((port_num-self.qsfp_port_start)*4+35)+"/value")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            reg_file = open("/sys/class/gpio/gpio"+str((port_num-self.qsfp_port_start)*4+35)+"/value", "r+")
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
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/gpio/gpio"+str((port_num-self.qsfp_port_start)*4+32)+"/value", "r+")
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
            reg_file = open("/sys/class/gpio/gpio"+str((port_num-self.qsfp_port_start)*4+32)+"/value", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

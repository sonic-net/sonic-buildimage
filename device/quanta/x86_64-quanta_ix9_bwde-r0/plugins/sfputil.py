try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    _port_start = 1
    _port_end = 32
    ports_in_block = 32

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
         1 : 13,
         2 : 14,
         3 : 15,
         4 : 16,
         5 : 17,
         6 : 18,
         7 : 19,
         8 : 20,
         9 : 21,
        10 : 22,
        11 : 23,
        12 : 24,
        13 : 25,
        14 : 26,
        15 : 27,
        16 : 28,
        17 : 29,
        18 : 30,
        19 : 31,
        20 : 32,
        21 : 33,
        22 : 34,
        23 : 35,
        24 : 36,
        25 : 37,
        26 : 38,
        27 : 39,
        28 : 40,
        29 : 41,
        30 : 42,
        31 : 43,
        32 : 44,
    }

    _qsfp_ports = range(1, ports_in_block + 1)

    def __init__(self):
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self._port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/cpld-qsfpdd/port-"+str(port_num)+"/reset", "r+")
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
            reg_file = open("/sys/class/cpld-qsfpdd/port-"+str(port_num)+"/reset", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/cpld-qsfpdd/port-"+str(port_num)+"/lpmode", "r+")
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

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/cpld-qsfpdd/port-"+str(port_num)+"/lpmode")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = int(reg_file.readline().rstrip())

        if reg_value == 0:
            return False

        return True

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        try:
            reg_file = open("/sys/class/cpld-qsfpdd/port-"+str(port_num)+"/module_present")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_file.readline().rstrip()
        if reg_value == '1':
            return True

        return False

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def osfp_ports(self):
        return range(self.port_start, self.ports_in_block + 1)

    @property
    def qsfp_ports(self):
        return {}

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

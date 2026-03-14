try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    _port_start = 0
    _port_end = 31
    ports_in_block = 32

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        1: 32,
        2: 33,
        3: 34,
        4: 35,
        5: 36,
        6: 37,
        7: 38,
        8: 39,
        9: 40,
        10: 41,
        11: 42,
        12: 43,
        13: 44,
        14: 45,
        15: 46,
        16: 47,
        17: 48,
        18: 49,
        19: 50,
        20: 51,
        21: 52,
        22: 53,
        23: 54,
        24: 55,
        25: 56,
        26: 57,
        27: 58,
        28: 59,
        29: 60,
        30: 61,
        31: 62,
        32: 63,
    }

    _qsfp_ports = list(range(0, ports_in_block + 1))

    def __init__(self):
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(0, self._port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x+1])
            self._port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/class/cpld-qsfp28/port-"+str(port_num+1)+"/reset", "r+")
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
            reg_file = open("/sys/class/cpld-qsfp28/port-"+str(port_num+1)+"/reset", "r+")
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
            reg_file = open("/sys/class/cpld-qsfp28/port-"+str(port_num+1)+"/lpmode", "r+")
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
            reg_file = open("/sys/class/cpld-qsfp28/port-"+str(port_num+1)+"/lpmode")
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

        #path = "/sys/class/cpld-qsfp28/port-{0}/module_present"
        #port_ps = path.format(self.port_to_i2c_mapping[port_num+1])

        try:
            reg_file = open("/sys/class/cpld-qsfp28/port-"+str(port_num+1)+"/module_present")
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
    def qsfp_ports(self):
        return list(range(0, self.ports_in_block + 1))

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

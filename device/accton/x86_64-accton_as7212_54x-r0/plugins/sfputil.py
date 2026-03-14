try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class sfputil(SfpUtilBase):
    """Platform specific sfputil class"""

    port_start = 0
    port_end = 31
    ports_in_block = 32

    port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        9: 18,
        10: 19,
        11: 20,
        12: 21,
        1: 22,
        2: 23,
        3: 24,
        4: 25,
        6: 26,
        5: 27,
        8: 28,
        7: 29,
        13: 30,
        14: 31,
        15: 32,
        16: 33,
        17: 34,
        18: 35,
        19: 36,
        20: 37,
        25: 38,
        26: 39,
        27: 40,
        28: 41,
        29: 42,
        30: 43,
        31: 44,
        32: 45,
        21: 46,
        22: 47,
        23: 48,
        24: 49,
    }

    _qsfp_ports = list(range(0, ports_in_block + 1))

    def __init__(self):
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/sfp_eeprom'
        for x in range(self.port_start, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x+1])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        path = "/sys/bus/i2c/devices/{0}-0050/sfp_port_reset"
        port_ps = path.format(self.port_to_i2c_mapping[port_num+1])

        try:
            reg_file = open(port_ps, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # toggle reset
        reg_file.seek(0)
        reg_file.write('1')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('0')
        reg_file.close()
        return True

    def set_low_power_mode(self, port_nuM, lpmode):
        raise NotImplementedErro

    def get_low_power_mode(self, port_num):
        raise NotImplementedErro

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        path = "/sys/bus/i2c/devices/{0}-0050/sfp_is_present"
        port_ps = path.format(self.port_to_i2c_mapping[port_num+1])

        reg_value = '0'
        try:
            reg_file = open(port_ps)
            reg_value = reg_file.readline().rstrip()
            reg_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

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

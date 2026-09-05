try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpUtil(SfpUtilBase):
    """Platform specific SfpUtill class"""

    first_port = 0
    last_port = 65
    port_num = 63

    port_to_eeprom = {}
    port_to_i2cbus_mapping = {
        1: 13,
        2: 14,
        3: 15,
        4: 16,
        5: 23,
    }

    eeprom_path_1 = "/sys/bus/i2c/devices/{0}-0020/sfp{1}_eeprom"
    eeprom_path = "/sys/bus/i2c/devices/{0}-005f/sfp{1}_eeprom"
    port_reset_path_1 = "/sys/bus/i2c/devices/{0}-0020/sfp{1}_port_reset"
    port_reset_path = "/sys/bus/i2c/devices/{0}-005f/sfp{1}_port_reset"
    present_path_1 = "/sys/bus/i2c/devices/{0}-0020/sfp{1}_is_present"
    present_path = "/sys/bus/i2c/devices/{0}-005f/sfp{1}_is_present"

    _qsfp_ports = list(range(first_port, port_num))

    @property
    def port_start(self):
        return self.first_port

    @property
    def port_end(self):
        return self.last_port

    @property
    def qsfp_ports(self):
        return list(range(self.first_port, self.port_num + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self.port_to_eeprom

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

    def __init__(self):
        for x in range(self.first_port, self.last_port + 1):
            cpld_index = (x / 16) + 1
            index = (x % 16) + 1
            if cpld_index == 5:
                path = self.eeprom_path_1
            else:
                path = self.eeprom_path
            self.port_to_eeprom[x] = path.format(self.port_to_i2cbus_mapping[cpld_index], index)
        SfpUtilBase.__init__(self)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.first_port or port_num > self.last_port:
            return False

        cpld_index = (port_num / 16) + 1
        index = (port_num % 16) + 1
        if cpld_index == 5:
            path = self.port_reset_path_1
        else:
            path = self.port_reset_path
        port_path = path.format(self.port_to_i2cbus_mapping[cpld_index], index)

        try:
            reg_file = open(port_path, 'w')
        except IOError as e:
            if cpld_index < 5:
                print("Error: unable to open file: %s" % str(e))
            return False

        # reset
        reg_file.write('1')

        time.sleep(1)

        reg_file.write('0')

        reg_file.close()
        return True

    def set_low_power_mode(self, port_nuM, lpmode):
        raise NotImplementedError

    def get_low_power_mode(self, port_num):
        raise NotImplementedError

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.first_port or port_num > self.last_port:
            return False

        cpld_index = (port_num / 16) + 1
        index = (port_num % 16) + 1
        if cpld_index == 5:
            path = self.present_path_1
        else:
            path = self.present_path
        port_path = path.format(self.port_to_i2cbus_mapping[cpld_index], index)

        try:
            reg_file = open(port_path)
        except IOError as e:
            if cpld_index < 5:
                print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_file.readline().rstrip()
        if reg_value == '1':
            return True

        return False

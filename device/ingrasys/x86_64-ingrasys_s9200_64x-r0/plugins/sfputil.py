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
    PORT_END = 63
    PORTS_IN_BLOCK = 64

    EEPROM_OFFSET = 29

    _port_to_eeprom_mapping = {}

    _logic_to_phy_port_mapping = {
        0: 0,
        1: 1,
        2: 4,
        3: 5,
        4: 8,
        5: 9,
        6: 12,
        7: 13,
        8: 16,
        9: 17,
        10: 20,
        11: 21,
        12: 24,
        13: 25,
        14: 28,
        15: 29,
        16: 32,
        17: 33,
        18: 36,
        19: 37,
        20: 40,
        21: 41,
        22: 44,
        23: 45,
        24: 48,
        25: 49,
        26: 52,
        27: 53,
        28: 56,
        29: 57,
        30: 60,
        31: 61,
        32: 2,
        33: 3,
        34: 6,
        35: 7,
        36: 10,
        37: 11,
        38: 14,
        39: 15,
        40: 18,
        41: 19,
        42: 22,
        43: 23,
        44: 26,
        45: 27,
        46: 30,
        47: 31,
        48: 34,
        49: 35,
        50: 38,
        51: 39,
        52: 42,
        53: 43,
        54: 46,
        55: 47,
        56: 50,
        57: 51,
        58: 54,
        59: 55,
        60: 58,
        61: 59,
        62: 62,
        63: 63
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(0, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            phy_port = self._logic_to_phy_port_mapping[x]
            self._port_to_eeprom_mapping[x] = eeprom_path.format(phy_port + self.EEPROM_OFFSET)

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

    # logic port to physical port mapping
        port_num = self._logic_to_phy_port_mapping[port_num]

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s9200-cpld.0/qsfp_modprs")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

    # logic port to physical port mapping
        port_num = self._logic_to_phy_port_mapping[port_num]

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s9200-cpld.0/qsfp_lpmode")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # LPMode is active high
        if reg_value & mask == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

    # logic port to physical port mapping
        port_num = self._logic_to_phy_port_mapping[port_num]

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s9200-cpld.0/qsfp_lpmode", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = reg_value | mask
        else:
            reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = format(reg_value, 'x')

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def reset(self, port_num):
        QSFP_RESET_REGISTER_DEVICE_FILE = "/sys/devices/platform/ingrasys-s9200-cpld.0/qsfp_reset"

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

    # logic port to physical port mapping
        port_num = self._logic_to_phy_port_mapping[port_num]

        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # File content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ResetL is active low
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = format(reg_value, 'x')

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_value | mask
        content = format(reg_value, 'x')

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

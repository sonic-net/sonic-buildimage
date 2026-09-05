# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import subprocess
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


i2c_set = 'i2cset'
i2c_get = 'i2cget'
cpld_addr = '0x33'
mux_reg = '0x4A'


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 63
    QSFP_PORT_START = 48
    PORTS_IN_BLOCK = 64

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        0: [2, 1],
        1: [2, 2],
        2: [2, 3],
        3: [2, 4],
        4: [2, 5],
        5: [2, 6],
        6: [2, 7],
        7: [2, 8],
        8: [2, 9],
        9: [2, 10],
        10: [2, 11],
        11: [2, 12],
        12: [2, 13],
        13: [2, 14],
        14: [2, 15],
        15: [2, 16],
        16: [2, 17],
        17: [2, 18],
        18: [2, 19],
        19: [2, 20],
        20: [2, 21],
        21: [2, 22],
        22: [2, 23],
        23: [2, 24],
        24: [3, 25],
        25: [3, 26],
        26: [3, 27],
        27: [3, 28],
        28: [3, 29],
        29: [3, 30],
        30: [3, 31],
        31: [3, 32],
        32: [3, 33],
        33: [3, 34],
        34: [3, 35],
        35: [3, 36],
        36: [3, 37],
        37: [3, 38],
        38: [3, 39],
        39: [3, 40],
        40: [3, 41],
        41: [3, 42],
        42: [3, 43],
        43: [3, 44],
        44: [3, 45],
        45: [3, 46],
        46: [3, 47],
        47: [3, 48],
        48: [4, 49],
        49: [4, 50],
        50: [4, 51],
        51: [4, 52],
        52: [4, 53],
        53: [4, 54],
        54: [4, 55],
        55: [4, 56],
        56: [4, 57],
        57: [4, 58],
        58: [4, 59],
        59: [4, 60],
        60: [4, 61],
        61: [4, 62],
        62: [4, 63],
        63: [4, 64]
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.QSFP_PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    def __init__(self):

        sfp_eeprom_path = '/sys/bus/i2c/devices/{0[0]}-0050/sfp{0[1]}'
        qsfp_eeprom_path = '/sys/bus/i2c/devices/{0[0]}-0050/qsfp{0[1]}'
        for x in range(self.port_start, self.qsfp_port_start):
            port_eeprom_path = sfp_eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        for x in range(self.qsfp_port_start, self.port_end + 1):
            port_eeprom_path = qsfp_eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_modprs")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # content is a string, either "0" or "1"
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_lpmode")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << (port_num - self.qsfp_port_start))

        # LPMode is active high
        if reg_value & mask == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_lpmode", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << (port_num - self.qsfp_port_start))

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = reg_value | mask
        else:
            reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def reset(self, port_num):
        QSFP_RESET_REGISTER_DEVICE_FILE = "/sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_reset"

        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # File content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << (port_num - self.qsfp_port_start))

        # ResetL is active low
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
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
        reg_file.seek(0)
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

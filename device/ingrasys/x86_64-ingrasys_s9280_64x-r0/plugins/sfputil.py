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
    EEPROM_OFFSET = 41
    CPLD1_PORTS = 12
    CPLDx_PORTS = 13
    # TODO: check init sequence for CPLD i2c bus
    CPLD_OFFSET = 1
    CPLD_PRES_BIT = 1
    CPLD_RESET_BIT = 0
    CPLD_LPMOD_BIT = 2
    CPLDx_I2C_ADDR = "33"
    EEPROM_I2C_ADDR = "50"
    CPLD_PORT_STATUS_KEY = "cpld_qsfp_port_status"
    CPLD_PORT_CONFIG_KEY = "cpld_qsfp_port_config"
    CPLD_REG_PATH = "/sys/bus/i2c/devices/{0}-00{1}/{2}_{3}"

    _port_to_eeprom_mapping = {}

    # TODO: check the fp port to phy port mapping
    _fp2phy_port_mapping = {
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
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-00{1}/eeprom"
        for x in range(self.port_start, self.port_end + 1):
            phy_port = self.fp2phy_port_num(x)
            port_eeprom_path = eeprom_path.format(phy_port + self.EEPROM_OFFSET, self.EEPROM_I2C_ADDR)
            self.port_to_eeprom_mapping[x] = port_eeprom_path

        SfpUtilBase.__init__(self)

    def qsfp_to_cpld_index(self, port_num):
        if port_num < self.CPLD1_PORTS:
            cpld_id = 0
            cpld_port_index = port_num + 1
        else:
            cpld_id = 1 + (port_num - self.CPLD1_PORTS) / self.CPLDx_PORTS
            cpld_port_index = ((port_num - self.CPLD1_PORTS) % self.CPLDx_PORTS) + 1
        return cpld_id, cpld_port_index

    def fp2phy_port_num(self, fp_port_num):

        phy_port_num = self._fp2phy_port_mapping[fp_port_num]
        return phy_port_num

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # logic port to physical port mapping
        port_num = self.fp2phy_port_num(port_num)

        cpld_id, cpld_port_index = self.qsfp_to_cpld_index(port_num)
        i2c_id = self.CPLD_OFFSET + cpld_id
        reg_path = self.CPLD_REG_PATH.format(i2c_id, self.CPLDx_I2C_ADDR,
                                             self.CPLD_PORT_STATUS_KEY, cpld_port_index)

        try:
            reg_file = open(reg_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the status register value
        content = reg_file.readline().rstrip()
        reg_file.close()

        reg_value = int(content, 16)
        # mask for presence bit (bit 1)
        mask = (1 << self.CPLD_PRES_BIT)

        # 0 - presence, 1 - absence
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # logic port to physical port mapping
        port_num = self.fp2phy_port_num(port_num)

        cpld_id, cpld_port_index = self.qsfp_to_cpld_index(port_num)
        i2c_id = self.CPLD_OFFSET + cpld_id
        reg_path = self.CPLD_REG_PATH.format(i2c_id, self.CPLDx_I2C_ADDR,
                                             self.CPLD_PORT_CONFIG_KEY, cpld_port_index)

        try:
            reg_file = open(reg_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the status register value
        content = reg_file.readline().rstrip()
        reg_file.close()

        reg_value = int(content, 16)
        # mask for lp_mod bit (bit 2)
        mask = (1 << self.CPLD_LPMOD_BIT)

        # 0 - disable, 1 - low power mode
        if reg_value & mask == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # logic port to physical port mapping
        port_num = self.fp2phy_port_num(port_num)

        cpld_id, cpld_port_index = self.qsfp_to_cpld_index(port_num)
        i2c_id = self.CPLD_OFFSET + cpld_id
        reg_path = self.CPLD_REG_PATH.format(i2c_id, self.CPLDx_I2C_ADDR,
                                             self.CPLD_PORT_CONFIG_KEY, cpld_port_index)

        try:
            reg_file = open(reg_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the status register value
        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        # mask for lp_mod bit (bit 2)
        mask = (1 << self.CPLD_LPMOD_BIT)

        # 1 - low power mode, 0 - high power mode
        if lpmode is True:
            reg_value = reg_value | mask
        else:
            reg_value = reg_value & ~mask

        # convert value to hex string
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # logic port to physical port mapping
        port_num = self.fp2phy_port_num(port_num)

        cpld_id, cpld_port_index = self.qsfp_to_cpld_index(port_num)
        i2c_id = self.CPLD_OFFSET + cpld_id
        reg_path = self.CPLD_REG_PATH.format(i2c_id, self.CPLDx_I2C_ADDR,
                                             self.CPLD_PORT_CONFIG_KEY, cpld_port_index)

        # reset the port
        try:
            reg_file = open(reg_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the status register value
        content = reg_file.readline().rstrip()
        reg_value = int(content, 16)
        # mask for reset bit (bit 0)
        mask = (1 << self.CPLD_RESET_BIT)

        # 1 - out of reset, 0 - reset
        reg_value = reg_value & ~mask

        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 1 second to reset done
        time.sleep(1)

        # take the port out of reset
        try:
            reg_file = open(reg_path, "w")
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

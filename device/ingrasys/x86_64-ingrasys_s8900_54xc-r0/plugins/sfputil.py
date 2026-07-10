# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

import os

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 53
    QSFP_PORT_START = 48
    PORTS_IN_BLOCK = 54
    GPIO_OFFSET = 0

    BASE_DIR_PATH = "/sys/class/gpio/gpio{0}/direction"
    BASE_VAL_PATH = "/sys/class/gpio/gpio{0}/value"

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        0: 18,
        1: 19,
        2: 20,
        3: 21,
        4: 22,
        5: 23,
        6: 24,
        7: 25,
        8: 26,
        9: 27,
        10: 28,
        11: 29,
        12: 30,
        13: 31,
        14: 32,
        15: 33,
        16: 34,
        17: 35,
        18: 36,
        19: 37,
        20: 38,
        21: 39,
        22: 40,
        23: 41,
        24: 42,
        25: 43,
        26: 44,
        27: 45,
        28: 46,
        29: 47,
        30: 48,
        31: 49,
        32: 50,
        33: 51,
        34: 52,
        35: 53,
        36: 54,
        37: 55,
        38: 56,
        39: 57,
        40: 58,
        41: 59,
        42: 60,
        43: 61,
        44: 62,
        45: 63,
        46: 64,
        47: 65,
        48: 66,
        49: 67,
        50: 68,
        51: 69,
        52: 70,
        53: 71
    }

    abs_to_gpio_mapping = {}
    lpmode_to_gpio_mapping = {}
    reset_to_gpio_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.QSFP_PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def set_gpio_offset(self):
        sys_gpio_dir = "/sys/class/gpio"
        self.GPIO_OFFSET = 0
        gpiochip_no = 0
        for d in os.listdir(sys_gpio_dir):
            if "gpiochip" in d:
                try:
                    gpiochip_no = int(d[8:], 10)
                except ValueError as e:
                    print("Error: %s" % str(e))
                if gpiochip_no > 255:
                    self.GPIO_OFFSET = 256
                    return True
        return True

    def init_abs_to_gpio_mapping(self):
        self.abs_to_gpio_mapping = {
            0: 192+self.GPIO_OFFSET,
            1: 193+self.GPIO_OFFSET,
            2: 194+self.GPIO_OFFSET,
            3: 195+self.GPIO_OFFSET,
            4: 196+self.GPIO_OFFSET,
            5: 197+self.GPIO_OFFSET,
            6: 198+self.GPIO_OFFSET,
            7: 199+self.GPIO_OFFSET,
            8: 200+self.GPIO_OFFSET,
            9: 201+self.GPIO_OFFSET,
            10: 202+self.GPIO_OFFSET,
            11: 203+self.GPIO_OFFSET,
            12: 204+self.GPIO_OFFSET,
            13: 205+self.GPIO_OFFSET,
            14: 206+self.GPIO_OFFSET,
            15: 207+self.GPIO_OFFSET,
            16: 176+self.GPIO_OFFSET,
            17: 177+self.GPIO_OFFSET,
            18: 178+self.GPIO_OFFSET,
            19: 179+self.GPIO_OFFSET,
            20: 180+self.GPIO_OFFSET,
            21: 181+self.GPIO_OFFSET,
            22: 182+self.GPIO_OFFSET,
            23: 183+self.GPIO_OFFSET,
            24: 184+self.GPIO_OFFSET,
            25: 185+self.GPIO_OFFSET,
            26: 186+self.GPIO_OFFSET,
            27: 187+self.GPIO_OFFSET,
            28: 188+self.GPIO_OFFSET,
            29: 189+self.GPIO_OFFSET,
            30: 190+self.GPIO_OFFSET,
            31: 191+self.GPIO_OFFSET,
            32: 160+self.GPIO_OFFSET,
            33: 161+self.GPIO_OFFSET,
            34: 162+self.GPIO_OFFSET,
            35: 163+self.GPIO_OFFSET,
            36: 164+self.GPIO_OFFSET,
            37: 165+self.GPIO_OFFSET,
            38: 166+self.GPIO_OFFSET,
            39: 167+self.GPIO_OFFSET,
            40: 168+self.GPIO_OFFSET,
            41: 169+self.GPIO_OFFSET,
            42: 170+self.GPIO_OFFSET,
            43: 171+self.GPIO_OFFSET,
            44: 172+self.GPIO_OFFSET,
            45: 173+self.GPIO_OFFSET,
            46: 174+self.GPIO_OFFSET,
            47: 175+self.GPIO_OFFSET,
            48: 240+self.GPIO_OFFSET,
            49: 241+self.GPIO_OFFSET,
            50: 242+self.GPIO_OFFSET,
            51: 243+self.GPIO_OFFSET,
            52: 244+self.GPIO_OFFSET,
            53: 245+self.GPIO_OFFSET
        }
        return True

    def init_lpmode_to_gpio_mapping(self):
        self.lpmode_to_gpio_mapping = {
            48: 224+self.GPIO_OFFSET,
            49: 225+self.GPIO_OFFSET,
            50: 226+self.GPIO_OFFSET,
            51: 227+self.GPIO_OFFSET,
            52: 228+self.GPIO_OFFSET,
            53: 229+self.GPIO_OFFSET
        }
        return True

    def init_reset_to_gpio_mapping(self):
        self.reset_to_gpio_mapping = {
            48: 208+self.GPIO_OFFSET,
            49: 209+self.GPIO_OFFSET,
            50: 210+self.GPIO_OFFSET,
            51: 211+self.GPIO_OFFSET,
            52: 212+self.GPIO_OFFSET,
            53: 213+self.GPIO_OFFSET
        }
        return True

    def __init__(self):
        # Init abs, lpmode, and reset to gpio mapping
        self.set_gpio_offset()
        self.init_abs_to_gpio_mapping()
        self.init_lpmode_to_gpio_mapping()
        self.init_reset_to_gpio_mapping()

        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(self.port_start, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            abs_device_file = self.BASE_VAL_PATH.format(
                self.abs_to_gpio_mapping[port_num])
            val_file = open(abs_device_file)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = val_file.readline().rstrip()

        # content is a string, either "0" or "1"
        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                self.lpmode_to_gpio_mapping[port_num])
            val_file = open(lpmode_val_device_file)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = val_file.readline().rstrip()

        # content is a string, either "0" or "1"
        if content == "1":
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                self.lpmode_to_gpio_mapping[port_num])
            val_file = open(lpmode_val_device_file, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        val_file.write("1" if lpmode is True else "0")
        val_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.port_end:
            print("Error: unable to reset non-QSFP module: port %s" % str(port_num))
            return False

        try:
            print("port %s" % str(port_num))
            reset_val_device_file = self.BASE_VAL_PATH.format(
                self.reset_to_gpio_mapping[port_num])
            val_file = open(reset_val_device_file, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        val_file.write("1")
        val_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        try:
            reset_val_device_file = self.BASE_VAL_PATH.format(
                self.reset_to_gpio_mapping[port_num])
            val_file = open(reset_val_device_file, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        val_file.write("0")
        val_file.close()

        return True

    def get_transceiver_change_event(self):
        """
        TODO: This function need to be implemented
        when decide to support monitoring SFP(Xcvrd)
        on this platform.
        """
        raise NotImplementedError

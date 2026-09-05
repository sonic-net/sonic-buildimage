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
    PORT_END = 33
    SFP_PORT_START = 32
    PORTS_IN_BLOCK = 34
    GPIO_OFFSET = 0

    BASE_DIR_PATH = "/sys/class/gpio/gpio{0}/direction"
    BASE_VAL_PATH = "/sys/class/gpio/gpio{0}/value"

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        0: 10,
        1: 9,
        2: 12,
        3: 11,
        4: 14,
        5: 13,
        6: 16,
        7: 15,
        8: 18,
        9: 17,
        10: 20,
        11: 19,
        12: 22,
        13: 21,
        14: 24,
        15: 23,
        16: 26,
        17: 25,
        18: 28,
        19: 27,
        20: 30,
        21: 29,
        22: 32,
        23: 31,
        24: 34,
        25: 33,
        26: 36,
        27: 35,
        28: 38,
        29: 37,
        30: 40,
        31: 39,
        32: 45,
        33: 46
    }

    abs_to_gpio_mapping = {}
    lpmode_to_gpio_mapping = {}
    reset_to_gpio_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return range(0, self.PORTS_IN_BLOCK + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping
    @property
    def sfp_port_start(self):
        return self.SFP_PORT_START

    def set_gpio_offset(self):
        sys_gpio_dir = "/sys/class/gpio"
        self.GPIO_OFFSET = 0
        gpiochip_no = 0
        for d in os.listdir(sys_gpio_dir):
            if "gpiochip" in d:
                try:
                    gpiochip_no = int(d[8:],10)
                except ValueError as e:
                    print "Error: %s" % str(e)
                if gpiochip_no > 255:
                    self.GPIO_OFFSET=256
                    return True
        return True

    def init_abs_to_gpio_mapping(self):
        self.abs_to_gpio_mapping = {
            0: 241+self.GPIO_OFFSET,
            1: 240+self.GPIO_OFFSET,
            2: 243+self.GPIO_OFFSET,
            3: 242+self.GPIO_OFFSET,
            4: 245+self.GPIO_OFFSET,
            5: 244+self.GPIO_OFFSET,
            6: 247+self.GPIO_OFFSET,
            7: 246+self.GPIO_OFFSET,
            8: 249+self.GPIO_OFFSET,
            9: 248+self.GPIO_OFFSET,
            10: 251+self.GPIO_OFFSET,
            11: 250+self.GPIO_OFFSET,
            12: 253+self.GPIO_OFFSET,
            13: 252+self.GPIO_OFFSET,
            14: 255+self.GPIO_OFFSET,
            15: 254+self.GPIO_OFFSET,
            16: 225+self.GPIO_OFFSET,
            17: 224+self.GPIO_OFFSET,
            18: 227+self.GPIO_OFFSET,
            19: 226+self.GPIO_OFFSET,
            20: 229+self.GPIO_OFFSET,
            21: 228+self.GPIO_OFFSET,
            22: 231+self.GPIO_OFFSET,
            23: 230+self.GPIO_OFFSET,
            24: 233+self.GPIO_OFFSET,
            25: 232+self.GPIO_OFFSET,
            26: 235+self.GPIO_OFFSET,
            27: 234+self.GPIO_OFFSET,
            28: 237+self.GPIO_OFFSET,
            29: 236+self.GPIO_OFFSET,
            30: 239+self.GPIO_OFFSET,
            31: 238+self.GPIO_OFFSET,
            32: 177+self.GPIO_OFFSET,
            33: 176+self.GPIO_OFFSET
        }
        return True

    def init_lpmode_to_gpio_mapping(self):
        self.lpmode_to_gpio_mapping = {
            0: 161+self.GPIO_OFFSET,
            1: 160+self.GPIO_OFFSET,
            2: 163+self.GPIO_OFFSET,
            3: 162+self.GPIO_OFFSET,
            4: 165+self.GPIO_OFFSET,
            5: 164+self.GPIO_OFFSET,
            6: 167+self.GPIO_OFFSET,
            7: 166+self.GPIO_OFFSET,
            8: 169+self.GPIO_OFFSET,
            9: 168+self.GPIO_OFFSET,
            10: 171+self.GPIO_OFFSET,
            11: 170+self.GPIO_OFFSET,
            12: 173+self.GPIO_OFFSET,
            13: 172+self.GPIO_OFFSET,
            14: 175+self.GPIO_OFFSET,
            15: 174+self.GPIO_OFFSET,
            16: 145+self.GPIO_OFFSET,
            17: 144+self.GPIO_OFFSET,
            18: 147+self.GPIO_OFFSET,
            19: 146+self.GPIO_OFFSET,
            20: 149+self.GPIO_OFFSET,
            21: 148+self.GPIO_OFFSET,
            22: 151+self.GPIO_OFFSET,
            23: 150+self.GPIO_OFFSET,
            24: 153+self.GPIO_OFFSET,
            25: 152+self.GPIO_OFFSET,
            26: 155+self.GPIO_OFFSET,
            27: 154+self.GPIO_OFFSET,
            28: 157+self.GPIO_OFFSET,
            29: 156+self.GPIO_OFFSET,
            30: 159+self.GPIO_OFFSET,
            31: 158+self.GPIO_OFFSET 
        }
        return True

    def init_reset_to_gpio_mapping(self):
        self.reset_to_gpio_mapping = { 
            0: 129+self.GPIO_OFFSET,
            1: 128+self.GPIO_OFFSET,
            2: 131+self.GPIO_OFFSET,
            3: 130+self.GPIO_OFFSET,
            4: 133+self.GPIO_OFFSET,
            5: 132+self.GPIO_OFFSET,
            6: 135+self.GPIO_OFFSET,
            7: 134+self.GPIO_OFFSET,
            8: 137+self.GPIO_OFFSET,
            9: 136+self.GPIO_OFFSET,
            10: 139+self.GPIO_OFFSET,
            11: 138+self.GPIO_OFFSET,
            12: 141+self.GPIO_OFFSET,
            13: 140+self.GPIO_OFFSET,
            14: 143+self.GPIO_OFFSET,
            15: 142+self.GPIO_OFFSET,
            16: 113+self.GPIO_OFFSET,
            17: 112+self.GPIO_OFFSET,
            18: 115+self.GPIO_OFFSET,
            19: 114+self.GPIO_OFFSET,
            20: 117+self.GPIO_OFFSET,
            21: 116+self.GPIO_OFFSET,
            22: 119+self.GPIO_OFFSET,
            23: 118+self.GPIO_OFFSET,
            24: 121+self.GPIO_OFFSET,
            25: 120+self.GPIO_OFFSET,
            26: 123+self.GPIO_OFFSET,
            27: 122+self.GPIO_OFFSET,
            28: 125+self.GPIO_OFFSET,
            29: 124+self.GPIO_OFFSET,
            30: 127+self.GPIO_OFFSET,
            31: 126+self.GPIO_OFFSET 
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
            print "Error: unable to open file: %s" % str(e)
            return False

        content = val_file.readline().rstrip()
        val_file.close()

        # content is a string, either "0" or "1"
        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.sfp_port_start: # TBD
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                    self.lpmode_to_gpio_mapping[port_num])
            val_file = open(lpmode_val_device_file)
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        content = val_file.readline().rstrip()
        val_file.close()

        # content is a string, either "0" or "1"
        if content == "1":
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.sfp_port_start: # TBD
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                    self.lpmode_to_gpio_mapping[port_num])
            val_file = open(lpmode_val_device_file, "w")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            return False

        val_file.write("1" if lpmode is True else "0")
        val_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.sfp_port_start: # TBD
            return False

        try:
            reset_val_device_file = self.BASE_VAL_PATH.format(
                    self.reset_to_gpio_mapping[port_num])
            val_file = open(reset_val_device_file, "w")
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
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
            print "Error: unable to open file: %s" % str(e)
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

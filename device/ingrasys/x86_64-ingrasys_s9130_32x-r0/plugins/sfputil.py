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
    PORT_END = 31
    PORTS_IN_BLOCK = 32
    EEPROM_OFFSET = 21
    ABS_GPIO_BASE_0_15 = 240
    ABS_GPIO_BASE_16_31 = 224
    LP_MODE_GPIO_BASE_0_15 = 176
    LP_MODE_GPIO_BASE_16_31 = 160
    RST_GPIO_BASE_0_15 = 144
    RST_GPIO_BASE_16_31 = 128
    GPIO_OFFSET = 0

    GPIO_VAL_PATH = "/sys/class/gpio/gpio{0}/value"

    _port_to_eeprom_mapping = {}

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

    def update_gpio_base(self):
        self.ABS_GPIO_BASE_0_15 = 240 + self.GPIO_OFFSET
        self.ABS_GPIO_BASE_16_31 = 224 + self.GPIO_OFFSET
        self.LP_MODE_GPIO_BASE_0_15 = 176 + self.GPIO_OFFSET
        self.LP_MODE_GPIO_BASE_16_31 = 160 + self.GPIO_OFFSET
        self.RST_GPIO_BASE_0_15 = 144 + self.GPIO_OFFSET
        self.RST_GPIO_BASE_16_31 = 128 + self.GPIO_OFFSET
        return True

    def __init__(self):
        # Update abs, lpmode, and reset gpio base
        self.set_gpio_offset()
        self.update_gpio_base()

        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end + 1):
            port_eeprom_path = eeprom_path.format(x + self.EEPROM_OFFSET)
            self.port_to_eeprom_mapping[x] = port_eeprom_path

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # open corrsponding gpio file
        try:
            if port_num <= 15:
                gpio_base = self.ABS_GPIO_BASE_0_15
            else:
                gpio_base = self.ABS_GPIO_BASE_16_31
            gpio_index = gpio_base + (port_num % 16)
            gpio_file_path = self.GPIO_VAL_PATH.format(gpio_index)
            gpio_file = open(gpio_file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the gpio value
        gpio_val = int(gpio_file.readline().rstrip())
        gpio_file.close()

        # the gpio pin is ACTIVE_LOW but reversed
        if gpio_val == 0:
            return False

        return True

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # open corrsponding gpio file
        try:
            if port_num <= 15:
                gpio_base = self.LP_MODE_GPIO_BASE_0_15
            else:
                gpio_base = self.LP_MODE_GPIO_BASE_16_31
            gpio_index = gpio_base + (port_num % 16)
            gpio_file_path = self.GPIO_VAL_PATH.format(gpio_index)
            gpio_file = open(gpio_file_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # content is a string containing the gpio value
        gpio_val = int(gpio_file.readline().rstrip())
        gpio_file.close()

        if gpio_val == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # open corrsponding gpio file
        try:
            if port_num <= 15:
                gpio_base = self.LP_MODE_GPIO_BASE_0_15
            else:
                gpio_base = self.LP_MODE_GPIO_BASE_16_31
            gpio_index = gpio_base + (port_num % 16)
            gpio_file_path = self.GPIO_VAL_PATH.format(gpio_index)
            gpio_file = open(gpio_file_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # the gpio pin is ACTIVE_HIGH
        if lpmode is True:
            gpio_val = "1"
        else:
            gpio_val = "0"

        # write value to gpio
        gpio_file.seek(0)
        gpio_file.write(gpio_val)
        gpio_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # open corrsponding gpio file
        try:
            if port_num <= 15:
                gpio_base = self.RST_GPIO_BASE_0_15
            else:
                gpio_base = self.RST_GPIO_BASE_16_31
            gpio_index = gpio_base + (port_num % 16)
            gpio_file_path = self.GPIO_VAL_PATH.format(gpio_index)
            gpio_file = open(gpio_file_path, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # set the gpio to take port into reset
        # the gpio pin is ACTIVE_LOW but reversed
        gpio_val = "1"
        # write value to gpio
        gpio_file.seek(0)
        gpio_file.write(gpio_val)
        gpio_file.close()

        # Sleep 1 second to let it settle
        time.sleep(1)

        # open corrsponding gpio file
        try:
            gpio_file = open(gpio_file_path, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # set gpio back low to take port out of reset
        # the gpio pin is ACTIVE_LOW but reversed
        gpio_val = "0"
        # write value to gpio
        gpio_file.seek(0)
        gpio_file.write(gpio_val)
        gpio_file.close()

        return True

    def get_transceiver_change_event(self, timeout=0):
        raise NotImplementedError

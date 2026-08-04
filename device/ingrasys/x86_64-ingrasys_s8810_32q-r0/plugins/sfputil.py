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

    EEPROM_OFFSET = 18

    ABS_GPIO_BASE = 224
    #INT_GPIO_BASE = 192
    LP_GPIO_BASE = 160
    RST_GPIO_BASE = 128
    GPIO_OFFSET = 0

    BASE_DIR_PATH = "/sys/class/gpio/gpio{0}/direction"
    BASE_VAL_PATH = "/sys/class/gpio/gpio{0}/value"

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
        self.ABS_GPIO_BASE = 224 + self.GPIO_OFFSET
        self.LP_GPIO_BASE = 160 + self.GPIO_OFFSET
        self.RST_GPIO_BASE = 128 + self.GPIO_OFFSET
        return True

    def __init__(self):
        # Update abs, lpmode, and reset gpio base
        self.set_gpio_offset()
        self.update_gpio_base()

        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(self.port_start, self.port_end + 1):
            self._port_to_eeprom_mapping[x] = eeprom_path.format(x + self.EEPROM_OFFSET)

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            abs_device_file = self.BASE_VAL_PATH.format(
                port_num + self.ABS_GPIO_BASE)
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
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                port_num + self.LP_GPIO_BASE)
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
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            lpmode_val_device_file = self.BASE_VAL_PATH.format(
                port_num + self.LP_GPIO_BASE)
            val_file = open(lpmode_val_device_file, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        val_file.write("1" if lpmode is True else "0")
        val_file.close()

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reset_val_device_file = self.BASE_VAL_PATH.format(
                port_num + self.RST_GPIO_BASE)
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
                port_num + self.RST_GPIO_BASE)
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

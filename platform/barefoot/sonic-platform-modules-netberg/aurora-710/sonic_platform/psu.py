try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common.logger import Logger
    from sonic_platform.fan import Fan
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

VOLTAGE_HIGH_THRESHOLD = 13.4
VOLTAGE_LOW_THRESHOLD = 10.2
TEMPERATURE_HIGH_THRESHOLD = 70.0
MAX_SUPPLIED_POWER = 550.0

GPIO_OFFSET = 9984

PSU_SYS_FS = "/sys/class/hwmon/hwmon{}/"

PSU_EXIST_INPOWER = "/sys/class/gpio/gpio{}/value"

PSU_EEPROM = "/sys/bus/i2c/devices/{}-0050/eeprom"

logger = Logger('sonic-platform-psu')


class Psu(PsuBase):

    __name_of_psus = ['PSU1', 'PSU2']

    def __init__(self, index):
        PsuBase.__init__(self)

        self.__index = index

        # Presence: PSU1: GPIO_OFFSET+100; PSU2: GPIO_OFFSET+97
        self.__psu_presence_attr = PSU_EXIST_INPOWER.format(
            GPIO_OFFSET + 100 - 3 * self.__index)

        # Powerstatus: PSU1: GPIO_OFFSET+99; PSU2: GPIO_OFFSET+96
        self.__psu_inpower_status_attr = PSU_EXIST_INPOWER.format(
            GPIO_OFFSET + 99 - 3 * self.__index)

        psun_sys_fs = PSU_SYS_FS.format(8 + self.__index)

        # /sys/bus/i2c/devices/i2c-57/57-0058/hwmon/hwmon8/in2_input
        self.__psu_voltage_out_attr = psun_sys_fs + "in2_input"

        # /sys/bus/i2c/devices/i2c-57/57-0058/hwmon/hwmon8/curr2_input
        self.__psu_current_out_attr = psun_sys_fs + "curr2_input"

        # /sys/bus/i2c/devices/i2c-57/57-0058/hwmon/hwmon8/power2_input
        self.__psu_power_out_attr = psun_sys_fs + "power2_input"

        # /sys/bus/i2c/devices/i2c-57/57-0058/hwmon/hwmon8/temp2_input
        self.__psu_temperature_attr = psun_sys_fs + "temp2_input"

        self.__psu_eeprom_attr = PSU_EEPROM.format(57 + index)

        self._fan_list.append(Fan(index=self.__index, psu_fan=True))

    def __eeprom_read_bytes(self, start, end):
        try:
            val = open(self.__psu_eeprom_attr, "rb").read()[start:end]
        except Exception:
            val = None
        return val.decode('ascii')

    def __get_attr_value(self, filepath):
        try:
            with open(filepath, 'r') as fd:
                # text
                data = fd.readlines()
                return data[0].rstrip('\r\n')
        except FileNotFoundError:
            logger.log_error(f"File {filepath} not found.  Aborting")
        except (OSError, IOError) as ex:
            logger.log_error("Cannot open - {}: {}".format(filepath, repr(ex)))

        return 'ERR'
##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__name_of_psus[self.__index]

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = self.__psu_presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if attr_rv != 'ERR':
            if attr_rv == "1":
                presence = True
        else:
            raise SyntaxError

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.__eeprom_read_bytes(22, 34)

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return self.__eeprom_read_bytes(50, 61)

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.__eeprom_read_bytes(46, 48)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        inpower = False
        attr_rv = self.__get_attr_value(self.__psu_inpower_status_attr)
        if attr_rv != 'ERR':
            if attr_rv == "1":
                inpower = True

        return self.get_presence() and inpower

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

##############################################
# PSU methods
##############################################

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        attr_path = self.__psu_voltage_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if attr_rv != 'ERR':
            voltage_out = float(attr_rv) / 1000
        else:
            raise SyntaxError

        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        attr_path = self.__psu_current_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if attr_rv != 'ERR':
            current_out = float(attr_rv) / 1000
        else:
            raise SyntaxError

        return current_out

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        attr_path = self.__psu_power_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if attr_rv != 'ERR':
            power_out = float(attr_rv) / 1000000
        else:
            raise SyntaxError

        return power_out

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        attr_rv = self.__get_attr_value(self.__psu_temperature_attr)
        if attr_rv != 'ERR':
            temperature = float(attr_rv) / 1000
        else:
            raise SyntaxError

        return temperature

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        powergood_status = False
        voltage_out = self.get_voltage()

        # Check the voltage out with 12V, plus or minus 20 percentage.
        if self.get_voltage_low_threshold() <= voltage_out <= self.get_voltage_high_threshold():
            powergood_status = True

        return powergood_status

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return VOLTAGE_HIGH_THRESHOLD

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return VOLTAGE_LOW_THRESHOLD

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return TEMPERATURE_HIGH_THRESHOLD

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return MAX_SUPPLIED_POWER

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        value = 0
        # PSU1: mask = 8
        # PSU2: mask = 16
        mask = 8 + 8 * self.__index

        if color == self.STATUS_LED_COLOR_GREEN:
            value = 0x00
        elif color == self.STATUS_LED_COLOR_AMBER:
            value = 0xFF
        else:
            logger.log_error(
                "Invalid Parameters. LED Color {} doesn't support".format(color))
            return False

        ret_val, log = subprocess.getstatusoutput(
            "i2cset -m {} -y -r 50 0x75 2 {}".format(mask, value))

        if ret_val != 0:
            logger.log_error("Unable set PSU{} color with following i2cset output {} ".format(
                self.__index + 1, log))
            return False

        return True

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        raise NotImplementedError

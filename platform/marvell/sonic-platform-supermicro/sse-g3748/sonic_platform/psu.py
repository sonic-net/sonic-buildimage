#######################################################################
# Supermicro SSE-G3748
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    import os
    import sys
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

sonic_logger = logger.Logger('psu')

class Psu(PsuBase):
    """Supermicro platform-specific PSU class for SSE-G3748 """
    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.PSU_OUTPUT_VOLTAGE_MIN = 11
        self.PSU_OUTPUT_VOLTAGE_MAX = 13
        G3748CPLD_I2C_DIR = "/sys/bus/i2c/devices/0-0066/"
        # PSU is 1-based in SMC G3748 platforms
        self.index = psu_index + 1
        self._fan_list = []

        self.get_psu_voltage_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/in{}_input".format(self.index)
        self.get_psu_status_reg = G3748CPLD_I2C_DIR+"psu_status".format()
        self.psu_frontled_reg = G3748CPLD_I2C_DIR+"psu_frontled".format()

        # PSU eeprom
        self.eeprom = Eeprom(is_psu=True, psu_index=self.index)

    def _read_sysfs_file(self, sysfs_file):
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if (int(self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if (int(self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """

        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 1
            if psustatus == 1:
                return True
        if self.index == 2:
            psustatus = int(psustatus) & 2
            if psustatus == 2:
                return True

        return False

    def get_model(self):
        """
        Retrieves the model of the PSU

        Returns:
            string: Model name of PSU.
                    Part number of PSU if model name is not available.
        """
        if self.eeprom.modelstr() == "NA":
            return self.eeprom.part_number_str()
        else:
            return self.eeprom.modelstr()


    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        return self.eeprom.serial_number_str()


    def get_part_number(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return self.eeprom.part_number_str()

    def get_revision(self):
        """
        Retrieves the hardware revision number of the PSU

        Returns:
            string: Revision number of PSU
        """
        return self.eeprom.revision_str()


    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """

        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 4
            if psustatus == 4:
                return False
        if self.index == 2:
            psustatus = int(psustatus) & 8
            if psustatus == 8:
                return False

        return True

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 4
            if psustatus == 0:
                psu_voltage = 12.0
                return psu_voltage
        if self.index == 2:
            psustatus = int(psustatus) & 8
            if psustatus == 0:
                psu_voltage = 12.0
                return psu_voltage

        psu_voltage = 0.0
        return psu_voltage

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """

        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 4
            if psustatus == 0:
                return True
        if self.index == 2:
            psustatus = int(psustatus) & 8
            if psustatus == 0:
                return True

        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.get_powergood_status():
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_OFF

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        # The firmware running in the PSU controls the LED
        # and the PSU LED state cannot be changed from CPU.

        psuled = self._read_sysfs_file(self.psu_frontled_reg)

        if color == self.STATUS_LED_COLOR_GREEN:
            if self.index == 1:
                psuled = int(psuled) & 0x0C | 0x02
            if self.index == 2:
                psuled = int(psuled) & 0x03 | 0x08
        if color == self.STATUS_LED_COLOR_RED:
            if self.index == 1:
                psuled = int(psuled) & 0x0C | 0x01
            if self.index == 2:
                psuled = int(psuled) & 0x03 | 0x04
        if color == self.STATUS_LED_COLOR_OFF:
            if self.index == 1:
                psuled = int(psuled) & 0x0C
            if self.index == 2:
                psuled = int(psuled) & 0x03

        self._write_sysfs_file(self.psu_frontled_reg, psuled)

        return True

    def get_status_master_led(self):
        """
        Gets the state of the front panel PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if (not os.path.isfile("/sys/class/gpio/psuLedGreen/value") or
            not os.path.isfile("/sys/class/gpio/psuLedAmber/value")):
            return None

        green = self._read_sysfs_file("/sys/class/gpio/psuLedGreen/value")
        amber = self._read_sysfs_file("/sys/class/gpio/psuLedAmber/value")
        if green == "ERR" or amber == "ERR":
            return None
        if green == "1":
            return self.STATUS_LED_COLOR_GREEN
        elif amber == "1":
            return self.STATUS_LED_COLOR_AMBER
        else:
            return None

    def set_status_master_led(self, color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        if (not os.path.isfile("/sys/class/gpio/psuLedGreen/value") or
            not os.path.isfile("/sys/class/gpio/psuLedAmber/value")):
            return False

        if color == self.STATUS_LED_COLOR_GREEN:
            rvg = self._write_sysfs_file("/sys/class/gpio/psuLedGreen/value", 1)
            if rvg != "ERR":
                rva = self._write_sysfs_file("/sys/class/gpio/psuLedAmber/value", 0)
        elif color == self.STATUS_LED_COLOR_AMBER:
            rvg = self._write_sysfs_file("/sys/class/gpio/psuLedGreen/value", 0)
            if rvg != "ERR":
                rva = self._write_sysfs_file("/sys/class/gpio/psuLedAmber/value", 1)
        else:
            return False

        if rvg == "ERR" or rva == "ERR":
            return False

        return True

    def get_power(self):
        """
        Retrieves the output power of the PSU

        Returns:
            string: output power of PSU
        """
        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 4
            if psustatus == 0:
                psu_power = 150
                return psu_power
        if self.index == 2:
            psustatus = int(psustatus) & 8
            if psustatus == 0:
                psu_power = 150
                return psu_power

        psu_power = 0
        return psu_power

    def get_current(self):
        """
        Retrieves the output current of the PSU

        Returns:
            string: output current of PSU
        """
        psustatus = self._read_sysfs_file(self.get_psu_status_reg)

        if self.index == 1:
            psustatus = int(psustatus) & 4
            if psustatus == 0:
                psu_current = 12.5
                return psu_current
        if self.index == 2:
            psustatus = int(psustatus) & 8
            if psustatus == 0:
                psu_current = 12.5
                return psu_current

        psu_current = 0.0
        return psu_current

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MAX)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MIN)



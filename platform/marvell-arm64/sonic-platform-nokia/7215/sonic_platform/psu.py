########################################################################
# Nokia IXS7215
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    import os
    import time
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sonic_logger = logger.Logger('psu')
INA230_DIR = "/sys/bus/i2c/devices/0-0040/iio:device0/"
CPLD_DIR = "/sys/bus/i2c/devices/0-0041/"

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7215 """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        

        # PSU eeprom
        self.eeprom = Eeprom(is_psu=True, psu_index=self.index)

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
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
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if ((self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if ((self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv
    
    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """  
        active_psus = 0
        psu1_good = self._read_sysfs_file(CPLD_DIR+"psu1_power_good")
        psu2_good = self._read_sysfs_file(CPLD_DIR+"psu2_power_good")

        active_psus = int(psu1_good) + int(psu2_good)
        
        return active_psus

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
        #Fixed PSU's on 7215-IXS-A1, Always return True

        return True

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return self.eeprom.modelstr()

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        return self.eeprom.serial_number_str()

    def get_revision(self):
        """
        Retrieves the HW revision of the PSU

        Returns:
            string: HW revision of PSU
        """
        return self.eeprom.part_number_str()

    def get_part_number(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return self.eeprom.part_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        psu_sysfs_str=CPLD_DIR+"psu{}_power_good".format(self.index)
        psu_status = self._read_sysfs_file(psu_sysfs_str)

        if psu_status == '1':
            return True

        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if(self.get_status()):
            psu_voltage = self._read_sysfs_file(INA230_DIR+"in_voltage1_raw")
        else:
            psu_voltage = 0.0
        
        psu_voltage = (float(psu_voltage)*1.25)/1000

        return psu_voltage
    
    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        num_psus = self._get_active_psus()
        if(self.get_status()):
            psu_current = self._read_sysfs_file(INA230_DIR+"in_current3_raw")
            psu_current = ((float(psu_current)*0.5)/1000)/num_psus
        else:
            psu_current = 0.0

        return psu_current
    
    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_voltage = self.get_voltage()
        psu_current = self.get_current()
        psu_power = psu_voltage * psu_current

        return psu_power

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
        return False

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        psu_sysfs_str=CPLD_DIR+"psu{}_power_good".format(self.index)
        psu_pg_status = self._read_sysfs_file(psu_sysfs_str)

        if psu_pg_status == '1':
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
        # No Individual Status LED for 7215-IXS-A1
        return False

    def get_status_master_led(self):
        """
        Gets the state of the front panel PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        psu_led = self._read_sysfs_file(CPLD_DIR+"psu_led")

        if psu_led == "green":
            return self.STATUS_LED_COLOR_GREEN
        elif psu_led == "amber":
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
        if color == self.STATUS_LED_COLOR_GREEN:
            status = self._write_sysfs_file(CPLD_DIR+"psu_led", "green")
        elif color == self.STATUS_LED_COLOR_AMBER:
            status = self._write_sysfs_file(CPLD_DIR+"psu_led", "amber")
        else:
            return False

        if status == "ERR":
            return False

        return True
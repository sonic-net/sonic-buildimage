#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common.logger import Logger
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = 'thermalctld'
logger = Logger(SYSLOG_IDENTIFIER)

# To do: should be defined in platDev
PSU_MAX_VOUT = 12.0  # voltage
PSU_MIN_VOUT = 3.3  # voltage
PSU_MAX_TEMP = 50.0  # C


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, index, info_list, is_bmc):
        PsuBase.__init__(self)
        self.index = index
        self.is_bmc = is_bmc
        self.attr_path = info_list[0]
        self.status_path = info_list[1]
        if is_bmc:
            speed_file = self.attr_path + 'psu{}_fan_speed'.format(index+1)
        else:
            speed_file = self.attr_path + 'psu_fan_speed_1'

        fan = Fan(index, 0, [self.status_path, speed_file], True)
        self._fan_list.append(fan)
        self.psu_name = "PSU{}".format(self.index+1)

    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath, 'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except FileNotFoundError:
            logger.log_error(f"File {filepath} not found.  Aborting")
        except OSError as ex:
            logger.log_error("Cannot open - {}: {}".format(filepath, repr(ex)))

        return None

    def get_name(self):
        return self.psu_name

    def get_presence(self):
        """
        Retrieves the presence status of power supply unit (PSU) defined
        Returns:
            bool: True if PSU is present, False if not
        """
        data = self.__read_attr_file(
            self.status_path + 'psu{}_prnt'.format(self.index+1))
        if data == '1':
            return True
        else:
            return False

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        data = self.__read_attr_file(
            self.status_path + 'psu{}_good'.format(self.index+1))
        if data == '1':
            return True
        else:
            return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts, 
            e.g. 12.1 
        """
        if self.is_bmc:
            path = self.attr_path + 'psu{}_vout'.format(self.index+1)
        else:
            path = self.attr_path + "/psu_vout"
        vout = self.__read_attr_file(path, 0)
        if vout is not None:
            return float(vout) / 1000

        return False

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        if self.is_bmc:
            path = self.attr_path + 'psu{}_iout'.format(self.index+1)
        else:
            path = self.attr_path + "/psu_iout"
        iout = self.__read_attr_file(path, 0)
        if iout is not None:
            return float(iout) / 1000
        return False

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        if self.is_bmc:
            path = self.attr_path + 'psu{}_pout'.format(self.index+1)
        else:
            path = self.attr_path + "/psu_pout"
        pout = self.__read_attr_file(path, 0)
        if pout is not None:
            return float(pout) / 1000000
        return False

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        raise NotImplementedError

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        if self.is_bmc:
            path = self.attr_path+'psu{}_temp'.format(self.index+1)
        else:
            path = self.attr_path + "/psu_temp_1"
        temperature = self.__read_attr_file(path, 0)
        if temperature is not None:
            return float(temperature) / 1000

        return False

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return PSU_MAX_TEMP

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts, 
            e.g. 12.1 
        """
        return PSU_MAX_VOUT

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts, 
            e.g. 12.1 
        """
        return PSU_MIN_VOUT

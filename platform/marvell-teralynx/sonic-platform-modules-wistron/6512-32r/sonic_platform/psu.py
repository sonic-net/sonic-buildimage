#!/usr/bin/env python

#############################################################################
# psuutil.py
# Platform-specific PSU status interface for SONiC
#############################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NAME_LIST = ["PSU-1", "PSU-2"]

class Psu(PsuBase):
    """Platform-specific Psu class"""

    SYSFS_PSU_DIR = ["/sys/bus/i2c/devices/0-005a",
                     "/sys/bus/i2c/devices/0-0059"]

    def __init__(self, psu_index):
        self._fan_list = []
        self.PSU_TEMP_MAX = 85 * 1000
        self.PSU_OUTPUT_POWER_MAX = 1300 * 1000
        self.PSU_OUTPUT_VOLTAGE_MIN = 11400
        self.PSU_OUTPUT_VOLTAGE_MAX = 12600
        self.index = psu_index
        PsuBase.__init__(self)
        self.__initialize_fan()

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        for fan_index in range(0, 1):
                fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
                self._fan_list.append(fan)

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return ""


    def get_fan(self, index):
        """
        Retrieves object representing the fan module contained in this PSU
        Returns:
            An object dervied from FanBase representing the fan module
            contained in this PSU
        """
        return self._fan_list[index]

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # Hardware not supported
        return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        attr_file ='present'
        attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
        status = 0
        try:
            with open(attr_path, 'r') as psu_prs:
                status = int(psu_prs.read())
        except IOError:
            return False

        return status == 1

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        attr_file = 'power_good'
        attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
        status = 0
        try:
            with open(attr_path, 'r') as power_status:
                status = int(power_status.read())
        except IOError:
            return False

        return status == 1

    def get_model(self):
        """
        Retrieves the model number/name of a power supply unit (PSU) defined
                by 1-based index <idx>
        :param idx: An integer, 1-based index of the PSU of which to query model number
        :return: String, denoting model number/name
        """
        try:
            if self.get_presence():
                attr_file = 'model'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return str(val)
        except Exception as e:
            return None

    def get_mfr_id(self):
        """
        Retrieves the manufacturing id of a power supply unit (PSU) defined
                by 1-based index <idx>
        :param idx: An integer, 1-based index of the PSU of which to query mfr id
        :return: String, denoting manufacturing id
        """
        try:
            if self.get_presence():
                attr_file = 'vendor'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return str(val)
        except Exception as e:
            return None

    def get_serial(self):
        """
        Retrieves the serial number of a power supply unit (PSU) defined
                by 1-based index <idx>
        :param idx: An integer, 1-based index of the PSU of which to query serial number
        :return: String, denoting serial number of the PSU unit
        """
        try:
            if self.get_presence():
                attr_file = 'sn'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return str(val)
        except Exception as e:
            return None

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A int number, the output voltage in volts.
        """
        try:
            if self.get_presence():
                attr_file = 'in2_input'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return float(val) / 1000.0
        except Exception as e:
            return None

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A int number, electric current in amperes
        """
        try:
            if self.get_presence():
                attr_file = 'curr2_input'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return float(val) / 1000.0
        except Exception as e:
            return None

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A int number, the power in watts.
        """
        try:
            if self.get_presence():
                attr_file = 'power2_input'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return float(val) / 1000000.0
        except Exception as e:
            return None

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        # TODO
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_RED
        else:
            return None

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        try:
            if self.get_presence():
                attr_file = 'temp1_input'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                val = self.__read_txt_file(attr_path)
                return float(val) / 1000.0
        except Exception as e:
            return None

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return float(self.PSU_TEMP_MAX/1000)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MAX/1000)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return float(self.PSU_OUTPUT_VOLTAGE_MIN/1000)

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return float(self.PSU_OUTPUT_POWER_MAX/1000)

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index + 1

    def is_replaceable(self):
        return True

    def get_revision(self):
        """
        Retrieves the hardware revision of the device
        Returns:
            string: Revision value of device
        """
        try:
            if self.get_presence():
                attr_file = 'rev'
                attr_path = self.SYSFS_PSU_DIR[self.index] +'/' + attr_file
                with open(attr_path, 'r') as revision:
                    val = revision.read()
                return val.strip()
        except IOError:
            return None

    def get_num_fans(self):
        """
        Retrieves the number of fan modules available on this PSU

        Returns:
            An integer, the number of fan modules available on this PSU
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this PSU

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this PSU
        """
        return self._fan_list

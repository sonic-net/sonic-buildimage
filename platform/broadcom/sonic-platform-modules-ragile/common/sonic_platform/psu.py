#!/usr/bin/env python3
########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################


try:
    import time
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


class Psu(PsuBase):
    """Platform-specific PSU class"""

    def __init__(self, interface_obj, index):
        self.psu_dict = {}
        self.psu_status_dict = {}
        self.psu_power_dict = {}
        self._fan_list = []
        self._thermal_list = []
        self.int_case = interface_obj
        self.index = index
        self.name = "PSU" + str(index)

        self.psu_dict_update_time = 0
        self.psu_status_dict_update_time = 0
        self.psu_power_dict_update_time = 0

        self._fan_list.append(Fan(self.int_case, 1, 1, psu_fan=True, psu_index=index))

    def psu_dict_update(self):
        local_time = time.time()
        if not self.psu_dict or (local_time - self.psu_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_dict_update_time = local_time
            self.psu_dict = self.int_case.get_psu_fru_info(self.name)

    def psu_status_dict_update(self):
        local_time = time.time()
        if not self.psu_status_dict or (
                local_time - self.psu_status_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_status_dict_update_time = local_time
            self.psu_status_dict = self.int_case.get_psu_status(self.name)

    def psu_power_dict_update(self):
        local_time = time.time()
        if not self.psu_power_dict or (
                local_time - self.psu_power_dict_update_time) >= 1:  # update data every 1 seconds
            self.psu_power_dict_update_time = local_time
            self.psu_power_dict = self.int_case.get_psu_power_status(self.name)

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "Psu{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        return self.int_case.get_psu_presence(self.name)

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        self.psu_dict_update()
        return self.psu_dict["DisplayName"]

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        self.psu_dict_update()
        return self.psu_dict["SN"]

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        return self.int_case.get_psu_input_output_status(self.name)

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Outputs"]["Voltage"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Outputs"]["Current"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Outputs"]["Power"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        return self.int_case.get_psu_input_output_status(self.name)

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence():
            return "N/A"
        if self.int_case.get_psu_input_output_status(self.name):
            return self.STATUS_LED_COLOR_GREEN
        return self.STATUS_LED_COLOR_RED

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
        # not supported
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        self.psu_status_dict_update()
        value = self.psu_status_dict["Temperature"]["Value"]
        if value is None:
            value = 0
        return round(float(value), 1)

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.psu_status_dict_update()
        value = self.psu_status_dict["Temperature"]["Max"]
        if value is None:
            value = 0
        return round(float(value), 1)

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        self.psu_power_dict_update()
        value = self.psu_power_dict["Outputs"]["Voltage"]["HighAlarm"]
        if value is None:
            value = 0
        return round(float(value), 1)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        self.psu_power_dict_update()
        value = self.psu_power_dict["Outputs"]["Voltage"]["LowAlarm"]
        if value is None:
            value = 0
        return round(float(value), 1)

    def get_input_voltage(self):
        """
        Get the input voltage of the PSU

        Returns:
            A float number, the input voltage in volts,
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Inputs"]["Voltage"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_input_current(self):
        """
        Get the input electric current of the PSU

        Returns:
            A float number, the input current in amperes, e.g 220.3
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Inputs"]["Current"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_input_power(self):
        """
        Get the input current energy of the PSU

        Returns:
            A float number, the input power in watts, e.g. 302.6
        """
        self.psu_status_dict_update()
        if self.psu_status_dict["InputStatus"] is False:
            value = 0
        else:
            self.psu_power_dict_update()
            value = self.psu_power_dict["Inputs"]["Power"]["Value"]
            if value is None:
                value = 0
        return round(float(value), 1)

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        self.psu_dict_update()
        return self.psu_dict["HW"]

    def get_vendor(self):
        """
        Retrieves the vendor name of the psu

        Returns:
            string: Vendor name of psu
        """
        self.psu_dict_update()
        return self.psu_dict["VENDOR"]

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return False

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        return False

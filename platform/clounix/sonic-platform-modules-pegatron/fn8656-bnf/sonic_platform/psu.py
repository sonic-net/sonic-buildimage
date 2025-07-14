#!/usr/bin/env python
#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    #import os
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Psu(PsuBase):

    def __init__(self, index, psu_conf):
        self.__num_of_fans = len(psu_conf[index]['fans'])
        self.__num_of_thermals = len(psu_conf[index]['thermals'])
        self.__index = index
        self.__conf = psu_conf
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = '/sys/switch/psu/psu{}/'.format(self.__index + 1)

        self._fan_list = []
        self._thermal_list = []
        PsuBase.__init__(self)
        # Initialize PSU FAN
        fan_conf=self.__conf[self.__index]['fans']
        for x in range(0, self.__num_of_fans):
            fan_conf[x].update({'container':'psu'})
            fan_conf[x].update({'container_index': self.__index})
            fan = Fan(x, fan_conf)
            self._fan_list.append(fan)

        # Initialize PSU THERMAL
        thermal_conf=self.__conf[self.__index]['thermals']
        for x in range(0, self.__num_of_thermals):
            thermal_conf[x].update({'container':'psu'})
            thermal_conf[x].update({'container_index': self.__index})
            thermal = Thermal(x,thermal_conf)
            self._thermal_list.append(thermal)

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'status')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if (attr_rv):
                presence = True
        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = 'N/A'

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'model_name')
        if (attr_rv != None):
            model = attr_rv

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = 'N/A'

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'serial_number')
        if (attr_rv != None):
            serial = attr_rv

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'status')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if(attr_rv == 0x1):
                status = True
        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.__index + 1

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_date(self):
        date = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'date')
        if (attr_rv != None):
            date = attr_rv
        return date

    def get_vendor(self):
        vendor = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'vendor')
        if (attr_rv != None):
            vendor = attr_rv
        return vendor

    def get_hw_version(self):
        hw_version = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'hardware_version')
        if (attr_rv != None):
            hw_version = attr_rv
        return hw_version

    def get_alarm(self):
        alarm = 'Exception: '
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'alarm')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if(attr_rv == 0x0):
                alarm = 'Normal'
                return alarm
            elif alarm & 0x01:
                alarm +=  ' Temperature'
            elif alarm & 0x02:
                alarm +=  ' Fan'
            elif alarm & 0x04:
                alarm +=  ' Voltage'

        return alarm

    def get_alarm_threshold_curr(self):
        alarm_threshold_curr = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'alarm_threshold_curr')
        if (attr_rv != None):
            alarm_threshold_curr = attr_rv
        return alarm_threshold_curr

    def get_alarm_threshold_vol(self):
        alarm_threshold_vol = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'alarm_threshold_vol')
        if (attr_rv != None):
            alarm_threshold_vol = attr_rv
        return alarm_threshold_vol

    def get_part_number(self):
        part_number = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'part_number')
        if (attr_rv != None):
            part_number = attr_rv
        return part_number
  
    def get_in_current(self):
        in_curr = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'in_curr')
        if (attr_rv != None):
            in_curr = attr_rv
        return in_curr
    def get_in_voltage(self):
        in_vol = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'in_vol')
        if (attr_rv != None):
            in_vol = attr_rv
        return in_vol
    def get_in_power(self):
        in_power = 'N/A'
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'in_power')
        if (attr_rv != None):
            in_power = attr_rv
        return in_power
              
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
        voltage_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_vol')
        if (attr_rv != None):
            voltage_out = float(attr_rv)

        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_curr')
        if (attr_rv != None):
            current_out = float(attr_rv)

        return current_out

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'out_power')
        if (attr_rv != None):
            power_out = float(attr_rv)

        return power_out

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
        color = "off"

        if self.get_presence() is False :
            return color

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'led_status')
        if (attr_rv != None):
            if (int(attr_rv, 16) == 0x1 or int(attr_rv, 16) == 0x4) :
                color = "green"
            elif (int(attr_rv, 16) == 0x3 or int(attr_rv, 16) == 0x6) :
                color = "red"

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'status')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if(attr_rv == 0x2):
                color = "red"
        return color

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_high_threshold = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'max_output_vol')
        if (attr_rv != None):
            voltage_high_threshold = float(attr_rv)

        return voltage_high_threshold

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        voltage_low_threshold = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'min_output_vol')
        if (attr_rv != None):
            voltage_low_threshold = float(attr_rv)

        return voltage_low_threshold

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        max_power_out = 0.0

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'max_output_power')
        if (attr_rv != None):
            max_power_out = float(attr_rv)

        return max_power_out

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        if self.get_presence():
            return self.get_thermal(0).get_high_threshold()
        else:
            return 0.0

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.get_presence():
            return self.get_thermal(0).get_temperature()
        else:
            return 0.0


        

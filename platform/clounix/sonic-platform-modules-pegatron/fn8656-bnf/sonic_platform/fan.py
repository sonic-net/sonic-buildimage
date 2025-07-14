#!/usr/bin/env python
#
# Name: fan.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Fan(FanBase):

    def __init__(self, index, fan_conf):
        self.__index = index
        self.__conf = fan_conf
        self.__api_helper = APIHelper()
        self.__attr_path_prefix = ''
        self.__is_psu_fan = False
        FanBase.__init__(self)

        if fan_conf[self.__index]['container'] == 'psu':
            self.__attr_path_prefix = '/sys/switch/psu/psu{}/'.format(fan_conf[self.__index]['container_index'] + 1)
            self.__is_psu_fan = True
        elif fan_conf[self.__index]['container'] == 'fan_drawer':
            self.__attr_path_prefix = '/sys/switch/fan/fan{}/'.format(self.__index+1)
        else:
            #chassis fan to be done
            self.__attr_path_prefix = '/sys/switch/fan/fan{}/'.format(self.__index+1)

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
            if (attr_rv == 0x1):
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

    def get_vendor(self):
        vendor = 'N/A'
        
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'vendor')
        if (attr_rv != None):
            vendor = attr_rv
        return vendor

    def get_part_number(self):
        part_num = 'N/A'
        
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'part_number')
        if (attr_rv != None):
            part_num = attr_rv
        return part_num
    
    def get_hw_version(self):
        hw_version = 'N/A'
        
        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'hardware_version')
        if (attr_rv != None):
            hw_version = attr_rv
        return hw_version

##############################################
# FAN methods
##############################################

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_EXHAUST

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'direction')
        if (attr_rv != None):
            attr_rv = int(attr_rv)
            if attr_rv == 0:
                direction = self.FAN_DIRECTION_EXHAUST
            else:
                direction = self.FAN_DIRECTION_INTAKE

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        if self.__is_psu_fan :
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'fan_speed')
        else:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'motor0/speed')
        
        if (attr_rv != None):
            attr_rv = int(attr_rv) 
            if (attr_rv >= 0):
                speed = attr_rv

        return speed

    def get_ratio(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        ratio = 0
        if self.__is_psu_fan :
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'fan_speed')
        else:
            attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'motor0/ratio')
        
        if (attr_rv != None):
            attr_rv = int(attr_rv) 
            if (attr_rv >= 0):
                ratio = attr_rv

        return ratio

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 100

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        raise NotImplementedError

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        ret_val = False
        if color == self.STATUS_LED_COLOR_GREEN:
            led_value = 1
        elif color == self.STATUS_LED_COLOR_RED:
            led_value = 3
        else:
            return False
        ret_val = self.__api_helper.write_txt_file(self.__attr_path_prefix + 'led_status',led_value)
        
        return ret_val

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        color = self.STATUS_LED_COLOR_OFF
        if self.get_presence() is False :
            return color

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'led_status')
        if (attr_rv != None):
            if (int(attr_rv, 16) == 0x1 or int(attr_rv, 16) == 0x4) :
                color = self.STATUS_LED_COLOR_GREEN
            elif (int(attr_rv, 16) == 0x3 or int(attr_rv, 16) == 0x6) :
                color = self.STATUS_LED_COLOR_RED
        
        if self.__is_psu_fan :
            if self.get_status() is False:
                    color = self.STATUS_LED_COLOR_RED

        return color



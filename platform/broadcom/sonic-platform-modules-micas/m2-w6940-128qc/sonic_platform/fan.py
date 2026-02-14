#!/usr/bin/env python3
########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################

try:
    import time
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e

PSU_NOT_PRESENCE = 0x01

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, interface_obj, fantray_index, fan_index, psu_fan=False, psu_index=0):
        self.restful = interface_obj
        self.fantray_index = fantray_index
        self.fan_index = fan_index
        self.psu_index = psu_index
        self.is_psu_fan = psu_fan
        if not self.is_psu_fan:
            self.name = "fan" + str(fantray_index)
        else:
            self.name = "psu" + str(psu_index)

        self.led_map = {
            "0": self.STATUS_LED_COLOR_OFF,
            "1": self.STATUS_LED_COLOR_GREEN,
            "2": self.STATUS_LED_COLOR_AMBER,
            "3": self.STATUS_LED_COLOR_RED
        }

    def fan_dict_update(self):
        if not self.is_psu_fan:
            return self.restful.get_fan_info(self.name)
        else:
            return self.restful.get_psu_info(self.name)

    def get_name(self):
        """
        Retrieves the fan name
        Returns:
            string: The name of the device
        """
        if not self.is_psu_fan:
            return "Fantray{}_{}".format(self.fantray_index, self.fan_index)
        return "PSU{}_FAN{}".format(self.psu_index, self.fan_index)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is None:
                return 'N/A'
            return fan_info.get("model_name", 'N/A')
        else:
            return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is None:
                return 'N/A'
            return fan_info.get("serial_number", 'N/A')
        else:
            return 'N/A'

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is None:
                return False
            status = fan_info.get('status', None)
            if status is None:
                return False
            if status == '1' or status == '2':
                return True
            return False
        else:
            psu_info = self.fan_dict_update()
            if psu_info is None:
                return False
            status = psu_info.get('status', None)
            if status is None or status in self.restful.restful_err_str:
                return False
            if int(status, 16) & PSU_NOT_PRESENCE == 0:
                return True
            return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.get_presence():
            return False

        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is not None:
                status = fan_info.get('status', None)
                if status == '1':
                    return True
                else:
                    return False
        else:
            psu_info = self.fan_dict_update()
            if psu_info is None:
                return False
            status = psu_info.get('status', None)
            if status is None or status in self.restful.restful_err_str:
                return False
            status = int(status, 16)
            if status != 0:
                return False
            return True

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

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        fan_info = self.fan_dict_update()
        if fan_info is None:
            return self.FAN_DIRECTION_NOT_APPLICABLE
        if not self.is_psu_fan:
            air_flow = fan_info.get("direction", "2")
        else:
            air_flow = fan_info.get("fan_direction", "2")
        if air_flow == "0":
            return self.FAN_DIRECTION_INTAKE
        elif air_flow == "1":
            return self.FAN_DIRECTION_EXHAUST
        else:
            return self.FAN_DIRECTION_NOT_APPLICABLE

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if not self.get_presence():
            return 0

        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is not None:
                motor = fan_info.get('motor{}'.format(self.fan_index))
                value = motor.get('speed')
                max_speed = motor.get('speed_max')
        else:
            psu_info = self.fan_dict_update()
            if psu_info is not None:
                value = psu_info.get('fan_speed', None)
                max_speed = psu_info.get('fan_speed_max', None)
        if value is None or value in self.restful.restful_err_str:
            return None
        if max_speed is None or max_speed in self.restful.restful_err_str:
            return None
        if isinstance(value, str):
            value = int(value)
            max_speed = int(max_speed)
        pwm = value * 100 / max_speed
        if pwm > 100:
            pwm = 100
        elif pwm < 0:
            pwm = 0
        return int(pwm)

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
        considered tolerable
        """
        # The default tolerance value is fixed as 30%
        return 30

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        return True

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # not supported
        return False

    def get_status_led(self):
        """
        Gets the state of the Fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.is_psu_fan:
            # No LED available for PSU Fan
            return 'N/A'

        if not self.get_presence():
            return 'N/A'

        fan_info = self.fan_dict_update()
        if fan_info is None:
            return 'N/A'

        led_status = fan_info.get('led_status', None)
        if led_status is None:
            return 'N/A'

        return self.led_map.get(led_status, 'N/A')

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        pwm = 0
        if not self.get_presence():
            return 0

        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is not None:
                pwm = fan_info.get('pwm')
        else:
            psu_info = self.fan_dict_update()
            if psu_info is not None:
                pwm = psu_info.get('fan_ratio')
        if pwm is None or pwm in self.restful.restful_err_str:
            return None
        return int(pwm)

    def get_vendor(self):
        """
        Retrieves the vendor name of the fan

        Returns:
            string: Vendor name of fan
        """
        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is None:
                return 'N/A'
            return fan_info.get("vendor", 'N/A')
        else:
            return 'N/A'

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        if not self.is_psu_fan:
            fan_info = self.fan_dict_update()
            if fan_info is None:
                return 'N/A'
            return fan_info.get("part_number", 'N/A')
        else:
            return 'N/A'

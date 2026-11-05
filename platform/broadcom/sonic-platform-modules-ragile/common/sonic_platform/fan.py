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


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, interface_obj, fantray_index, fan_index, psu_fan=False, psu_index=0):
        self.fan_dict = {}
        self.int_case = interface_obj
        self.fantray_index = fantray_index
        self.fan_index = fan_index
        self.psu_index = psu_index
        self.is_psu_fan = psu_fan
        self.update_time = 0
        if not self.is_psu_fan:
            self.name = "FAN" + str(fantray_index)
        else:
            self.name = "PSU" + str(psu_index)

    def fan_dict_update(self):
        local_time = time.time()
        if not self.fan_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            if not self.is_psu_fan:
                self.fan_dict = self.int_case.get_fan_info(self.name)
            else:
                self.fan_dict = self.int_case.get_psu_fru_info(self.name)

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
            self.fan_dict_update()
            return self.fan_dict["NAME"]
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        if not self.is_psu_fan:
            self.fan_dict_update()
            return self.fan_dict["SN"]
        return 'N/A'

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        if not self.is_psu_fan:
            return self.int_case.get_fan_presence(self.name)
        return self.int_case.get_psu_presence(self.name)

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.get_presence():
            return False

        if not self.is_psu_fan:
            fan_dir = {}
            fan_dir = self.int_case.get_fan_info_rotor(self.name)
            # get fan rotor pwm
            rotor_name = "Rotor" + str(self.fan_index)
            value = fan_dir[rotor_name]["Speed"]
            min_speed = fan_dir[rotor_name]["SpeedMin"]
            max_speed = fan_dir[rotor_name]["SpeedMax"]
            tolerance = fan_dir[rotor_name]["Tolerance"]
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            value = psu_status_dict["FanSpeed"]["Value"]
            min_speed = psu_status_dict["FanSpeed"]["Min"]
            max_speed = psu_status_dict["FanSpeed"]["Max"]
            tolerance = psu_status_dict["FanSpeed"]["Tolerance"]

        if isinstance(tolerance, str) or tolerance is None:
            tolerance = 30

        if isinstance(value, str) or value is None:
            return False

        if value < min_speed:
            return False

        speed = int(value * 100 / max_speed)
        if speed > 100:
            speed = 100
        elif speed < 0:
            speed = 0
        target = self.get_target_speed()

        if (speed - target) > target * tolerance / 100:
            return False
        if (target - speed) > target * tolerance / 100:
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
        self.fan_dict_update()
        air_flow = self.fan_dict["AirFlow"]
        if air_flow is not None:
            return air_flow
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
            fan_dir = {}
            fan_dir = self.int_case.get_fan_info_rotor(self.name)
            # get fan rotor pwm
            rotor_name = "Rotor" + str(self.fan_index)
            value = fan_dir[rotor_name]["Speed"]
            max_speed = fan_dir[rotor_name]["SpeedMax"]
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            value = psu_status_dict["FanSpeed"]["Value"]
            max_speed = psu_status_dict["FanSpeed"]["Max"]

        if isinstance(value, str) or value is None:
            return 0
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
        if not self.is_psu_fan:
            fan_dir = {}
            fan_dir = self.int_case.get_fan_info_rotor(self.name)
            # get fan rotor tolerance
            rotor_name = "Rotor" + str(self.fan_index)
            tolerance = fan_dir[rotor_name]["Tolerance"]
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            tolerance = psu_status_dict["FanSpeed"]["Tolerance"]

        if isinstance(tolerance, str) or tolerance is None:
            return 30
        return tolerance

    def fan_set_speed_pwm(self, pwm):
        status = self.int_case.set_fan_speed_pwm(self.name, self.fan_index, pwm)
        if status == -1:
            return False
        return True

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        if not self.is_psu_fan:
            return self.fan_set_speed_pwm(speed)
        return self.int_case.set_psu_fan_speed_pwm(self.name, int(speed))

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

        ret, color = self.int_case.get_fan_led(self.name)
        if ret is True:
            return color
        return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if not self.is_psu_fan:
            # get fan rotor pwm
            pwm = int(self.int_case.get_fan_speed_pwm(self.name, self.fan_index))
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            if psu_status_dict["InputStatus"] is False:
                pwm = 0
            else:
                pwm = self.get_speed()  # target equal to real pwm, to avoid alarm
        return int(pwm)

    def get_vendor(self):
        """
        Retrieves the vendor name of the fan

        Returns:
            string: Vendor name of fan
        """
        if not self.is_psu_fan:
            return "WB"
        return 'N/A'

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        if not self.is_psu_fan:
            self.fan_dict_update()
            return self.fan_dict["HW"]
        return 'N/A'

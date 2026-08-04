#!/usr/bin/env python3
########################################################################
# Ruijie
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################

try:
    import time
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.logger import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(FanBase):
    """Ruijie Platform-specific Fan class"""

    def __init__(self, interface_obj, index, psu_fan=False, psu_index=0):
        self.fan_dict = {}
        self.int_case = interface_obj
        self.index = index
        self.psu_index = psu_index
        self.is_psu_fan = psu_fan

        if not self.is_psu_fan:
            self.name = "FAN" + str(index)
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
            return "Fan{}".format(self.index)
        else:
            return "Psu{}-Fan{}".format(self.psu_index, self.index)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        if not self.is_psu_fan:
            self.fan_dict_update()
            return self.fan_dict["NAME"]
        else:
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
        else:
            return 'N/A'

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        if not self.is_psu_fan:
            return self.int_case.get_fan_presence(self.name)
        else:
            return self.int_case.get_psu_presence(self.name)

    def checkFanRotorStatus(self):
        err_num = 0
        rotor_num = self.int_case.get_fan_rotor_number(self.name)
        for i in range(rotor_num):
            rotor_name = "Rotor" + str(i + 1)
            status = self.int_case.get_fan_rotor_status(self.name, rotor_name)
            if status == False:
                err_num = err_num + 1
        if (err_num == 0):
            return True
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.get_presence():
            return False

        speed = self.get_speed()
        tolerance = self.get_speed_tolerance()
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
        if air_flow == "F2B":
            return self.FAN_DIRECTION_INTAKE
        elif air_flow == "B2F":
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
            fan_dir = {}
            fan_dir = self.int_case.get_fan_info_rotor(self.name)
            # get fan rotor1 pwm
            value = fan_dir["Rotor1"]["Speed"]
            max = fan_dir["Rotor1"]["SpeedMax"]
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            value = psu_status_dict["FanSpeed"]["Value"]
            max = psu_status_dict["FanSpeed"]["Max"]

        if isinstance(value, str) or value is None:
            return 0
        pwm = value * 100 / max
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
        # The default tolerance value is fixed as 30% for all the Ruijie platform
        if not self.is_psu_fan:
            fan_dir = {}
            fan_dir = self.int_case.get_fan_info_rotor(self.name)
            # get fan rotor1 tolerance
            tolerance = fan_dir["Rotor1"]["Tolerance"]
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            tolerance = psu_status_dict["FanSpeed"]["Tolerance"]

        if isinstance(tolerance, str) or tolerance is None:
            return 30
        return tolerance

    def fan_set_speed_pwm(self, pwm):
        err_num = 0
        rotor_num = self.int_case.get_fan_rotor_number(self.name)
        for i in range(rotor_num):
            status = self.int_case.set_fan_speed_pwm(self.name, i + 1, pwm)
            if status == -1:
                err_num = err_num + 1
        if err_num == 0:
            return True
        else:
            return False

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
        else:
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

        ret, color = self.int_case.get_fan_led(self.name)
        if ret is True:
            return color
        else:
            return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if not self.is_psu_fan:
            # get fan rotor1 pwm
            pwm = int(self.int_case.get_fan_speed_pwm(self.name, 0))
        else:
            psu_status_dict = self.int_case.get_psu_status(self.name)
            if psu_status_dict["InputStatus"] is False:
                pwm = 0
            else:
                psu_pwm = int(self.int_case.get_psu_fan_speed_pwm(self.name))
                if psu_pwm == 0:  # psu fan not control
                    pwm = self.get_speed()  # target equal to real pwm, to avoid alarm
                else:
                    pwm = psu_pwm
        return int(pwm)

    def get_vendor(self):
        """
        Retrieves the vendor name of the fan

        Returns:
            string: Vendor name of fan
        """
        if not self.is_psu_fan:
            return "Ruijie"
        else:
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
        else:
            return 'N/A'


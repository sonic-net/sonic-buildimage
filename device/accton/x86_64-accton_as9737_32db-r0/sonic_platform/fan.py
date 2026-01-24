#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    import os.path
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TARGET_SPEED_PATH = "/tmp/fan_target_speed"
PSU_FAN_MAX_RPM = 29952 # PMBus MFR_FAN_SPEED_MAX(0xC3)
FAN_HWMON_PATH = "/sys/devices/platform/as9737_32db_fan/hwmon/hwmon*/fan"
PSU_HWMON_PATH = {
    0: "/sys/devices/platform/as9737_32db_psu.0/hwmon/hwmon*/psu1",
    1: "/sys/devices/platform/as9737_32db_psu.1/hwmon/hwmon*/psu2",
}

fan_list = {
    0: {"name":"FAN-1F", "ss_index":1},
    1: {"name":"FAN-1R", "ss_index":7},
    2: {"name":"FAN-2F", "ss_index":2},
    3: {"name":"FAN-2R", "ss_index":8},
    4: {"name":"FAN-3F", "ss_index":3},
    5: {"name":"FAN-3R", "ss_index":9},
    6: {"name":"FAN-4F", "ss_index":4},
    7: {"name":"FAN-4R", "ss_index":10},
    8: {"name":"FAN-5F", "ss_index":5},
    9: {"name":"FAN-5R", "ss_index":11},
    10: {"name":"FAN-6F", "ss_index":6},
    11: {"name":"FAN-6R", "ss_index":12},
}

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self,
                 fan_tray_index,
                 fan_index=0,
                 is_psu_fan=False,
                 psu_index=0):
        self._api_helper = APIHelper()
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self.psu_index = psu_index

        if self.is_psu_fan:
            self.psu_hwmon_path = PSU_HWMON_PATH[psu_index]
        else:
            self.hwmon_path = FAN_HWMON_PATH + str(fan_list[fan_tray_index * 2 + fan_index]["ss_index"])
        FanBase.__init__(self)

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_NOT_APPLICABLE
        if not self.is_psu_fan:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + "_dir")
            if val is not None:
                direction = self.FAN_DIRECTION_EXHAUST \
                    if val == "F2B" \
                    else self.FAN_DIRECTION_INTAKE
        else:
            val = self._api_helper.glob_read_txt_file(self.psu_hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return self.FAN_DIRECTION_NOT_APPLICABLE

            val = self._api_helper.glob_read_txt_file(self.psu_hwmon_path + "_fan_dir")
            if val is not None:
                    direction = self.FAN_DIRECTION_EXHAUST \
                        if val == "F2B" \
                        else self.FAN_DIRECTION_INTAKE

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        """
        speed = 0
        if self.is_psu_fan:
            psu_fan_speed_rpm = self._api_helper.glob_read_txt_file(
                    self.psu_hwmon_path + "_fan1_input")
            if psu_fan_speed_rpm is not None:
                speed = (int(psu_fan_speed_rpm, 10)) * 100 / PSU_FAN_MAX_RPM
            else:
                return 0
        elif self.get_presence():
            if os.path.isfile(TARGET_SPEED_PATH):
                speed = self._api_helper.read_txt_file(TARGET_SPEED_PATH)
            else:
                fan_input = self._api_helper.glob_read_txt_file(self.hwmon_path + "_input")

                fan_target = self._api_helper.glob_read_txt_file(self.hwmon_path + "_target")

                if fan_input is None or fan_target is None:
                    return 0

                speed = (int(fan_input) * self.get_target_speed()) / int(fan_target) 

        speed = int(speed)
        if speed > 100:
            speed = 100
        return int(speed)

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed_pc = pwm_target/255*100

            0   : when PWM mode is use
            pwm : when pwm mode is not use
        """
        speed = 0
        if self.is_psu_fan:
            fan_speed_rpm = self._api_helper.glob_read_txt_file(
                    self.psu_hwmon_path + "_fan1_input")
            if fan_speed_rpm is not None:
                speed = (int(fan_speed_rpm, 10)) * 100 / PSU_FAN_MAX_RPM
                speed = 100 if (speed > 100) else speed
            else:
                return 0
        elif self.get_presence():
            if os.path.isfile(TARGET_SPEED_PATH):
                speed = self._api_helper.read_txt_file(TARGET_SPEED_PATH)
            else:
                speed = self._api_helper.glob_read_txt_file(self.hwmon_path + '_pwm')
                if speed is None:
                    return 0

        return int(speed)

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 20

        speed = 0
        if not self.is_psu_fan:
            speed = self._api_helper.glob_read_txt_file(self.hwmon_path + '_tolerance')
            if speed is None:
                return 0

        return int(speed)

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        """

        if not self.is_psu_fan and self.get_presence():
            ret = self._api_helper.glob_write_txt_file(self.hwmon_path + '_pwm', speed)
            if ret == True:
                self._api_helper.write_txt_file(TARGET_SPEED_PATH, int(speed))
            return ret

        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False #Not supported

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return {
            True: self.STATUS_LED_COLOR_GREEN,
            False: self.STATUS_LED_COLOR_RED
        }.get(self.get_status(), self.STATUS_LED_COLOR_OFF)

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            val = self._api_helper.glob_read_txt_file(self.psu_hwmon_path + "_present")
        else:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + "_present")

        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            val = self._api_helper.glob_read_txt_file(self.psu_hwmon_path + '_power_good')
            if val is None or int(val, 10)==0:
                return False

            val = self._api_helper.glob_read_txt_file(self.psu_hwmon_path + '_fan1_input')
            if val is not None:
                return int(val, 10)!=0
            else:
                return False
        else:
            val = self._api_helper.glob_read_txt_file(self.hwmon_path + '_fault')
            if val is not None:
                return int(val, 10)==0
            else:
                return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = fan_list[self.fan_tray_index * 2 + self.fan_index]["name"] \
            if not self.is_psu_fan \
            else "PSU-{} FAN-{}".format(self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """

        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return (self.fan_index+1) \
            if not self.is_psu_fan else (self.psu_index+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False


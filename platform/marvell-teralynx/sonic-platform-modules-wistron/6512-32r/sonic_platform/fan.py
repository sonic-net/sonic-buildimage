#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################


import os.path

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SPEED_TOLERANCE = 15
SYSFS_PATH = "/sys/bus/i2c/devices/0-0044"
SYSFS_PSU_DIR = ["/sys/bus/i2c/devices/0-005a",
                 "/sys/bus/i2c/devices/0-0059"]

FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R",
                 "FAN-3F", "FAN-3R", "FAN-4F", "FAN-4R",
                 "FAN-5F", "FAN-5R", "FAN-6F", "FAN-6R",
                 "FAN-7F", "FAN-7R"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan

        if self.is_psu_fan:
            self.psu_index = psu_index

        FanBase.__init__(self)

    def __search_hwmon_dir_name(self, directory):
        try:
            dirs = os.listdir(directory)
            for file in dirs:
                if file.startswith("hwmon"):
                    return file
        except IOError:
            pass
        return ''

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def __write_txt_file(self, file_path, data):
        try:
            with open(file_path, 'w') as fd:
                fd.write(data)
        except IOError:
            pass
        return None

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = 0
        if self.is_psu_fan:
            # psu fan direction should be the same as fan tray
            path= "{}/fan1_direction".format(SYSFS_PATH)
            direction=self.__read_txt_file(path)
            if direction is None:
                return self.FAN_DIRECTION_EXHAUST
        elif self.get_presence():
            path= "{}/fan{}_direction".format(SYSFS_PATH, self.fan_tray_index + 1)
            direction=self.__read_txt_file(path)
            if direction is None:
                return self.FAN_DIRECTION_EXHAUST

        return self.FAN_DIRECTION_EXHAUST if int(direction) == 0 else self.FAN_DIRECTION_INTAKE


    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        """
        speed = 0
        if self.is_psu_fan:
            fan_path="{}/pwm".format(SYSFS_PSU_DIR[self.psu_index])
            speed = self.__read_txt_file(fan_path)
            if speed is not None:
                return int(speed)
            else:
                return 0
        elif self.get_presence():
            path= "{}/pwm".format(SYSFS_PATH)
            speed=self.__read_txt_file(path)
            if speed is None:
                return 0

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
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return SPEED_TOLERANCE

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        """

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
        status=self.get_presence()
        if status is None:
            return  self.STATUS_LED_COLOR_OFF

        return {
            1: self.STATUS_LED_COLOR_GREEN,
            0: self.STATUS_LED_COLOR_RED
        }.get(status, self.STATUS_LED_COLOR_OFF)

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        fan_name = FAN_NAME_LIST[self.fan_tray_index*2 + self.fan_index] \
            if not self.is_psu_fan \
            else "PSU-{} FAN-{}".format(self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """

        if self.is_psu_fan:
            present_path="{}/present".format(SYSFS_PSU_DIR[self.psu_index])
        else:
            present_path="{}/fan{}_present".format(SYSFS_PATH, self.fan_tray_index + 1)

        val=self.__read_txt_file(present_path)
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
            status_path = "{}/power_good".format(SYSFS_PSU_DIR[self.psu_index])
            val=self.__read_txt_file(status_path)
            if val is not None:
                return int(val, 10)==1
            else:
                return False
        else:
            status=self.get_presence()
            if status is None:
                return  False
            return status

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
        return (self.fan_tray_index * 2 + self.fan_index + 1) \
            if not self.is_psu_fan else (self.psu_index + 1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False


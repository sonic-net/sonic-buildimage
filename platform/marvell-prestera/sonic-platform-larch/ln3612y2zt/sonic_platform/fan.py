try:
    import subprocess as cmd
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

_fan_path_patterns = {
    "ALARM":"fan{}_alarm",
    "SPEED":"fan{}_input",
    "MIN":"fan{}_min"
}

_fan_idx_to_controller = {
    1: 1,
    2: 1,
    3: 1,
    4: 2,
    5: 2,
}

_fan_idx_to_controller_idx = {
    1: 1,
    2: 2,
    3: 3,
    4: 1,
    5: 2,
}

_fan_idx_hw_address = {
        1: "2c",
        2: "2c",
        3: "2c",
        4: "2d",
        5: "2d",
}

MAX_SPEED_IN_RPM = 23000

class Fan(FanBase):
    def __init__(self, fan_index=1, fantrayindex = 0, idx_offset=0):
        super().__init__()
        self.fan_index = fan_index
        self.fan_tray_idx = fantrayindex
        self._idx_offset = idx_offset
        self.name = "Fan"+str(fan_index)
        self._api_helper = APIHelper()
        
    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.__get_fan_status() == 0:
            return False
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
        return True

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

    def __generate_path(self, pattern):
        device_addr = _fan_idx_hw_address[self.fan_index]    
        file_path = f"/sys/bus/i2c/devices/13-00{device_addr}/hwmon/hwmon{_fan_idx_to_controller[self.fan_index]}/{_fan_path_patterns[pattern].format(_fan_idx_to_controller_idx[self.fan_index])}"
        return file_path

    def __get_fan_status(self):
        """Checks the status of a specific fan.

        Returns:
            0 if the fan is running, 1 if the fan is failed, None if an error occurs.
        """
        path = self.__generate_path("ALARM")
        return self._api_helper.read_txt_file(path)

    def set_speed(self, speed):
        """
        Sets the speed of all the FANs to a value denoted by the duty-cycle percentage val

        :param speed: An integer, <30-100> denoting FAN duty cycle percentage
        :return: Boolean, True if operation is successful, False if not
        """
        return True;

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        path = self.__generate_path("SPEED")
        speed_in_rpm = float(self._api_helper.read_txt_file(path))
        speed_in_percents = (speed_in_rpm/MAX_SPEED_IN_RPM) * 100
        return int(speed_in_percents)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.__get_fan_status()

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

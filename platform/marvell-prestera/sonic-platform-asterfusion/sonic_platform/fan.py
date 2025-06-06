####################################################################
# Asterfusion CX-N Devices Fan                                     #
#                                                                  #
# Module contains an implementation of SONiC Platform Base API and #
# provides the fan status which are available in the platform      #
#                                                                  #
####################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.fan_base import FanBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_drawer_index, fan_index, num_fan_per_drawer, hwsku, asic):
        # type: (int, int, int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        FanBase.__init__(self)
        self._fan_index = fan_drawer_index * num_fan_per_drawer + fan_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_fan_info()

    def _init_fan_info(self):
        # type: () -> None
        fan_info = FAN_INFO.get(self._hwsku, {}).get(self._asic, None)
        if fan_info is None:
            self._logger.log_fatal("Failed in initializing fan info")
            raise RuntimeError("failed in initializing fan info")
        if self._fan_index >= len(fan_info):
            self._logger.log_fatal("Failed in initializing fan info")
            raise RuntimeError("failed in initializing fan info")
        self._fan_info = fan_info[self._fan_index]
        # Static information
        self._fan_name = self._fan_info.get("name", NOT_AVAILABLE)
        self._logger.log_info("Initialized info for <{}>".format(self._fan_name))

    def get_direction(self):
        # type: () -> str
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        return self._helper.get_sysfs_content(self._fan_info, "direction")

    def get_speed(self):
        # type: () -> int
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        try:
            return int(self._helper.get_sysfs_content(self._fan_info, "speed"))
        except Exception as err:
            self._logger.log_error(
                "Failed in getting speed for <{}>".format(self._fan_name)
            )
            return 0

    def get_target_speed(self):
        # type: () -> int
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        try:
            return int(self._helper.get_sysfs_content(self._fan_info, "speed"))
        except Exception as err:
            self._logger.log_error(
                "Failed in getting target speed for <{}>".format(self._fan_name)
            )
            return 0

    def get_speed_tolerance(self):
        # type: () -> int
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return FAN_SPEED_TOLERANCE

    def is_under_speed(self):
        """
        Calculates if the fan speed is under the tolerated low speed threshold

        Default calculation requires get_speed_tolerance to be implemented, and checks
        if the current fan speed (expressed as a percentage) is lower than <get_speed_tolerance>
        percent below the target fan speed (expressed as a percentage)

        Returns:
            A boolean, True if fan speed is under the low threshold, False if not
        """
        return self.get_speed() * 100 < self.get_target_speed() * (100 - self.get_speed_tolerance())

    def is_over_speed(self):
        """
        Calculates if the fan speed is over the tolerated high speed threshold

        Default calculation requires get_speed_tolerance to be implemented, and checks
        if the current fan speed (expressed as a percentage) is higher than <get_speed_tolerance>
        percent above the target fan speed (expressed as a percentage)

        Returns:
            A boolean, True if fan speed is over the high threshold, False if not
        """
        return self.get_speed() * 100 > self.get_target_speed() * (100 + self.get_speed_tolerance())

    def set_speed(self, speed):
        # type: () -> bool
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
        # type: () -> bool
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        # type: () -> str
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if not self._helper.get_sysfs_content(self._fan_info, "presence"):
            return self.STATUS_LED_COLOR_OFF
        if not self._helper.get_sysfs_content(self._fan_info, "status"):
            return self.STATUS_LED_COLOR_RED
        return self.STATUS_LED_COLOR_GREEN

    def get_fan_info_dict(self):
        # type: () -> dict[str, bool|int|str]
        """
        Retrieves the fan info dict of the fan

        Returns:
            dict: The fan info dict of the fan who has following keys:
                status: the status of the fan
                direction: the rotate direction of the fan
                speed: the speed of the fan
                presence: the presence status of the fan
        """
        return {
            "status": self.get_status(),
            "direction": self.get_direction(),
            "speed": self.get_speed(),
            "presence": self.get_presence(),
        }

    def get_warning(self):
        # type: () -> bool
        """
        Retrieves the warning status of the device

        Returns:
            A boolean value, True if device is warning, False if not
        """
        return not self._helper.get_sysfs_content(self._fan_info, "status")

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._fan_name

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return self._helper.get_sysfs_content(self._fan_info, "presence")

    def get_model(self):
        # type: () -> str
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return NOT_AVAILABLE

    def get_serial(self):
        # type: () -> str
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return NOT_AVAILABLE

    def get_revision(self):
        # type: () -> str
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return NOT_AVAILABLE

    def get_status(self):
        # type: () -> bool
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._helper.get_sysfs_content(self._fan_info, "status")

    def get_position_in_parent(self):
        # type: () -> int
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._fan_index + 1

    def is_replaceable(self):
        # type: () -> bool
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

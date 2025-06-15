####################################################################
# Asterfusion CX-N Devices Fan Drawer                              #
#                                                                  #
# Module contains an implementation of SONiC Platform Base API and #
# provides the fan status which are available in the platform      #
#                                                                  #
####################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger
    from .fan import Fan

    from sonic_platform_base.fan_drawer_base import FanDrawerBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class FanDrawer(FanDrawerBase):
    """Platform-specific Fan Drawer class"""

    def __init__(self, fan_drawer_index, num_fan_per_drawer, hwsku, asic):
        # type: (int, int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        FanDrawerBase.__init__(self)
        self._fan_drawer_index = fan_drawer_index
        self._num_fan_per_drawer = num_fan_per_drawer
        self._hwsku = hwsku
        self._asic = asic
        self._init_fan_drawer_info()
        self._init_fan_info()

    def _init_fan_drawer_info(self):
        fan_drawer_info = FAN_DRAWER_INFO.get(self._hwsku, {}).get(self._asic, None)
        if fan_drawer_info is None:
            self._logger.log_fatal("Failed in initializing fan drawer info")
            raise RuntimeError("failed in initializing fan drawer info")
        if self._fan_drawer_index >= len(fan_drawer_info):
            self._logger.log_fatal("Failed in initializing fan drawer info")
            raise RuntimeError("failed in initializing fan drawer info")
        self._fan_drawer_info = fan_drawer_info[self._fan_drawer_index]
        # Static information
        self._fan_drawer_name = self._fan_drawer_info.get("name", NOT_AVAILABLE)
        self._logger.log_info("Initialized info for <{}>".format(self._fan_drawer_name))

    def _init_fan_info(self):
        for fan_index in range(self._num_fan_per_drawer):
            self._fan_list.append(
                Fan(
                    self._fan_drawer_index,
                    fan_index,
                    self._num_fan_per_drawer,
                    self._hwsku,
                    self._asic,
                )
            )

    def get_num_fans(self):
        # type: () -> int
        """
        Retrieves the number of fans available on this fan drawer

        Returns:
            An integer, the number of fan modules available on this fan drawer
        """
        return len(self._fan_list)

    def get_all_fans(self):
        # type: () -> list[Fan]
        """
        Retrieves all fan modules available on this fan drawer

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this fan drawer
        """
        return self._fan_list

    def get_fan(self, index):
        # type: (int) -> Fan
        """
        Retrieves fan module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan module to
            retrieve

        Returns:
            An object dervied from FanBase representing the specified fan
            module
        """
        fan = None

        try:
            fan = self._fan_list[index]
        except IndexError:
            self._logger.log_error(
                "Fan index <{}> out of range (<0-{}>)".format(
                    index, len(self._fan_list) - 1
                )
            )

        return fan

    def set_status_led(self, color):
        # type: () -> bool
        """
        Sets the state of the fan drawer status LED

        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        # type: () -> str
        """
        Gets the state of the fan drawer status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        return NOT_AVAILABLE

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._fan_drawer_name

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        for fan in self._fan_list:
            if not fan.get_presence():
                return False
        return True

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
        for fan in self._fan_list:
            if not fan.get_status():
                return False
        return True

    def get_position_in_parent(self):
        # type: () -> int
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._fan_drawer_index + 1

    def is_replaceable(self):
        # type: () -> bool
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

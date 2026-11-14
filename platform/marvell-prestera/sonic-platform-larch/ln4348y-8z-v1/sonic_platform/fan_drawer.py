########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.led import (I2CLed, I2CRegisterBitField,
                                    LED_I2CBUS, LED_I2CADDR,
                                    FANLED_MASK, FANLED_STATES)
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FANS_PER_TRAY = 1


class FanDrawer(FanDrawerBase):
    # All fan trays share a single front-panel fan LED
    _fan_led = I2CLed(I2CRegisterBitField(LED_I2CBUS, LED_I2CADDR, None,
                                          FANLED_MASK, FANLED_STATES))
    # every drawer instance, so that the shared LED can be driven from the
    # state of all trays instead of the last drawer thermalctld looked at
    _drawers = []

    def __init__(self, fantray_index):
        super().__init__()
        FanDrawer._drawers.append(self)

        # FanTray indexing is 0-based in platforms
        self.fantrayindex = fantray_index
        self.__initialize_fan_drawer()

    def __initialize_fan_drawer(self):
        from sonic_platform.fan import Fan

        for i in range(NUM_FANS_PER_TRAY):
            self._fan_list.append(Fan(self.fantrayindex, i))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantrayindex + 1)

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        return self._fan_list[0].get_presence()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._fan_list[0].get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self._fan_list[0].get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._fan_list[0].get_status()

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
        return (self.fantrayindex + 1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED

        Note: the platform has a single front-panel fan LED shared by
        all fan trays.
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # thermalctld calls this once per fan with that fan's colour; with a
        # single LED the last call would win (a pulled tray followed by a
        # healthy one turned the LED green again - Lanaotek report, #36006).
        # Drive the LED from the aggregate instead: red when any tray is
        # absent or faulty, green when all are fine. The requested colour is
        # only honoured when it is not green/red (e.g. off/amber for tests).
        color = color.lower()
        if color in ("green", "red"):
            color = self._aggregate_color()
        try:
            self._fan_led.set_state("STATUS_LED_COLOR_" + color.upper())
        except Exception:          # LED register unreachable until the PLD is located
            return False
        return True

    @classmethod
    def _aggregate_color(cls):
        for drawer in cls._drawers:
            try:
                if not drawer.get_presence() or not drawer.get_status():
                    return "red"
            except Exception:
                return "red"
        return "green"

    def get_status_led(self):
        """
        Gets the state of the fan drawer status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        try:
            return self._get_status_led_impl()
        except Exception:
            return self.STATUS_LED_COLOR_OFF

    def _get_status_led_impl(self):
        state = self._fan_led.get_state()
        # Convert "STATUS_LED_COLOR_GREEN" -> "green"
        return state.replace("STATUS_LED_COLOR_", "").lower()

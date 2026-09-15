########################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan-Drawers' information available in the platform.
#
########################################################################

try:
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def _get_platform_fan_layout(pddf_obj):
    """Return (num_fantrays, num_fans_pertray) from PDDF JSON."""
    if pddf_obj is None:
        return (1, 1)

    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    if not isinstance(pddf_json, dict):
        return (1, 1)

    platform = pddf_json.get("PLATFORM", {})
    if not isinstance(platform, dict):
        return (1, 1)

    try:
        num_fantrays = int(platform.get("num_fantrays", 1))
    except (ValueError, TypeError):
        num_fantrays = 1

    try:
        num_fans_pertray = int(platform.get("num_fans_pertray", 1))
    except (ValueError, TypeError):
        num_fans_pertray = 1

    if num_fantrays < 1:
        num_fantrays = 1
    if num_fans_pertray < 1:
        num_fans_pertray = 1

    return (num_fantrays, num_fans_pertray)


class FanDrawer(PddfFanDrawer):
    """Platform-specific Fan class"""

    # LED colour constants (inherited from DeviceBase, repeated for clarity)
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED   = "red"
    STATUS_LED_COLOR_OFF   = "off"

    def __init__(self, fantray_index, pddf_data=None, pddf_plugin_data=None):

        PddfFanDrawer.__init__(self, fantray_index, pddf_data, pddf_plugin_data)
        # FanTray is 0-based in platforms
        self.fantrayindex = fantray_index
        self.pddf_data = pddf_data
        self.pddf_plugin_data = pddf_plugin_data
        self.__initialize_fan_drawer()


    def __initialize_fan_drawer(self):
        self._fan_list = []
        from sonic_platform.fan import Fan
        num_fantrays, num_fans_pertray = _get_platform_fan_layout(self.pddf_data)
        for tray_idx in range(num_fantrays):
            if tray_idx != self.fantrayindex:
                continue
            for fan_idx in range(num_fans_pertray):
                self._fan_list.append(Fan(
                    tray_idx,
                    fan_idx,
                    pddf_data=self.pddf_data,
                    pddf_plugin_data=self.pddf_plugin_data,
                    is_psu_fan=False,
                    psu_index=0,
                ))

    def get_name(self):
        """
        Retrieves the fan drawer name
        Returns:
            string: The name of the device
        """
        try:
            name = PddfFanDrawer.get_name(self)
            if name is not None:
                return name
        except Exception:
            pass
        return "FanTray{}".format(self.fantrayindex+1)

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        try:
            presence = PddfFanDrawer.get_presence(self)
            if presence is not None:
                return presence
        except Exception:
            pass
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

    def set_status_led(self, color):
        """Set the fan drawer status LED.

        The platform has no dedicated per-fan-drawer LED; the overall
        system status LED on the chassis is used instead.  Accept and
        acknowledge the request so that thermalctld does not raise an
        error, but there is no physical LED to drive.

        Args:
            color: A string — 'green', 'amber', 'red', or 'off'.
        Returns:
            bool: True always (no hardware to fail).
        """
        return True

    def get_status_led(self):
        """Get the fan drawer status LED colour.

        Derived from the operational health of all fans in the tray:
          - All fans present and no faults → green
          - Any fan absent or faulted       → amber

        Returns:
            A string: 'green', 'amber', or 'off'.
        """
        if not self._fan_list:
            return self.STATUS_LED_COLOR_OFF

        for fan in self._fan_list:
            if not fan.get_presence() or not fan.get_status():
                return self.STATUS_LED_COLOR_AMBER

        return self.STATUS_LED_COLOR_GREEN

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
        return (self.fantrayindex+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

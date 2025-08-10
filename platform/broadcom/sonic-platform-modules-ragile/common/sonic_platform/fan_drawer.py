#!/usr/bin/env python3
#
# fan_drawer_base.py
#
# Abstract base class for implementing a platform-specific class with which
# to interact with a fan drawer module in SONiC
#

try:
    import time
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e


class FanDrawer(FanDrawerBase):
    """
    Abstract base class for interfacing with a fan drawer
    """
    # Device type definition. Note, this is a constant.
    DEVICE_TYPE = "fan_drawer"

    def __init__(self, interface_obj, fantray_index):
        FanDrawerBase.__init__(self)
        self.fantray_dict = {}
        self.fantray_update_time = 0
        self.fantray_index = fantray_index
        self.int_case = interface_obj
        self.fantrayname = "FAN" + str(fantray_index)
        self.num_fans_per_fantray = self.int_case.get_fan_rotor_number(self.fantrayname)
        for i in range(self.num_fans_per_fantray):
            self._fan_list.append(Fan(interface_obj, fantray_index, i + 1))

    def fantray_dict_update(self):
        local_time = time.time()
        # update data every 1 seconds
        if not self.fantray_dict or (local_time - self.fantray_update_time) >= 1:
            self.fantray_update_time = local_time
            self.fantray_dict = self.int_case.get_fan_info(self.fantrayname)

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        return "Fantray{}".format(self.fantray_index)

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        return self.int_case.get_fan_presence(self.fantrayname)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        self.fantray_dict_update()
        return self.fantray_dict["NAME"]

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        self.fantray_dict_update()
        return self.fantray_dict["SN"]

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        self.fantray_dict_update()
        return self.fantray_dict["HW"]

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        for i in range(self.num_fans_per_fantray):
            if self._fan_list[i].get_status() is False:
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

    def get_num_fans(self):
        """
        Retrieves the number of fans available on this fan drawer
        Returns:
            An integer, the number of fan modules available on this fan drawer
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this fan drawer
        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this fan drawer
        """
        return self._fan_list

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        # not supported
        return False

    def get_status_led(self):
        """
        Gets the state of the Fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence():
            return 'N/A'

        ret, color = self.int_case.get_fan_led(self.fantrayname)
        if ret is True:
            return color
        return 'N/A'

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer

        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        self.fantray_dict_update()
        return self.fantray_dict["PowerMax"]

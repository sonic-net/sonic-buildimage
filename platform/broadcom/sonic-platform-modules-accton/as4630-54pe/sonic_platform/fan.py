#!/usr/bin/env python
import os.path

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SPEED_TOLERANCE = 10
HOST_FAN_TOLERANCE_FILE = "/usr/share/sonic/device/{}/tolerance_flag"
PMON_FAN_TOLERANCE_FILE = "/usr/share/sonic/platform/tolerance_flag"

FAN_NAME_LIST = ["FAN-1", "FAN-2", "FAN-3"]

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

        self._api_helper = APIHelper()
        self.is_psu_fan = is_psu_fan
        self.is_host = self._api_helper.is_host()

        # tolerance path
        self.tolerance_flag = PMON_FAN_TOLERANCE_FILE
        if self.is_host:
            self.tolerance_flag = HOST_FAN_TOLERANCE_FILE.format(self._api_helper.get_platform())

    # Provide the functions/variables below for which implementation is to be overwritten
    # Since AS4630 psu_fan airflow direction cant be read from sysfs, it is fixed as 'F2B' or 'Exhaust'
    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = 'N/A'
        if self.is_psu_fan:
            direction = self.FAN_DIRECTION_EXHAUST
        else:
            direction = super().get_direction()
            if direction is not None and len(direction) > 0:
                return direction

        return direction

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_NAME_LIST[self.fantray_index - 1] \
            if not self.is_psu_fan \
            else "PSU-{} FAN-{}".format(self.fans_psu_index, self.fan_index)

        return fan_name

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan:
            return super().get_speed()
        else:
            return super().get_target_speed()

    def set_tolerance_mode(self, mode):
        """
        Set the fan tolerance mode using a flag file.
        Args:
            mode:
                - "off":  tolerance off      → create flag file
                - "on": tolerance activate   → delete flag file
        Returns:
            bool: True if the operation succeeded, False otherwise.
        """
        try:
            if mode == "off":
                open(self.tolerance_flag, "a").close()
            elif mode == "on":
                if os.path.exists(self.tolerance_flag):
                    os.remove(self.tolerance_flag)
            else:
                return False
            return True

        except OSError:
            return False

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        tolerance = SPEED_TOLERANCE
        if self.is_psu_fan:
            return tolerance

        if os.path.exists(self.tolerance_flag):
            raise NotImplementedError

        return tolerance
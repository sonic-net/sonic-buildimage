#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from .helper import APIHelper
    import os.path
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TARGET_SPEED_PATH = "/tmp/fan_target_speed"
FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R",
                 "FAN-3F", "FAN-3R", "FAN-4F", "FAN-4R",
                 "FAN-5F", "FAN-5R"]

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_NAME_LIST[(self.fantray_index-1)*2 + self.fan_index-1] \
            if not self.is_psu_fan \
            else "PSU-{} FAN-{}".format(self.fans_psu_index, self.fan_index)

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

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0
        if self.is_psu_fan:
            return super().get_speed()
        elif super().get_presence():
            if os.path.isfile(TARGET_SPEED_PATH):
                speed = self._api_helper.read_txt_file(TARGET_SPEED_PATH)
            else:
                speed = super().get_target_speed()
            if speed is None:
                return 0

        return int(speed)

    def set_speed(self, speed):
        ret = super().set_speed(speed)
        if ret == True:
            self._api_helper.write_txt_file(TARGET_SPEED_PATH, int(speed))

        return ret

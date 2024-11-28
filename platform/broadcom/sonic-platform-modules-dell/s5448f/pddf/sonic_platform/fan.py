#!/usr/bin/env python
try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    import subprocess
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten e.g.

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
            considered tolerable
        """
        return 25

    def set_speed(self, speed):
        print("Setting fan speed is not allowed")
        return False

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            String: Part number of FAN
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        return 'NA'

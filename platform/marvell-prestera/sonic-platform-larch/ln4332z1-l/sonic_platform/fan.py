#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


# Max RPM measured at full PWM (0xFF → raw 0xD5 → 0xD5 * 112 = 23856)
MAX_RPM = 23856


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_speed(self):
        rpm = self.get_speed_rpm()
        percent = int(rpm * 100 / MAX_RPM)
        return min(percent, 100)

    def get_target_speed(self):
        return self.get_speed()

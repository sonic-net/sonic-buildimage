#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/2/18 15:43
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from . import helper
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

Fan_Direction_Cmd = "0x3a 0x54 {}"


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self.helper = helper.APIHelper()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 15 if "PSU" in self.get_name() else 10

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM
        (cause of the conversion, it needs to * 150)

        Returns:
            An integer, Speed of fan in RPM
        """
        rpm_speed = super().get_speed_rpm()
        if self.helper.is_bmc_present():
            if self.is_psu_fan:
                return rpm_speed
            fan_name = self.get_name()
            return (rpm_speed * 156) if "Front" in fan_name else (rpm_speed * 142)
        return rpm_speed

    def get_status_led(self):
        if not self.is_psu_fan and not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        else:
            return super().get_status_led()

#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/3/10 10:23
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    import re
    import os
    from . import helper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

Change_Channel_Cmd = "0x3a 0x3e 6 0xe0 0 {}"


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self.STATUS_LED_COLOR_BLUE = "blue"
        self.helper = helper.APIHelper()

    @staticmethod
    def get_capacity():
        return 2000

    def get_type(self):
        return 'AC'

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # PSU LED is controlled by the PSU firmware, so soft
        # simulating the LED in a generic way based on the PSU status
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_BLUE
            else:
                return self.STATUS_LED_COLOR_AMBER

        return "off"   

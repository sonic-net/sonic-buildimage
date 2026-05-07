#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(PddfFanDrawer):
    """PDDF Platform-Specific Fan-Drawer class"""

    def __init__(self, tray_idx, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based
        PddfFanDrawer.__init__(self, tray_idx, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def set_status_led(self, color):
        result = False
        # led color descriptions are not same with BSP driver, so converts here
        color_dict = {
            self.STATUS_LED_COLOR_GREEN : "STATUS_LED_COLOR_GREEN",
            self.STATUS_LED_COLOR_RED   : "STATUS_LED_COLOR_RED",
            self.STATUS_LED_COLOR_AMBER : "STATUS_LED_COLOR_AMBER",
            self.STATUS_LED_COLOR_OFF   : "STATUS_LED_COLOR_OFF"
        }
        led_device_name = "FANTRAY{}".format(self.fantray_index) + "_LED"
        result, msg = self.pddf_obj.set_system_led_color(led_device_name, color_dict[color])
        return (result)
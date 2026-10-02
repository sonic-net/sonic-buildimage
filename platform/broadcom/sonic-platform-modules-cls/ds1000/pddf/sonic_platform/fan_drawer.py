#############################################################################
# PDDF
#
# PDDF fan_drawer base class inherited from the common base class fan_drawer.py
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class FanDrawer(PddfFanDrawer):
    """PDDF Platform-Specific Fan-Drawer class"""

    def __init__(self, tray_idx, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based
        PddfFanDrawer.__init__(self, tray_idx, pddf_data, pddf_plugin_data)


    def set_status_led(self, color):
        return self._fan_list[0].set_status_led(color)

    def get_status_led(self, color=None):
        return self._fan_list[0].get_status_led()

    def get_serial(self):
        serial = "Unknown"
        return serial

    def get_model(self):
        model = "Unknown"
        return model

    def get_name(self):
        return "Drawer {0}".format(self.fantray_index)

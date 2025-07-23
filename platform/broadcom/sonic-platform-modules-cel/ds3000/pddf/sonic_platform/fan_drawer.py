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
    def get_presence(self):
        status = False
        # Usually if a tray is removed, all the fans inside it are absent
        if self._fan_list and len(self._fan_list) == 2:
            status = self._fan_list[0].get_presence() or self._fan_list[1].get_presence()
        else:
            status = self._fan_list[0].get_presence()
        return status

    def get_name(self):
        return "Fantray {0}".format(self.fantray_index)

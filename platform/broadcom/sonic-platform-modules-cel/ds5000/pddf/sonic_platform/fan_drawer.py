#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the fan drawer management function
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
        
    def get_status_led(self):
        return self._fan_list[0].get_status_led()
        
    def set_status_led(self, color):
        return self._fan_list[0].set_status_led(color)

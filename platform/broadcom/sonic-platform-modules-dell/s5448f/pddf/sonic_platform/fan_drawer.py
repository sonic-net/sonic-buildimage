#!/usr/bin/env python
try:
    import re
    import subprocess
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(PddfFanDrawer):
    """PDDF Platform-Specific Fan-Drawer class"""
    FAN_FRU_MAPPING = { 1: 3, 2: 4, 3: 5, 4: 6 }
    def __init__(self, tray_idx, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based 
        PddfFanDrawer.__init__(self, tray_idx, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_model(self):
        """
        Retrieves the part number of the fan drawer
        Returns:
            string: Part number of fan drawer
        """
        try:
            result = ""
            command = "ipmitool fru print {}".format(self.FAN_FRU_MAPPING[self.fantray_index])
            status, output = subprocess.getstatusoutput(command)
            if not status:
                board_info = re.search(r"%s\s*:(.*)" % "Board Part Number", output)
                if result is not None:
                    result = board_info.group(1).strip()

        except:
            pass

        return result

    def get_serial(self):
        """
        Retrieves the serial number of the fan drawer
        Returns:
            string: Serial number of the fan drawer
        """
        try:
            result = ""
            command = "ipmitool fru print {}".format(self.FAN_FRU_MAPPING[self.fantray_index])
            status, output = subprocess.getstatusoutput(command)
            if not status:
                board_info = re.search(r"%s\s*:(.*)" % "Board Serial", output)
                if result is not None:
                    result = board_info.group(1).strip()

        except:
            pass

        return result

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by Fan Drawer
        Returns:
            A float, with value of the maximum consumable power of the
            component.
        """
        return 0.0

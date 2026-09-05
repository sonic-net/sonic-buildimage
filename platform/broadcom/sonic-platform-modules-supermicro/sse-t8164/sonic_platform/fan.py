#!/usr/bin/env python


try:
    import subprocess
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten e.g.
    #def get_name(self):
        ## Since AS7712 has two fans in a tray, modifying this function to return proper name
        #if self.is_psu_fan:
            #return "PSU_FAN{}".format(self.fan_index)
        #else:
            #return "Fantray{}_{}".format(self.fantray_index, {1:'Front', 2:'Rear'}.get(self.fan_index,'none'))

    def get_direction(self):
        """
        Retrieves the direction of the fan based on IPMI FRU data.

        Returns:
            str: FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
        """
        if self.is_psu_fan:
            return super().get_direction()
        else:
            if not self.get_presence():
                return None

            fru_id = self.fantray_index + 2  # 1->3, 2->4, 3->5, 4->6
            cmd = ["ipmitool", "fru", "print", str(fru_id)]

            try:
                result = subprocess.run(cmd, text=True, capture_output=True, check=True)
                fru_data = result.stdout.splitlines()
            except subprocess.CalledProcessError as e:
                print(f"Error executing IPMI command: {e}")
                return self.FAN_DIRECTION_EXHAUST

            asset_tag = None
            for line in fru_data:
                if line.startswith(" Product Asset Tag"):
                    asset_tag = line.split(":")[-1].strip()
                    break

            return self.FAN_DIRECTION_INTAKE if asset_tag == "B2F" else self.FAN_DIRECTION_EXHAUST

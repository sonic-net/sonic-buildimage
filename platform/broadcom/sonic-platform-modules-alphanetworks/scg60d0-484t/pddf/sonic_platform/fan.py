try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan:
            direction = self.FAN_DIRECTION_EXHAUST

        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_direction"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)
            if not output:
                return False

            mode = output['mode']
            val = output['status']

            val = val.rstrip()
            vmap = self.plugin_data['FAN']['direction'][mode]['valmap']
            if val in vmap:
                direction = vmap[val]
            else:
                direction = val

        return direction

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        # Fix the speed vairance to 20 percent. If it changes based on platforms, overwrite
        # this value in derived pddf fan class
        return 20

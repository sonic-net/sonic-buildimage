#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    def get_speed_rpm(self):
        if self.is_psu_fan:
            return int(round(super().get_speed_rpm()))
        else:
            divisor = 300000
            mask_low = 0xff
            ret = 15
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_input"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)
            if output is None:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                ret = int(float(output['status']))

            # Add a calibration factor and pin to max speed
            ret += 4
            if ret < 15:
                ret = 15

            return int(round(divisor/ret))

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan:
            direction = 'exhaust'
        else:
            direction = super().get_direction()

        return direction

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"


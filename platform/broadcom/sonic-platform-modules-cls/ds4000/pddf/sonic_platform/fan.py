#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/7/21  16:36
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from . import helper
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST = helper.APIHelper().get_bmc_status()
Fan_Direction_Cmd = "0x3a 0x62 {}"
SET_FAN_STATUS_LED_CMD = "0x3A 0x39 0x02 {} {}"


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self.helper = helper.APIHelper()

    def get_presence(self):
        """
          Retrieves the presence of fan
        """
        return super().get_presence()

    def get_name(self):
        """
        Retrieves the fan name
        Returns: String containing fan-name
        """
        fan_name = None

        if self.is_psu_fan and "fan_name" in self.plugin_data['PSU']:
            fan_name = self.plugin_data['PSU']['fan_name'][str(self.fans_psu_index)][str(self.fan_index)]

        elif not self.is_psu_fan and "name" in self.plugin_data['FAN']:
            fan_name = self.plugin_data['FAN']['name'][str(self.fantray_index)][str(self.fan_index)]

        return super().get_name() if fan_name is None else fan_name

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 15 if "PSU" in self.get_name() else 25

    def is_under_speed(self):
        speed = float(self.get_speed())
        target_speed = float(self.get_target_speed())
        speed_tolerance = self.get_speed_tolerance()

        speed_min_th = target_speed * (1 - float(speed_tolerance) / 100)
        if speed < speed_min_th:
            return True
        else:
            return False

    def is_over_speed(self):
        speed = float(self.get_speed())
        target_speed = float(self.get_target_speed())
        speed_tolerance = self.get_speed_tolerance()

        speed_max_th = target_speed * (1 + float(speed_tolerance) / 100)
        if speed > speed_max_th:
            return True
        else:
            return False

    def set_status_led(self,color):
        if self.is_psu_fan:
            return super().set_status_led(color)

        if color == self.get_status_led():
            return False

        if BMC_EXIST:
            fan_led_color_map = {
                'off': '00',
                'green': '01',
                'amber': '02',
                'red': '02'
            }

            fan_index_val = hex(self.fantray_index + 3)

            color_val = fan_led_color_map.get(color.lower(), None)

            if fan_index_val is None:
                return False

            if color_val is None:
                return False

            status, _ = self.helper.ipmi_raw(SET_FAN_STATUS_LED_CMD.format(fan_index_val,color_val))

            return status
        else:
            return self.set_system_led("SYS_LED", color)

import os

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SET_FAN_STATUS_LED_CMD = "0x3A 0x39 0x02 {} {}"
BMC_EXIST = APIHelper().is_bmc_present()

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self.helper = APIHelper()

    def get_presence(self):
        """
          Retrieves the presence of fan
        """
        if self.is_psu_fan:
            from sonic_platform.platform import Platform
            return Platform().get_chassis().get_psu(self.fans_psu_index-1).get_presence()

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

    def get_direction(self):
        """
          Retrieves the direction of fan
 
          Returns:
               A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
               depending on fan direction
               Or N/A if fan removed or abnormal
        """
        if not self.get_status():
           return 'N/A'
 
        return super().get_direction()


    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        target_speed = 0
        if self.is_psu_fan:
            # Target speed not usually supported for PSU fans
            raise NotImplementedError
        else:
            fan_name = self.get_name()
            f_r_fan = "Front" if fan_name.endswith(("1", "Front")) else "Rear"
            speed_rpm = self.get_speed_rpm()
            if(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][f_r_fan].isnumeric()):
                max_fan_rpm = int(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][f_r_fan])
            else:
                return target_speed
            speed_percentage = round(int((speed_rpm * 100) / max_fan_rpm))
            target_speed = speed_percentage

        return target_speed

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        fan_name = self.get_name()
        if self.is_psu_fan:
            attr = "psu_fan{}_speed_rpm".format(self.fan_index)
            device = "PSU{}".format(self.fans_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, attr)
            if not output:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                speed = int(float(output['status']))

            max_speed = int(self.plugin_data['PSU']['PSU_FAN_MAX_SPEED'])
            speed_percentage = round((speed*100)/max_speed)
            if speed_percentage >= 100:
                speed_percentage = 100
            return speed_percentage
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_input"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

            if not output:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                speed = int(float(output['status']))

            f_r_fan = "Front" if fan_name.endswith(("1", "Front")) else "Rear"
            if(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][f_r_fan].isnumeric()):
                max_speed = int(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][f_r_fan])
            else:
                return 0;
            speed_percentage = round((speed*100)/max_speed)
            if speed_percentage >= 100:
                speed_percentage = 100

            return speed_percentage

    def get_status_led(self):
        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        if self.is_psu_fan:
            # Usually no led for psu_fan hence raise a NotImplementedError
            raise NotImplementedError
        else:
            fan_led_device = "FANTRAY{}".format(self.fantray_index) + "_LED"
            if (not fan_led_device in self.pddf_obj.data.keys()):
                # Implement a generic status_led color scheme
                if self.get_status():
                    return self.STATUS_LED_COLOR_GREEN
                else:
                    return self.STATUS_LED_COLOR_OFF

            result, color = self.pddf_obj.get_system_led_color(fan_led_device)
            return (color)

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


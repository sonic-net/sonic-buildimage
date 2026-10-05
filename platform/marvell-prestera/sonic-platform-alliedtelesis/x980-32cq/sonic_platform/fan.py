#!/usr/bin/env python


try:
    import array as arr
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
            divisor = 2
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
            return int(round(ret))

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed). The x980-32cq can only be set
                   in units of 10%.

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if self.is_psu_fan:
            print("Setting PSU fan speed is not allowed")
            return False
        else:
            if speed < 0 or speed > 100:
                print("Error: Invalid speed %d. Please provide a valid speed percentage" % speed)
                return False

            if self.fan_index == 2:
                return True

            if 'duty_cycle_to_pwm' not in self.plugin_data['FAN']:
                print("Setting fan speed is not allowed !")
                return False
            else:
                possible = arr.array('i', [0x00, 0x19, 0x33, 0x4c, 0x66, 0x7f,
                                           0x99, 0xb2, 0xcc, 0xe5, 0xff])
                index = int((speed+5)/10)
                pwm = possible[index]
                status = False
                idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
                attr = "fan" + str(idx) + "_pwm"
                output = self.pddf_obj.set_attr_name_output("FAN-CTRL", attr, pwm)
                if not output:
                    return False
                status = output['status']
                return status

    def get_target_speed(self):
        if self.is_psu_fan:
            return super().get_speed()

        return super().get_target_speed()


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

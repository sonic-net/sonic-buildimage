#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2025/10
# @Mail    : sandyli@celestica.com
# @Author  : Sandy Li
try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from . import helper
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_PWM_CTRL_REG = 0xA44

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self.helper = helper.APIHelper()

    def get_name(self):
        """
        Retrieves the fan name
        Returns: String containing fan-name
        """
        if self.is_psu_fan:
            if 'fan_name' in self.plugin_data['PSU']:
                return self.plugin_data['PSU']['fan_name'][str(self.fans_psu_index)][str(self.fan_index)]

            return "PSU{}_FAN{}".format(self.fans_psu_index, self.fan_index)
        else:
            if 'name' in self.plugin_data['FAN']:
                return self.plugin_data['FAN']['name'][str(self.fantray_index)][str(self.fan_index)]

            return "Fantray{}_{}".format(self.fantray_index, self.fan_index)

    def get_direction(self):
        """
          Retrieves the direction of fan

          Returns:
               A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
               depending on fan direction
               Or N/A if fan removed or abnormal
        """
        """
        if self.is_psu_fan:
            if self.get_speed() != 0:
                return "EXHAUST"
            else:
                return "N/A"
        else:
            if not self.get_status():
                return 'N/A'
        """
        return super().get_direction()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 15 if "PSU" in self.get_name() else 25

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
            if self.helper.get_bmc_status():
                idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
                attr = "fan" + str(idx) + "_pwm"
                output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

                if not output:
                    return 0

                output['status'] = output['status'].rstrip()
                if output['status'].isalpha():
                    return 0
                else:
                    fpwm = int(float(output['status']))
                target_speed = int(fpwm)
            else:
                pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
                status, pwm = self.helper.cpld_lpc_read(FAN_PWM_CTRL_REG)

                if not status:
                    return 0

                fpwm = int(pwm, 16)
                speed_percentage = int(round(pwm_to_dc(fpwm)))
                target_speed = speed_percentage

        return 100 if target_speed > 100 else target_speed

    def get_speed(self):
        """
        Obtain the fan speed ratio (rpm/max rpm) according to the fan maximum rpm in the pd-plugin.json file

        returns: if the value > 100, return the value of rpm. else return Speed/percentage of maximum speed.
        """
        fan_name = self.get_name()
        speed_rpm = self.get_speed_rpm()
        fan_dir = self.get_direction()

        if speed_rpm is None:
            return 0

        f_r_fan = "Front" if "Front" in fan_name else "Rear"
        if "PSU" in fan_name:
            max_psu_fan_rpm = int(self.plugin_data['PSU']['PSU_FAN_MAX_SPEED'])
            psu_speed_percentage = int(round(speed_rpm / max_psu_fan_rpm * 100))
            return 100 if psu_speed_percentage > 100 else psu_speed_percentage

        # if use 'get_direction' to get the fan direction, it will make python maximum recursion depth exceeded.
        max_fan_rpm = int(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][fan_dir][f_r_fan])
        fan_speed_percentage = int(round(speed_rpm / max_fan_rpm * 100))
        return 100 if fan_speed_percentage > 100 else fan_speed_percentage

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

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

            if 'duty_cycle_to_pwm' not in self.plugin_data['FAN']:
                print("Setting fan speed is not allowed !")
                return False
            else:
                duty_cycle_to_pwm = eval(self.plugin_data['FAN']['duty_cycle_to_pwm'])
                pwm = int(round(duty_cycle_to_pwm(speed)))

                status = self.helper.cpld_lpc_write(FAN_PWM_CTRL_REG, pwm)

                return status

    def get_status(self):
        status = None
        if self.is_psu_fan:
            status = self.is_psu_power_on()
        else:
            speed = self.get_speed()
            status = True if (speed != 0) else False
        return status

    def is_psu_power_on(self):
        fan_psu_index = self.fans_psu_index
        if fan_psu_index is not None:
            device = "PSU{}".format(fan_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_p_in")
            if not output:
                return False
            elif float(output['status'])/1000000 != 0:
                return True
        return False

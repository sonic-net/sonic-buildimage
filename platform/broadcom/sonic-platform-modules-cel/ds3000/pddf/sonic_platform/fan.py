import os

try:
    from .helper import APIHelper
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    BMC_FAN_FSC_STATUS_CMD = "ipmitool raw 0x3a 0x26 0x00"
    LPC_CPLD_SETREG_PATH = "/sys/bus/platform/devices/baseboard/setreg"
    FAN_PWM_CTRL_REG_MAP = {
        1: '0xa1b2',
        2: '0xa1b8',
        3: '0xa1c4',
        4: '0xa1ca'
    }

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()

    def get_presence(self):
        """
          Retrieves the presence of fan
        """
        if self.is_psu_fan:
            return super().get_presence()
        return super().get_presence() and self.get_status()

    def get_direction(self):
        """
          Retrieves the direction of fan

          Returns:
               A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
               depending on fan direction
               Or N/A if fan removed or abnormal
        """
        if not self.get_status():
           return "N/A"

        if self.is_psu_fan:
            from sonic_platform.psu import Psu
            psu = Psu(self.fans_psu_index - 1, self.pddf_obj, self.plugin_data)
            model = psu.get_model()
            if model in ["FSP550-20FM", "G1251-0550WNA"]:
                return "EXHAUST"
            elif model in ["FSP550-29FM", "G1251-0550WRA"]:
                return "INTAKE"
            return "Unknown"

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
            # PSU fan not controllable, return current speed
            return self.get_speed()
        else:
            speed_rpm = self.get_speed_rpm()
            if self.fan_index == 1:
                max_fan_rpm = eval(self.plugin_data['FAN']['FRONT_FAN_MAX_RPM_SPEED']) # nosemgrep
            else:
                max_fan_rpm = eval(self.plugin_data['FAN']['REAR_FAN_MAX_RPM_SPEED']) # nosemgrep
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
            return speed_percentage if speed_percentage <= 100 else 100
        else:
            # Get fan rpm instead of fan pwm
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

            if self.fan_index == 1:
                max_speed = int(self.plugin_data['FAN']['FRONT_FAN_MAX_RPM_SPEED'])
            else:
                max_speed = int(self.plugin_data['FAN']['REAR_FAN_MAX_RPM_SPEED'])
            speed_percentage = round((speed*100)/max_speed)

            return speed_percentage if speed_percentage <= 100 else 100

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

        if speed < 0 or speed > 100:
            print("Error: Invalid speed %d. Please provide a valid speed percentage" % speed)
            return False

        if 'duty_cycle_to_pwm' not in self.plugin_data['FAN']:
            print("Setting fan speed is not allowed !")
            return False

        duty_cycle_to_pwm = eval(self.plugin_data['FAN']['duty_cycle_to_pwm']) # nosemgrep
        pwm = int(round(duty_cycle_to_pwm(speed)))

        if self._api_helper.is_bmc_present():
            status, data = self._api_helper.get_cmd_output(self.BMC_FAN_FSC_STATUS_CMD)
            if status != 0:
                print("Error: failed to get BMC FSC status")
                return False
            if data == '01':
                # Enable BMC FSC mode
                return False

        # FAN 1 & 2 in same fantray share the same register, skip Fan2 setting
        if self.fan_index == 2:
            return True
        # Set FAN PWM through baseboard CPLD
        reg = self.FAN_PWM_CTRL_REG_MAP.get(self.fantray_index)
        status = self._api_helper.lpc_setreg(self.LPC_CPLD_SETREG_PATH, reg, hex(pwm))

        return status

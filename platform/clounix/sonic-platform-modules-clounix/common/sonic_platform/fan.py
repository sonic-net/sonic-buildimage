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

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_presence(self):
        if self.is_psu_fan:
            device = "PSU{}".format(self.fans_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_present")
            if not output:
                return False

            mode = output['mode']
            status = output['status']

            vmap = self.plugin_data['PSU']['psu_present'][mode]['valmap']

            if status.rstrip('\n') in vmap:
                return vmap[status.rstrip('\n')]
            else:
                return False
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr_name = "fan" + str(idx) + "_present"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr_name)
            if not output:
                return False

            mode = output['mode']
            presence = output['status'].rstrip()

            vmap = self.plugin_data['FAN']['present'][mode]['valmap']

            if presence in vmap:
                status = vmap[presence]
            else:
                status = False

            return status
    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan:
            if not self.get_presence():
               return 0
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
            return speed_percentage
        else:
            # TODO This calculation should change based on MAX FAN SPEED
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

            pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
            speed_percentage = int(round(pwm_to_dc(fpwm)))

            return speed_percentage

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, Speed of fan in RPM
        """
        if self.is_psu_fan:
            if not self.get_presence():
               return 0
            attr = "psu_fan{}_speed_rpm".format(self.fan_index)
            device = "PSU{}".format(self.fans_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, attr)
            if not output:
                return 0

            output['status'] = output['status'].rstrip()

            if output['status'].replace('.', '', 1).isdigit():
                speed = int(float(output['status']))
            else:
                return 0

            rpm_speed = speed
            return rpm_speed
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_input"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

            if output is None:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                rpm_speed = int(float(output['status']))

            return rpm_speed
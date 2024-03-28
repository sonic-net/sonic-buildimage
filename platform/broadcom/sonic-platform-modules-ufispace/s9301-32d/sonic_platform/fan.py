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
    # Since psu_fan airflow direction cant be read from sysfs, it is fixed as 'F2B' or 'intake'

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed_percentage = 0
        if self.is_psu_fan:
            attr = "psu_fan{}_speed_rpm".format(self.fan_index)
            device = "PSU{}".format(self.fans_psu_index)
            max_speed = int(self.plugin_data['PSU']['PSU_FAN_MAX_SPEED']) 
        else:
            if self.fan_index == 1:
                pos = "f"
                max_speed = int(self.plugin_data['FAN']['FAN_F_MAX_SPEED']) 
            else:
                pos = "r"
                max_speed = int(self.plugin_data['FAN']['FAN_R_MAX_SPEED']) 
            attr = "fan{}_{}_speed_rpm".format(self.fantray_index, pos)
            device = "FAN-CTRL"

        output = self.pddf_obj.get_attr_name_output(device, attr)
        if not output:
            return speed_percentage

        output['status'] = output['status'].rstrip()
        if output['status'].isalpha():
            return speed_percentage
        else:
            speed = int(float(output['status']))

        speed_percentage = round((speed*100)/max_speed)
        
        return min(speed_percentage, 100)

    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, Speed of fan in RPM
        """
        rpm_speed = 0
        if self.is_psu_fan:
            attr = "psu_fan{}_speed_rpm".format(self.fan_index)
            device = "PSU{}".format(self.fans_psu_index)
        else:
            if self.fan_index == 1:
                pos = "f"
            else:
                pos = "r"
            attr = "fan{}_{}_speed_rpm".format(self.fantray_index, pos)
            device = "FAN-CTRL"

        output = self.pddf_obj.get_attr_name_output(device, attr)

        if output is None:
            return rpm_speed

        output['status'] = output['status'].rstrip()
        if output['status'].isalpha():
            return rpm_speed
        else:
            rpm_speed = int(float(output['status']))

        return rpm_speed

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_INTAKE
        if self.is_psu_fan:
            attr = "psu_fan{}_dir".format(self.fan_index)
            device = "PSU{}".format(self.fans_psu_index)
        else:
            attr = "fan{}_dir".format(self.fantray_index)
            device = "FAN-CTRL"

        output = self.pddf_obj.get_attr_name_output(device, attr)
        if not output:
            return direction

        mode = output['mode']
        val = output['status'].strip()
        vmap = self.plugin_data['FAN']['direction'][mode]['valmap']
   
        if val in vmap:
            direction = vmap[val]
            
        return direction

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        if self.is_psu_fan:
            attr = "psu_present"
            device = "PSU{}".format(self.fans_psu_index)
        else:
            attr = "fan{}_present".format(self.fantray_index)
            device = "FAN-CTRL"

        output = self.pddf_obj.get_attr_name_output(device, attr)
        if not output:
            return presence


        mode = output['mode']
        val = output['status'].strip()
        vmap = self.plugin_data['FAN']['present'][mode]['valmap']

        if val in vmap:
            presence = vmap[val]

        return presence

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return self.get_speed()

    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """

        print("Setting Fan speed is not allowed")
        return False


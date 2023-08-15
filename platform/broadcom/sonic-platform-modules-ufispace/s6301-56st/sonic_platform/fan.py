#!/usr/bin/env python

import os

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from sonic_platform.psu_fru import PsuFru
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
            max_speed = int(self.plugin_data['PSU']['PSU_FAN_MAX_SPEED'])
        else:
            max_speed = int(self.plugin_data['FAN']['FAN_MAX_SPEED'])

        speed = int(self.get_speed_rpm())


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
            output = self.pddf_obj.get_attr_name_output(device, attr)
            if output is None:
                return rpm_speed

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return rpm_speed
            else:
                rpm_speed = int(float(output['status']))
        else:
            ucd_path = "/sys/bus/i2c/devices/5-0034/hwmon/"
            if os.path.exists(ucd_path):
                hwmon_dir = os.listdir(ucd_path)
                with open("{}/{}/temp{}_input".format(ucd_path, hwmon_dir[0], self.fantray_index), "rb") as f:
                    rpm_speed = int(f.read().strip())

        return rpm_speed

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_NOT_APPLICABLE
        if self.is_psu_fan:
            psu_fru = PsuFru(self.fans_psu_index)
            if psu_fru.mfr_id == "not available":
                return direction
            for dev in self.plugin_data['PSU']['psu_support_list']:
                if dev['Mfr_id'] == psu_fru.mfr_id and dev['Model'] == psu_fru.model:
                    dir = dev['Dir']
                    break
        else:
            attr = "fan{}_direction".format(self.fantray_index)
            device = "FAN-CTRL"
            output = self.pddf_obj.get_attr_name_output(device, attr)
            if not output:
                return direction
            mode = output['mode']
            val = output['status'].strip()
            vmap = self.plugin_data['FAN']['direction'][mode]['valmap']
            if val in vmap:
                dir = vmap[val]

        return dir

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


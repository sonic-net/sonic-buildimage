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

    def get_mfr_id(self):
        """
        Retrieves the manufacturer id of the device

        Returns:
            string: Manufacturer Id of device
        """
        if self.is_psu_fan:
            device = "PSU{}".format(self.fans_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_mfr_id")
        else:
            raise NotImplementedError

        if not output:
            return None

        mfr = output['status']

        # strip_non_ascii
        stripped = (c for c in mfr if 0 < ord(c) < 127)
        mfr = ''.join(stripped)

        return mfr.rstrip('\n')

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            device = "PSU{}".format(self.fans_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_model_name")
        else:
            raise NotImplementedError

        if not output:
            return None

        model = output['status']

        # strip_non_ascii
        stripped = (c for c in model if 0 < ord(c) < 127)
        model = ''.join(stripped)

        return model.rstrip('\n')

    def get_max_speed(self):
        """
        Retrieves the max speed

        Returns:
            An Integer, the max speed
        """
        if self.is_psu_fan:
            mfr = self.get_mfr_id()
            model = self.get_model()

            max_speed = int(self.plugin_data['PSU']['valmap']['PSU_FAN_MAX_SPEED_AC'])
            if mfr and model :
                for dev in self.plugin_data['PSU']['psu_support_list']:
                    if dev['Manufacturer'] == mfr and dev['Name'] == model:
                        max_speed = int(self.plugin_data['PSU']['valmap'][dev['MaxSpd']])
                        break
        else:
            max_speed = int(self.plugin_data['FAN']['FAN_MAX_SPEED'])

        return max_speed

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed_percentage = 0

        max_speed = self.get_max_speed()
        rpm_speed = self.get_speed_rpm()

        speed_percentage = round((rpm_speed*100)/max_speed)

        return min(speed_percentage, 100)

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.is_psu_fan:
            attr_name = "psu_present"
            device = "PSU{}".format(self.fans_psu_index)
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr_name = "fan" + str(idx) + "_present"
            device = "FAN-CTRL"

        output = self.pddf_obj.get_attr_name_output(device, attr_name)
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

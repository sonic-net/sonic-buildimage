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

    def calculate_speed_percentage(self, current_rpm, target_rpm, max_speed_ratio):
        if max_speed_ratio == 0:
            raise ValueError("max_speed_ratio must not be zero to avoid division by zero.")
        
        current_speed_percentage = (current_rpm / max_speed_ratio) * 100
        max_speed = round(current_speed_percentage)
        
        if max_speed == 0:
            raise ValueError("max_speed must not be zero to avoid division by zero.")
        
        target_speed_percentage = round((target_rpm * 100) / max_speed)
        
        return target_speed_percentage

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
        
    def set_status_led(self, color):
        result = False
        # led color descriptions are not same with BSP driver, so converts here
        color_dict = {
            self.STATUS_LED_COLOR_GREEN : "STATUS_LED_COLOR_GREEN",
            self.STATUS_LED_COLOR_RED   : "STATUS_LED_COLOR_RED",
            self.STATUS_LED_COLOR_AMBER : "STATUS_LED_COLOR_AMBER",
            self.STATUS_LED_COLOR_OFF   : "STATUS_LED_COLOR_OFF"
        }
        if self.is_psu_fan:
            # Usually no led for psu_fan hence raise a NotImplementedError
            raise NotImplementedError
        else:
            # Usually there is no led for psu_fan
            led_device_name = "FANTRAY{}".format(self.fantray_index) + "_LED"
            result, msg = self.pddf_obj.set_system_led_color(led_device_name, color_dict[color])
        return (result)

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the rpm of full fan speed
        """
        target_speed = 0
        if self.is_psu_fan:
            # Target speed not usually supported for PSU fans
            raise NotImplementedError
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_speed_target"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

            if not output:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                target_speed = int(float(output['status']))

        speed_rpm = self.get_speed_rpm()
        speed_ratio = self.get_speed()
        target_speed_percent = 0 
        try:
            target_speed_percent = self.calculate_speed_percentage(speed_rpm, target_speed, speed_ratio)
        except ValueError as e:
            print(f"Error in calculating speed percentage: {e}")
        return target_speed_percent 

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the rpm of variance from target speed which is
                 considered tolerable
        """
        target_tolerance = 0
        if self.is_psu_fan:
            # Target speed not usually supported for PSU fans
            raise NotImplementedError
        else:
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_speed_tolerance"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

            if not output:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                target_tolerance = int(float(output['status']))

        speed_rpm = self.get_speed_rpm()
        speed_ratio = self.get_speed()
        target_tolerance_percent = 0 
        try:
            target_tolerance_percent = self.calculate_speed_percentage(speed_rpm, target_tolerance, speed_ratio)
        except ValueError as e:
            print(f"Error in calculating speed percentage: {e}")

        return target_tolerance_percent

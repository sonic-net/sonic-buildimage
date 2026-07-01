#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from sonic_platform import platform
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST = APIHelper().get_bmc_status()


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""
    PSU_MAX_FAN = "18000"
    POE_PSU_MAX_FAN = "26000"

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._direction = None

    # Provide the functions/variables below for which implementation is to be overwritten

    def _get_psu_fan_max_speed(self):
        platform_chassis = platform.Platform().get_chassis()
        num_psus = platform_chassis.get_num_psus()
        is_poe_psu_present = False

        for i in range(num_psus):
            psu = platform_chassis.get_psu(i)
            if psu and psu.get_presence():
                if psu.get_capacity() == psu.PLATFORM_POE_PSU_CAPACITY:
                    is_poe_psu_present = True
                    break

        if is_poe_psu_present:
            speed = self.POE_PSU_MAX_FAN
        else:
            speed = self.PSU_MAX_FAN
        return speed

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

    def get_status(self):
        # Optimize status logic
        if self.is_psu_fan:
            try:
                rpm = self.get_speed_rpm()
                return rpm is not None and rpm > 0
            except Exception:
                return False
        if not self.is_psu_fan and not self.get_presence():
            return False
        return True

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self._direction is None:
            if self.is_psu_fan:
                self._direction = self.FAN_DIRECTION_EXHAUST.upper()
            else:
                self._direction = super().get_direction()
        return self._direction

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 15 if "PSU" in self.get_name() else 10

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

            if BMC_EXIST:
                target_speed = int(fpwm + 1)
            else:
                pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
                speed_percentage = int(round(pwm_to_dc(fpwm)))
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

            max_speed = self._get_psu_fan_max_speed()
            speed_percentage = round((speed*100)/int(max_speed))
            return speed_percentage
        else:
            # TODO This calculation should change based on MAX FAN SPEED

            if 'FAN' not in self.plugin_data or\
                'FAN_MAX_RPM_SPEED' not in self.plugin_data['FAN']:
                return self.get_target_speed()

            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            attr = "fan" + str(idx) + "_input"
            output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)

            if output is None:
                return 0

            output['status'] = output['status'].rstrip()
            if output['status'].isalpha():
                return 0
            else:
                speed = int(float(output['status']))

            direction = self.get_direction()
            plugin_dict = self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][direction]
            if str(idx - 1) in plugin_dict:
                max_speed = plugin_dict[str(idx - 1)]
            elif str((idx - 1) % 2) in plugin_dict:
                max_speed = plugin_dict[str((idx - 1) % 2)]
            else:
                max_speed = plugin_dict["0"]

            speed_percentage = round((speed*100)/int(max_speed))

            return speed_percentage

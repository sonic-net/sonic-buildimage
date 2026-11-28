#!/usr/bin/env python
try:
    import array as arr
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    # Platforms that report fan speed as a raw RPM value directly (no
    # conversion formula needed) and support PWM speed control only in
    # fixed 10% steps via a lookup table. Add new platforms as needed.
    PLATFORMS_WITH_PWM_STEP_CONTROL = {
        'x86_64-alliedtelesis_x980_32cq-r0',
    }

    # PWM lookup table used on platforms with step-based speed control
    PWM_TABLE = arr.array('i', [0x00, 0x19, 0x33, 0x4c, 0x66, 0x7f,
                                 0x99, 0xb2, 0xcc, 0xe5, 0xff])

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._platform_name = device_info.get_platform()
        self._has_pwm_step_control = self._platform_name in self.PLATFORMS_WITH_PWM_STEP_CONTROL

    def get_speed_rpm(self):
        if self.is_psu_fan:
            return int(round(super().get_speed_rpm()))

        idx = (self.fantray_index - 1) * self.platform['num_fans_pertray'] + self.fan_index
        attr = "fan" + str(idx) + "_input"
        output = self.pddf_obj.get_attr_name_output("FAN-CTRL", attr)
        if output is None:
            return 0
        output['status'] = output['status'].rstrip()
        if output['status'].isalpha():
            return 0
        ret = int(float(output['status']))

        if self._has_pwm_step_control:
            # x980-32cq: sensor already reports RPM directly
            return int(round(ret))
        else:
            # x980-32dq: sensor reports a raw counter; convert via
            # calibrated divisor formula, pinned to a max-speed floor
            divisor = 300000
            ret += 4  # calibration factor
            if ret < 15:
                ret = 15
            return int(round(divisor / ret))

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed). On platforms with
                   PWM step control (e.g. x980-32cq), speed can only be set
                   in units of 10%.
        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if not self._has_pwm_step_control:
            return super().set_speed(speed)

        if self.is_psu_fan:
            print("Setting PSU fan speed is not allowed")
            return False

        if speed < 0 or speed > 100:
            print("Error: Invalid speed %d. Please provide a valid speed percentage" % speed)
            return False
        if 'duty_cycle_to_pwm' not in self.plugin_data['FAN']:
            print("Setting fan speed is not allowed !")
            return False

        index = int((speed + 5) / 10)
        pwm = self.PWM_TABLE[index]
        idx = (self.fantray_index - 1) * self.platform['num_fans_pertray'] + self.fan_index
        attr = "fan" + str(idx) + "_pwm"
        output = self.pddf_obj.set_attr_name_output("FAN-CTRL", attr, pwm)
        if not output:
            return False
        return output['status']

    def get_target_speed(self):
        if not self._has_pwm_step_control:
            return super().get_target_speed()
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
        if not self.is_psu_fan:
            return super().get_direction()

        # The PSU (Liteon PS-2162-6L2) has no PMBUS command for airflow
        # direction, so derive it from the model number instead: Liteon
        # appends 'R' to the base model for the reverse-airflow (intake)
        # variant, e.g. PS-2162-6L2R vs PS-2162-6L2.
        device = "PSU{}".format(self.fans_psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_model_name")
        if not output:
            return self.FAN_DIRECTION_EXHAUST

        model = output['status']
        # strip_non_ascii
        model = ''.join(c for c in model if 0 < ord(c) < 127).rstrip()

        if model.endswith('R'):
            return self.FAN_DIRECTION_INTAKE

        return self.FAN_DIRECTION_EXHAUST

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

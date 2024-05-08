try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
# ------------------------------------------------------------------
# HISTORY:
#    5/1/2022 (A.D.)
#    add function:set_status_led,
#    Solve the problem that when a fan is pulled out, the Fan LED on the front panel is still green Issue-#11525
# ------------------------------------------------------------------

MIN_SPEED = 40 # Percentage

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

        self.max_speed_rpm = 28600 #Max RPM from FAN spec

        # Remap LED color READ and OFF to AMBER as they are unsupported
        self.STATUS_LED_COLOR_RED = "amber"
        self.STATUS_LED_COLOR_OFF = "amber"

    def get_presence(self):
        if not self.is_psu_fan:
            # FANs on Ds1000 are all Fixed and present
            return True

        #For PSU, FAN must be present when PSU is present
        try:
            cmd = ['i2cget', '-y', '-f', '0x2', '0x32', '0x41']
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
            data = p.communicate()
            status = int(data[0].strip(), 16)
            if (self.fans_psu_index == 1 and (status & 0x10) == 0) or \
                (self.fans_psu_index == 2 and (status & 0x20) == 0):
                return True
        except (IOError, ValueError):
            pass

    def get_status(self):
        if not self.is_psu_fan:
            if not self.get_presence():
                return False

            # FANs must not be operated below MIN_SPEED
            target_speed = self.get_target_speed()
            if target_speed < MIN_SPEED:
                return False

            # FANs target speed and actual speed must
            # be within specified tolerance limits
            current_speed = self.get_speed()
            speed_tolerance = self.get_speed_tolerance()
            if abs(target_speed - current_speed) > speed_tolerance:
                return False

            return True

        return super().get_status()

    # Override get_speed as PDDF retrieves the speed from the
    # set PWM value which is actually the target fan speed
    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan:
            return super().get_speed()

        # Percentage of current FAN speed against the max FAN speed
        return round(super().get_speed_rpm() * 100 / self.max_speed_rpm)

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan:
            # Ds1000 PSU module only has EXHAUST fan
            return "exhaust"

        return super().get_direction()

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.is_psu_fan:
            return "N/A"

        return super().get_status_led()

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        if self.is_psu_fan:
            return False

        if self.get_status_led() == color:
            return True

        return super().set_status_led(color)

    def get_name(self):
        if self.is_psu_fan:
            return "PSU {} Fan {}".format(self.fans_psu_index, self.fan_index)
        else:
            return "Fan {}".format(self.fantray_index)
    
    def is_under_speed(self):
        speed = float(self.get_speed())
        target_speed = float(self.get_target_speed())
        speed_tolerance = self.get_speed_tolerance()

        speed_min_th = target_speed * (1 - float(speed_tolerance) / 100)
        if speed < speed_min_th:
            return True
        else:
            return False

    def is_over_speed(self):
        speed = float(self.get_speed())
        target_speed = float(self.get_target_speed())
        speed_tolerance = self.get_speed_tolerance()

        speed_max_th = target_speed * (1 + float(speed_tolerance) / 100)
        if speed > speed_max_th:
            return True
        else:
            return False

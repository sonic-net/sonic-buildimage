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


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)


    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        # Fix the speed vairance to 10 percent. If it changes based on platforms, overwrite
        # this value in derived pddf fan class
        return 20

    def get_presence(self):
        if self.is_psu_fan:
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

            return False
        else:
            #Overwirte the PDDF Common since the FANs on Belgite are all Fixed and present
            return True

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan:
            # Belgite PSU module only has EXHAUST fan
            return "EXHAUST"
        else:
            return super().get_direction()

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.is_psu_fan:
            return "N/A"
        else:
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
        else:
            return super().set_status_led(color)

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
FAN_DIRECTION_FILE_PATH = "/var/fan_direction"


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
            cmd_num = "58" if self.fans_psu_index == 1 else "59"
            cmd = ["i2cget", "-y", "-f", "4", "", "0x80"]
            cmd[4] = "0x" + cmd_num
            res = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True).stdout.read()
            # F2B 
            if res.strip() == "0x01":
                direction = "EXHAUST"
            else:
                direction = "INTAKE"
        else:
            direction = "INTAKE"
            with open(FAN_DIRECTION_FILE_PATH, "r") as f:
                fan_direction = f.read()
                if fan_direction.strip() == "FB":
                    direction = "EXHAUST"
        return direction
    

    def get_status(self):
        speed = self.get_speed_rpm()
        status = True if (speed != 0) else False
        return status

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
            target_speed = "N/A"
        else:
            idx = (self.fantray_index - 1) * 1 + self.fan_index
            attr = "fan" + str(idx) + "_pwm"
            pwm_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-2/2-0066/" + attr
            pwm = 0
            with open(pwm_path, "r") as f:
                pwm = f.read()

            percentage = int(pwm.strip())
            speed_percentage = int(round(percentage / 255 * 100))
            target_speed = speed_percentage

        return target_speed

    def set_status_led(self, color):
        color_dict = {"green": "STATUS_LED_COLOR_GREEN",
                      "red": "STATUS_LED_COLOR_AMBER"}
        color = color_dict.get(color, "off")
        index = str(self.fantray_index - 1)
        led_device_name = "FANTRAY{}".format(self.fantray_index) + "_LED"

        result, msg = self.pddf_obj.is_supported_sysled_state(led_device_name, color)
        if result is False:
            return False
        device_name = self.pddf_obj.data[led_device_name]['dev_info']['device_name']
        self.pddf_obj.create_attr('device_name', device_name, self.pddf_obj.get_led_path())
        self.pddf_obj.create_attr('index', index, self.pddf_obj.get_led_path())
        self.pddf_obj.create_attr('color', color, self.pddf_obj.get_led_cur_state_path())

        self.pddf_obj.create_attr('dev_ops', 'set_status', self.pddf_obj.get_led_path())
        return True

    @staticmethod
    def get_model():
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = "Unknown"
        return model

    @staticmethod
    def get_serial():
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = "Unknown"
        return serial

    def get_position_in_parent(self):
        """
        Retrieves the fan/psu fan index number
        """
        return self.fantray_index if not self.is_psu_fan else self.fans_psu_index + 4
		
    @staticmethod
    def is_replaceable():
        """
        Retrieves whether the device is replaceable
        """
        return False

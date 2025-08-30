#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform import platform
    from .helper import APIHelper
    from .thermal import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_FAN_MAX_RPM = 30000 # Taken from SPEC FSH082-610G Rev A12 at "9. Fans Control Requirement"
TRAY_FRONT_FAN_MAX_RPM = 31000
TRAY_REAR_FAN_MAX_RPM = 28000
TRAY_FANSPEED_TOLERANCE = 25
TRAY_LOW_FANSPEED_TOLERANCE = 45
TRAY_LOW_FANSPEED_RPM = 30

FP_FAN_LED_FILE = "/sys/class/leds/es9618xx_led::fan/brightness"
CPLD_I2C_PATH = "/sys/bus/i2c/devices/10-0066/fan"
FAN_LED_MODE = CPLD_I2C_PATH + "_led_mode"
FAN_PERCENTAGE = CPLD_I2C_PATH + "_duty_cycle_percentage"
FAN_PERCENTAGE_TMP = "/tmp/fan_duty_cycle_percentage"
PSU_I2C_PATH ="/sys/bus/i2c/devices/{}-00{}/"
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "59"
    },
    1: {
        "num": 2,
        "addr": "58"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "51"
    },
    1: {
        "num": 2,
        "addr": "50"
    },
}

LED_DEBUG_MODE_OFF = "0"
LED_DEBUG_MODE_ON = "1"


class Fan(FanBase):
    """Platform-specific Fan class"""
    
    MIN_VALID_COOLING_LEVEL = 1
    MAX_VALID_COOLING_LEVEL = 10
    min_cooling_level = 2
    current_cooling_level = min_cooling_level

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        # Front panel fan LED
        self.SYSFANLED_MODES = {
            "0" : self.STATUS_LED_COLOR_OFF,
            "1" : self.STATUS_LED_COLOR_GREEN,
            "3" : self.STATUS_LED_COLOR_AMBER
        }
        # Fan drawer status LED
        self.FANLED_MODES = {
            "3" : self.STATUS_LED_COLOR_OFF,
            "2" : self.STATUS_LED_COLOR_GREEN,
            "1" : self.STATUS_LED_COLOR_RED
        }

        self._api_helper=APIHelper()
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_HWMON_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_HWMON_I2C_MAPPING[self.psu_index]['addr']
            self.psu_hwmon_path = PSU_I2C_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

            self.psu_i2c_num = PSU_CPLD_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_CPLD_I2C_MAPPING[self.psu_index]['addr']
            self.psu_cpld_path = PSU_I2C_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

        FanBase.__init__(self)

    def get_name(self):
        """
        Retrieves fan name
        Returns:
            A string with fan name either from fan tray or from psu
        """
        if self.is_psu_fan:
            fan_name = "PSU {} fan {}".format(self.psu_index+1, self.fan_index)
        else:
            fan_name = "FanTray {} fan {}".format(self.fan_tray_index, self.fan_index)

        return fan_name

    def get_model(self):
        """
        Retrieves the fan model
        Returns:
            string: The model of the device

        """
        return "R40W12BGNL9-07T17"

    def get_serial(self):
        """
        Retrieves the fan serial
        Returns:
            string: The serial of the device

        """
        return "N/A"

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if not self.is_psu_fan:
            dir_str = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index+1, '_direction')
            val = self._api_helper.read_txt_file(dir_str)
            if val is not None:
                if val == '0': #F2B
                    direction = self.FAN_DIRECTION_EXHAUST
                else:
                    direction = self.FAN_DIRECTION_INTAKE
            else:
                direction = self.FAN_DIRECTION_EXHAUST

        else: #For PSU
            direction = self.FAN_DIRECTION_EXHAUST # FIX_ME

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        """
        speed = 0
        if self.is_psu_fan:
            psu_fan_path= "{}{}".format(self.psu_hwmon_path, 'psu_fan1_speed_rpm')
            fan_speed_rpm = self._api_helper.read_txt_file(psu_fan_path)
            if fan_speed_rpm is not None:
                speed = (int(fan_speed_rpm, 10)) * 100 / PSU_FAN_MAX_RPM
                if speed > 100:
                    speed = 100
            else:
                return 0

        elif self.get_presence():
            if 0 == self.fan_index:
                speed_rpm = "_front_speed_rpm"
            else:
                speed_rpm = "_rear_speed_rpm"
            speed = self._api_helper.read_txt_file(
                "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index + 1, speed_rpm))
            if speed is None:
                return 0

            speed_max_rpm = TRAY_FRONT_FAN_MAX_RPM
            if self.fan_index == 1:
                speed_max_rpm = TRAY_REAR_FAN_MAX_RPM
            speed = (int(speed, 10)) * 100 / speed_max_rpm
            if speed > 100:
                speed = 100

        return int(speed)

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed_pc = pwm_target/255*100

            0   : when PWM mode is use
            pwm : when pwm mode is not use
        """
        if self.is_psu_fan:
            return 100
        elif self.get_presence():
            speed = 100
            try:
                speed = int(self._api_helper.read_txt_file(FAN_PERCENTAGE_TMP))
            except:
                pass
            return speed
        else:
            return None

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        tolerance = TRAY_FANSPEED_TOLERANCE
        if self.is_psu_fan:
            tolerance = 100
        else:
            if self.get_target_speed() <= TRAY_LOW_FANSPEED_RPM:
                tolerance = TRAY_LOW_FANSPEED_TOLERANCE

        return tolerance

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        """

        if not self.is_psu_fan and self.get_presence():
            speed = int(speed)
            sts1 = self._api_helper.write_txt_file(FAN_PERCENTAGE, speed)
            sts2 = self._api_helper.write_txt_file(FAN_PERCENTAGE_TMP, speed)
            return sts1 and sts2

        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        Notes:
            The following notes are applicable only to fans that are not related to the PSU.
            This function handle the fan drawer LED status, not the fan LED status.
            This fan drawer LED status is managed by default in auto mode via the fan CPLD.
            It can be controlled by software only when the `fan drawer LED mode` is set to
            debug mode.
            This function also manages the front panel fan LED status based on the status of
            all fan drawer LEDs.
        """
        if not self.is_psu_fan:
            sts1 = False

            # This code is executed when the `fan drawer LED mode` is set to debug mode.
            led_mode = self._api_helper.read_txt_file(FAN_LED_MODE)
            if led_mode == LED_DEBUG_MODE_ON:
                filename = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index + 1, '_led')
                for key, val in self.FANLED_MODES.items():
                    if val == color:
                        sts1 = self._api_helper.write_txt_file(filename, key)
            else:
                sts1 = True

            # Set the front panel fan LED based on the status of all fan drawer LEDs
            fp_led_color = 'green'
            fans = platform.Platform().get_chassis().get_all_fans()
            for fan in fans:
                if fan.get_status_led() == 'red':
                    # Force `amber` instead of `red` since there is no option for 'red' in hardware
                    fp_led_color = 'amber'
            sts2 = self.set_front_panel_status_led(fp_led_color)

            return sts1 and sts2
        else:
            if self.get_presence():
                platform_chassis = platform.Platform().get_chassis()
                psu = platform_chassis.get_psu(self.psu_index)
                return psu.set_status_led(color)
            else:
                return False

    def get_status_led(self):
        """
        Gets the state of the fan module status LED
        Args:
            No
        Returns:
            string: representing the color with which is the status of
                   fan modules
        Notes:
            The following notes are applicable only to fans that are not related to the PSU.
            This function read the fan drawer LED status, not the fan LED status.
            This fan drawer LED status is managed by default in auto mode via the fan CPLD.
            It can be controlled by software only when the `fan drawer LED mode` is set to
            debug mode.
            This fanction implements the fan drawer LED status to ensure the correct hardware
            LED indication is displayed via the `show platform fan` command.
        """
        if not self.is_psu_fan:
            filename = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index + 1, '_led')
            key = self._api_helper.read_txt_file(filename)
            return self.FANLED_MODES.get(key, 'N/A')
        else:
            platform_chassis = platform.Platform().get_chassis()
            psu = platform_chassis.get_psu(self.psu_index)
            return psu.get_status_led()

    def set_front_panel_status_led(self, color):
        """
        Sets the state of the front panel fan status LED
        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        for key, val in self.SYSFANLED_MODES.items():
            if val == color:
                return self._api_helper.write_txt_file(FP_FAN_LED_FILE, key)
        return False

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            present_path = "{}{}".format(self.psu_cpld_path, 'psu_present')
        else:
            present_path = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index + 1, '_present')

        val = self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10) == 1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            psu_fan_path = "{}{}".format(self.psu_hwmon_path, 'psu_fan1_speed_rpm')
            val = self._api_helper.read_txt_file(psu_fan_path)
            if val is not None:
                return int(val, 10) != 0
            else:
                return False
        else:
            path = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index + 1, '_fault')
            val = self._api_helper.read_txt_file(path)
            if val is not None:
                return int(val, 10) == 0
            else:
                return False

    @classmethod
    def get_cooling_level(cls):
        """
        Class method
        Returns:
            int: Current cooling level in range 1..10, means cooling level number * 10%
        """
        return cls.current_cooling_level

    @classmethod
    def set_cooling_level(cls, level, cur_state):
        """
        Changes cooling level. The input level should be an integer value 1..10.
        1 means 10%, 2 means 20%, 10 means 100%.
        """
        if not isinstance(level, int):
            raise RuntimeError("Failed to set cooling level, input parameter must be integer")

        if level < cls.MIN_VALID_COOLING_LEVEL or level > cls.MAX_VALID_COOLING_LEVEL:
            raise RuntimeError("Failed to set cooling level, level value must be in range [{}, {}], got {}".format(
                cls.MIN_VALID_COOLING_LEVEL,
                cls.MAX_VALID_COOLING_LEVEL,
                level
                ))
        try:
            from sonic_platform import platform
            platform_chassis = platform.Platform().get_chassis()
            if platform_chassis is not None:
                cls.current_cooling_level = level
                fans_num = platform_chassis.get_num_fans()
                for fan_ins in platform_chassis.get_all_fans():
                    fan_ins.set_speed(level * 10)
            else:
                logger.log_error("Fan: Chassis is not available !")
            # See on top of thermal.py how to enable log_debug
            logger.log_debug("Fan:set_cooling_level: {} x10%".format(str(level)))
        except (ValueError, IOError) as e:
            raise RuntimeError("Failed to set cooling level - {}".format(e))

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        if self.is_psu_fan:
            return (self.psu_index + 1)
        else:
            return (self.fan_index + 1)

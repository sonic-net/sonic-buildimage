#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################



try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
    import os.path
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TARGET_SPEED_PATH = "/tmp/fan_target_speed"
SPEED_TOLERANCE = 10
PSU_FAN_MAX_RPM = 26688
SPEED_TOLERANCE = 15
CPLD_FAN_I2C_PATH = "/sys/bus/i2c/devices/3-0063/fan"
I2C_PATH ="/sys/bus/i2c/devices/{}-00{}/"
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "58"
    },
    1: {
        "num": 12,
        "addr": "5b"
    },
}


PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "50"
    },
    1: {
        "num": 12,
        "addr": "53"
    },
}



FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R",
                 "FAN-3F", "FAN-3R", "FAN-4F", "FAN-4R",
                 "FAN-5F", "FAN-5R"]

FAN_TARGET_SPEED_TABLE = {
    0:   { "front_target_speed": 0      , "rear_target_speed": 0     }, # percentage <--> target_rpm
    5:   { "front_target_speed": 1075   , "rear_target_speed": 900   },
    10:  { "front_target_speed": 2150   , "rear_target_speed": 1800  },
    15:  { "front_target_speed": 3225   , "rear_target_speed": 2700  },
    20:  { "front_target_speed": 4300   , "rear_target_speed": 3600  },
    25:  { "front_target_speed": 5375   , "rear_target_speed": 4500  },
    30:  { "front_target_speed": 6450   , "rear_target_speed": 5400  },
    35:  { "front_target_speed": 7525   , "rear_target_speed": 6300  },
    40:  { "front_target_speed": 8600   , "rear_target_speed": 7200  },
    45:  { "front_target_speed": 9675   , "rear_target_speed": 8100  },
    50:  { "front_target_speed": 10750  , "rear_target_speed": 9000  },
    55:  { "front_target_speed": 11825  , "rear_target_speed": 9900  },
    60:  { "front_target_speed": 12900  , "rear_target_speed": 10800 },
    65:  { "front_target_speed": 13975  , "rear_target_speed": 11700 },
    70:  { "front_target_speed": 15050  , "rear_target_speed": 12600 },
    75:  { "front_target_speed": 16125  , "rear_target_speed": 13500 },
    80:  { "front_target_speed": 17200  , "rear_target_speed": 14400 },
    85:  { "front_target_speed": 18275  , "rear_target_speed": 15300 },
    90:  { "front_target_speed": 19350  , "rear_target_speed": 16200 },
    95:  { "front_target_speed": 20425  , "rear_target_speed": 17100 },
    100: { "front_target_speed": 21500  , "rear_target_speed": 18000 }
}

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self._api_helper=APIHelper()
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_HWMON_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_HWMON_I2C_MAPPING[self.psu_index]['addr']
            self.psu_hwmon_path = I2C_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

            self.psu_i2c_num = PSU_CPLD_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_CPLD_I2C_MAPPING[self.psu_index]['addr']
            self.psu_cpld_path = I2C_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

        FanBase.__init__(self)


    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """


        if not self.is_psu_fan:
            dir_str = "{}{}{}".format(CPLD_FAN_I2C_PATH, self.fan_tray_index+1, '_direction')
            val=self._api_helper.read_txt_file(dir_str)
            if val is not None:
                if int(val, 10)==0:
                    direction=self.FAN_DIRECTION_EXHAUST
                else:
                    direction=self.FAN_DIRECTION_INTAKE
            else:
                direction=self.FAN_DIRECTION_EXHAUST
        else: #For PSU
            dir_str = "{}{}".format(self.psu_hwmon_path,'psu_fan_dir')

            val=self._api_helper.read_txt_file(dir_str)

            if val is not None:
                if val=='F2B':
                    direction=self.FAN_DIRECTION_EXHAUST
                else:
                    direction=self.FAN_DIRECTION_INTAKE
            else:
                direction=self.FAN_DIRECTION_EXHAUST

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
                speed = (int(fan_speed_rpm,10))*100/26688
                if speed > 100:
                    speed=100
            else:
                return 0
        elif self.get_presence():
            fan_target = self._api_helper.read_txt_file("{}{}".format(CPLD_I2C_PATH, '_duty_cycle_percentage'))
            if self.fan_index == 0:
                front_fan_index = self.fan_tray_index + 1 #1~5
                fan_input  = self._api_helper.read_txt_file("{}{}{}".format(CPLD_I2C_PATH, front_fan_index, '_input'))
            else:
                rear_fan_index = self.fan_tray_index + 11 #11~15
                fan_input  = self._api_helper.read_txt_file("{}{}{}".format(CPLD_I2C_PATH, rear_fan_index, '_input'))

            if fan_input is None or fan_target is None:
                return 0

            # When set low_speed, then speed is 0
            if ( FAN_TARGET_SPEED_TABLE[int(fan_target)]["front_target_speed"] == 0 or
                 FAN_TARGET_SPEED_TABLE[int(fan_target)]["rear_target_speed"] == 0 or int(fan_input) == 0 ):
                return 0

            if self.fan_index == 0:
                speed = int(fan_target) * ( int(fan_input) / FAN_TARGET_SPEED_TABLE[int(fan_target)]["front_target_speed"] )
            else:
                speed = int(fan_target) * ( int(fan_input) / FAN_TARGET_SPEED_TABLE[int(fan_target)]["rear_target_speed"] )

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
        speed = 0
        if self.is_psu_fan:
            return self.get_speed()
        elif self.get_presence():
            if os.path.isfile(TARGET_SPEED_PATH) and self.is_host is False: #docker
                speed=self._api_helper.read_txt_file(TARGET_SPEED_PATH)
            else:
                speed_path = "{}{}".format(CPLD_I2C_PATH, '_duty_cycle_percentage')
                speed=self._api_helper.read_txt_file(speed_path)
            if speed is None:
                return 0
        return int(speed)


    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        if os.path.isfile(TARGET_SPEED_PATH):
            target_speed  = self._api_helper.read_txt_file("{}{}".format(CPLD_I2C_PATH, '_duty_cycle_percentage'))
            set_speed_val = self._api_helper.read_txt_file(TARGET_SPEED_PATH)
            speed_sub     = abs(int(target_speed) - int(set_speed_val))
        else:
            speed_sub = 0

        return SPEED_TOLERANCE + speed_sub


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
            speed_path = "{}{}".format(CPLD_FAN_I2C_PATH, '_duty_cycle_percentage')
            ret = self._api_helper.write_txt_file(speed_path, int(speed))
            if ret == True:
                self._api_helper.write_txt_file(TARGET_SPEED_PATH, int(speed))
            return ret

        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False #Not supported

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.is_psu_fan:
            return None

        return {
            True: self.STATUS_LED_COLOR_GREEN,
            False: self.STATUS_LED_COLOR_RED
        }.get(self.get_status(), self.STATUS_LED_COLOR_RED)

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_NAME_LIST[self.fan_tray_index*2 + self.fan_index] \
            if not self.is_psu_fan \
            else "PSU-{} FAN-{}".format(self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            present_path="{}{}".format(self.psu_cpld_path, 'psu_present')
        else:
            present_path = "{}{}{}".format(CPLD_FAN_I2C_PATH, self.fan_tray_index+1, '_present')

        val=self._api_helper.read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            psu_path = "{}{}".format(self.psu_cpld_path, 'psu_power_good')
            val = self._api_helper.read_txt_file(psu_path)
            if val is None or int(val, 10)==0:
                return False

            psu_fan_path= "{}{}".format(self.psu_hwmon_path, 'psu_fan1_fault')
            val=self._api_helper.read_txt_file(psu_fan_path)
            if val is not None:
                return int(val, 10)==0
            else:
                return False
        else:
            sys_index = self.fan_index * 10 + self.fan_tray_index+1
            path = "{}{}{}".format(CPLD_FAN_I2C_PATH, sys_index, '_fault')
            val=self._api_helper.read_txt_file(path)
            if val is not None:
                return int(val, 10)==0
            else:
                return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return (self.fan_index+1) \
            if not self.is_psu_fan else (self.psu_index+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False


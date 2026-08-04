#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_MAX_RPM = 25500
PSU_FAN_MAX_RPM = 25500
SPEED_TOLERANCE = 15
CPLD_I2C_PATH = "/sys/bus/i2c/devices/11-0066/fan"
PSU_I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"
PSU_HWMON_I2C_MAPPING = {
    0: {
        "bus": 17,
        "addr": "59"
    },
    1: {
        "bus": 13,
        "addr": "5b"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "bus": 17,
        "addr": "51"
    },
    1: {
        "bus": 13,
        "addr": "53"
    },
}

FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R",
                 "FAN-3F", "FAN-3R", "FAN-4F", "FAN-4R", 
                 "FAN-5F", "FAN-5R", "FAN-6F", "FAN-6R"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self,
                 fan_tray_index,
                 fan_index=0,
                 is_psu_fan=False,
                 psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self.psu_index = psu_index

        if self.is_psu_fan:
            psu_i2c_bus = PSU_HWMON_I2C_MAPPING[psu_index]["bus"]
            psu_i2c_addr = PSU_HWMON_I2C_MAPPING[psu_index]["addr"]
            self.psu_hwmon_path = PSU_I2C_PATH.format(psu_i2c_bus,
                                                      psu_i2c_addr)
            psu_i2c_bus = PSU_CPLD_I2C_MAPPING[psu_index]["bus"]
            psu_i2c_addr = PSU_CPLD_I2C_MAPPING[psu_index]["addr"]
            self.cpld_path = PSU_I2C_PATH.format(psu_i2c_bus, psu_i2c_addr)

        FanBase.__init__(self)

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                return fd.read().strip()
        except IOError:
            pass
        return ""

    def __write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if not self.is_psu_fan:
            val = self.__read_txt_file(
                CPLD_I2C_PATH + str(self.fan_tray_index+1) + "_direction")
            direction = self.FAN_DIRECTION_EXHAUST if (
                val == "0") else self.FAN_DIRECTION_INTAKE
        else:
            val = self.__read_txt_file(self.psu_hwmon_path + "psu_fan_dir")
            direction = self.FAN_DIRECTION_EXHAUST if (
                val == "F2B") else self.FAN_DIRECTION_INTAKE
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
            speed = self.__read_txt_file(
                self.psu_hwmon_path + "psu_fan1_speed_rpm")
            speed = (int(speed, 10)) * 100 / PSU_FAN_MAX_RPM
            speed = 100 if (speed > 100) else speed
        elif self.get_presence():
            speed = self.__read_txt_file(CPLD_I2C_PATH + str(
                self.fan_index * 10 + self.fan_tray_index + 1) + "_input")
            speed = (int(speed, 10)) * 100 / FAN_MAX_RPM
            speed = 100 if (speed > 100) else speed
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
        return self.get_speed()

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return SPEED_TOLERANCE

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
            return self.__write_txt_file(
                CPLD_I2C_PATH + "_duty_cycle_percentage", int(speed))

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
        return False  #Not supported

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status=self.get_presence()
        if status is None:
            return  self.STATUS_LED_COLOR_OFF

        return {
            1: self.STATUS_LED_COLOR_GREEN,
            0: self.STATUS_LED_COLOR_RED            
        }.get(status, self.STATUS_LED_COLOR_OFF)

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
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        if self.is_psu_fan:
            val = self.__read_txt_file(self.cpld_path + "psu_present")
            return int(val, 10) == 1

        val = self.__read_txt_file(
            CPLD_I2C_PATH + str(self.fan_tray_index + 1) + "_present")
        return int(val, 10)==1

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            psu_fan_path= "{}{}".format(self.psu_hwmon_path, 'psu_fan1_fault')
            val=self.__read_txt_file(psu_fan_path)
            if val is not None:
                return int(val, 10)==0
            else:
                return False
        else:    
            path = "{}{}{}".format(CPLD_I2C_PATH, self.fan_tray_index+1, '_fault')
            val=self.__read_txt_file(path)
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

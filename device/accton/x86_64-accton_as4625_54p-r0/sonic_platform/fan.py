#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################
import glob

try:
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_FAN_MAX_RPM = 25500
SPEED_TOLERANCE = 15

FAN_HWMON_I2C_PATH = "/sys/devices/platform/as4625_fan/hwmon/hwmon*/fan"

I2C_PATH ="/sys/bus/i2c/devices/{}-00{}/"
PSU_PMBUS_I2C_MAPPING = {
    0: {
        "num": 8,
        "addr": "58"
    },
    1: {
        "num": 9,
        "addr": "59"
    }
}

PSU_EEPROM_I2C_MAPPING = {
    0: {
        "num": 8,
        "addr": "50"
    },
    1: {
        "num": 9,
        "addr": "51"
    }
}

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan

        if not self.is_psu_fan:
            self.hwmon_path = FAN_HWMON_I2C_PATH
        else:
            self.psu_index = psu_index
            i2c_num = PSU_PMBUS_I2C_MAPPING[self.psu_index]['num']
            i2c_addr = PSU_PMBUS_I2C_MAPPING[self.psu_index]['addr']
            self.hwmon_path = I2C_PATH.format(i2c_num, i2c_addr)

            i2c_num = PSU_EEPROM_I2C_MAPPING[self.psu_index]['num']
            i2c_addr = PSU_EEPROM_I2C_MAPPING[self.psu_index]['addr']
            self.psu_eeprom_path = I2C_PATH.format(i2c_num, i2c_addr)

        FanBase.__init__(self)

    def __read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'r') as fd:
                    data = fd.readline().rstrip()
                    if len(data) > 0:
                        return data
            except IOError as e:
                pass

        return None

    def __write_txt_file(self, file_path, value):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'w') as fd:
                    fd.write(str(value))
            except IOError:
                return False
        return True

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        if self.is_psu_fan: #For PSU
            dir_str = "{}{}".format(self.psu_eeprom_path,'psu_fan_dir')
        else:
            dir_str = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_dir')

        val = self.__read_txt_file(dir_str)
        if val is not None:
            if val == 'B2F':
                direction = self.FAN_DIRECTION_INTAKE
            else:
                direction = self.FAN_DIRECTION_EXHAUST
        else:
            direction = self.FAN_DIRECTION_NOT_APPLICABLE

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
            path = "{}{}".format(self.hwmon_path, 'fan1_input')
            rpm = self.__read_txt_file(path)
            if rpm is not None:
                speed = (int(rpm, 10)) * 100 / PSU_FAN_MAX_RPM
                if speed > 100:
                    speed = 100
            else:
                return 0

        else:
            input_path = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_input')
            input = self.__read_txt_file(input_path)
            target_rpm_path = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_target_rpm')
            target_rpm = self.__read_txt_file(target_rpm_path)
            pwm = self.get_target_speed()

            if input is None or target_rpm is None or pwm is None:
                return 0

            if int(target_rpm) > 0:
                speed = pwm * int(input) / int(target_rpm)
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
        if not self.is_psu_fan and self.get_presence():
            speed_path = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_pwm')
            speed = self.__read_txt_file(speed_path)
            if speed is None:
                return 0
            return int(speed)
        else:
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
            speed_path = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_pwm')
            return self.__write_txt_file(speed_path, int(speed))

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
        if not self.is_psu_fan:
            return None

        if self.get_presence() is not True:
            return None

        return {
            True: self.STATUS_LED_COLOR_GREEN,
            False: self.STATUS_LED_COLOR_AMBER
        }.get(self.get_status(), self.STATUS_LED_COLOR_AMBER)

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        if self.is_psu_fan:
            return "PSU-{} FAN-{}".format(self.psu_index+1, self.fan_index+1)

        return "FAN-{}".format(self.fan_tray_index+1)

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            present_path = "{}{}".format(self.psu_eeprom_path, 'psu_present')
            val = self.__read_txt_file(present_path)
            if val is not None:
                return int(val, 10) == 1
            else:
                return False
        else:
            return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            if self.get_presence() is not True:
                return None

            power_path = "{}{}".format(self.psu_eeprom_path, 'psu_power_good')
            val = self.__read_txt_file(power_path)
            if val is not None:
                if int(val, 10) != 1:
                    return False

            psu_fan_path= "{}{}".format(self.hwmon_path, 'fan1_input')
            val=self.__read_txt_file(psu_fan_path)
            if val is not None:
                return int(val, 10) != 0
            else:
                return False
        else:
            path = "{}{}{}".format(self.hwmon_path, self.fan_tray_index+1, '_fault')
            val=self.__read_txt_file(path)
            if val is not None:
                return int(val, 10) == 0
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

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

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
        return (self.fan_tray_index+1) \
            if not self.is_psu_fan else (self.psu_index+1)

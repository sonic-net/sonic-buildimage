#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################


import os.path
import time

try:
    from sonic_platform.eeprom import Tlv
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_FAN_MAX_RPM = 18000
SPEED_TOLERANCE = 15
SYSFS_PATH = "/sys/bus/i2c/devices/{0}-00{1}"
CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/0-0033"

HWMON_PATH = "/sys/bus/i2c/devices/{0}-00{1}/hwmon"
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "59"
    },
    1: {
        "num": 3,
        "addr": "58"
    },
}

FAN_HWMON_I2C_MAPPING = {
    0: {
        "num": 2,
        "addr": "2f"
    },
}

#FAN_PCA9539_I2C_MAPPING = {
#    0: {
#        "num": 9,
#        "addr": "74"
#    },
#    1: {
#        "num": 9,
#        "addr": "76"
#    },
#}

# 3 fan sku not exist now
#FAN_ROTOR_MAPPING = {
#    0: 1,
#    1: 8,
#    2: 4
#}

#FAN_NAME_LIST = ["FAN-1", "FAN-2", "FAN-3"]

FAN_ROTOR_MAPPING = {
    0: 4,
    1: 8
}

FAN_NAME_LIST = ["FAN-1", "FAN-2"]

class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self.pwm_conv=[]

        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_HWMON_I2C_MAPPING[self.psu_index]['num']
            self.psu_i2c_addr = PSU_HWMON_I2C_MAPPING[self.psu_index]['addr']
            hwmon_dir = self.__search_hwmon_dir_name(HWMON_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr))
            self.psu_hwmon_path = os.path.join(HWMON_PATH.format(self.psu_i2c_num, self.psu_i2c_addr), hwmon_dir)

            self.psu_status_path = CPLD_SYSFS_DIR
        else:
            self.fan_i2c_num = FAN_HWMON_I2C_MAPPING[0]['num']
            self.fan_i2c_addr = FAN_HWMON_I2C_MAPPING[0]['addr']
            hwmon_dir = self.__search_hwmon_dir_name(HWMON_PATH.format(
                self.fan_i2c_num, self.fan_i2c_addr))
            self.fan_hwmon_path = os.path.join(HWMON_PATH.format(self.fan_i2c_num, self.fan_i2c_addr), hwmon_dir)

            self.fan_status_path = CPLD_SYSFS_DIR

#            self.an_fan_i2c_num = FAN_HWMON_I2C_MAPPING[1]['num']
#            self.an_fan_i2c_addr = FAN_HWMON_I2C_MAPPING[1]['addr']
#            hwmon_dir = self.__search_hwmon_dir_name(HWMON_PATH.format(
#                self.fan_i2c_num, self.fan_i2c_addr))
#            self.an_fan_hwmon_path = os.path.join(HWMON_PATH.format(self.fan_i2c_num, self.fan_i2c_addr), hwmon_dir)

#            self.fan_sts_i2c_num = FAN_PCA9539_I2C_MAPPING[0 if fan_tray_index < 3 else 1]['num']
#            self.fan_sts_i2c_addr = FAN_PCA9539_I2C_MAPPING[0 if fan_tray_index < 3 else 1]['addr']
#            self.fan_status_path = SYSFS_PATH.format(self.fan_sts_i2c_num, self.fan_sts_i2c_addr)


        self._eeprom = Tlv()

        for i in range(0, 101):
            self.pwm_conv.append(int(i * 255 / 100))

        FanBase.__init__(self)

    def __search_hwmon_dir_name(self, directory):
        try:
            dirs = os.listdir(directory)
            for file in dirs:
                if file.startswith("hwmon"):
                    return file
        except:
            pass
        return ''

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def __write_txt_file(self, file_path, data):
        try:
            with open(file_path, 'w') as fd:
                fd.write(data)
        except IOError:
            pass
        return None

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = 0
        if self.is_psu_fan:
            if self.get_presence():
                path_pre=SYSFS_PATH.format(self.psu_i2c_num, self.psu_i2c_addr)
                psu_fan_path= "{}/{}".format(path_pre, 'mfr_model')
                model=self.__read_txt_file(psu_fan_path)
                if model is None:
                    direction = 0
                direction = 0 if 'WNA' in model else 1
        elif self.get_presence():
            part=self._eeprom.get_model()
            direction = 0 if 'ES-1227-54TS-O' in part else 1

        return self.FAN_DIRECTION_EXHAUST if int(direction) == 0 else self.FAN_DIRECTION_INTAKE

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        """
        speed = 0
        if self.is_psu_fan:
            psu_fan_path= "{}/{}".format(self.psu_hwmon_path, 'fan1_input')
            rpm=self.__read_txt_file(psu_fan_path)
            if rpm is None:
                return 0
            speed = int(int(rpm) * 100 / PSU_FAN_MAX_RPM)
        elif self.get_presence():
            path= "{}/pwm1".format(self.fan_hwmon_path)
            pwm=self.__read_txt_file(path)
            if pwm is None:
                return 0
            elif int(pwm) == 77:
                pwm = 76
            speed = self.pwm_conv.index(int(pwm))

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
            pwm = int((speed * 255) / 100)
            speed_path = "{}/{}".format(self.fan_hwmon_path, 'pwm1')
            self.__write_txt_file(speed_path, str(pwm))
#            speed_path = "{}/{}".format(self.an_fan_hwmon_path, 'pwm1')
#            self.__write_txt_file(speed_path, int(speed))

            return True

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
        if color is self.STATUS_LED_COLOR_GREEN:
            val = 1
        else:
            val = 2

        led_path="{}/fan{}_led".format(self.fan_status_path, self.fan_tray_index+1)
        self.__write_txt_file(led_path, str(val))

        return True

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
        fan_name = FAN_NAME_LIST[self.fan_tray_index + self.fan_index] \
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
            present_path="{}/psu{}_present".format(self.psu_status_path, self.psu_index+1)
        else:
            present_path="{}/cnfan{}_present".format(self.fan_status_path, self.fan_tray_index+1)

        val=self.__read_txt_file(present_path)
        if val is not None:
            return int(val, 10)==1
        else:
            return False

    def try_clear_fault(self):
        path= "{}/pwm1".format(self.fan_hwmon_path)
        speed=self.__read_txt_file(path)

        self.__write_txt_file(path, str(speed))
        time.sleep(2)

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu_fan:
            psu_fan_path= "{}/{}".format(self.psu_hwmon_path, 'fan1_fault')
            val=self.__read_txt_file(psu_fan_path)
            if val is not None:
                return int(val, 10)==0
            else:
                return False
        else:
            path= "{}/fan{}_fault".format(self.fan_hwmon_path, FAN_ROTOR_MAPPING[self.fan_tray_index + self.fan_index])
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
        return (self.fan_tray_index+1) \
            if not self.is_psu_fan else (self.psu_index+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False


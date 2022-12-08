#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

from __future__ import division
import math
import os.path

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

EMC2305_PATH = "/sys/bus/i2c/drivers/emc2305/"
GPIO_DIR = "/sys/class/gpio"
GPIO_LABEL = "pca9505"
EMC2305_MAX_PWM = 255
EMC2305_FAN_PWM = "pwm{}"
EMC2305_FAN_TARGET = "fan{}_target"
EMC2305_FAN_INPUT = "pwm{}"
FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R",
                 "FAN-3F", "FAN-3R", "FAN-4F", "FAN-4R", "FAN-5F", "FAN-5R"]
FAN_SPEED_TOLERANCE = 10
PSU_FAN_MAX_RPM = 11000
PSU_HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
PSU_I2C_MAPPING = {
    0: {
        "num": 10,
        "addr": "5a"
    },
    1: {
        "num": 11,
        "addr": "5b"
    },
}
NULL_VAL = "N/A"


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        FanBase.__init__(self)
        self.fan_index = fan_index
        self._api_helper = APIHelper()
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_I2C_MAPPING[self.psu_index]["num"]
            self.psu_i2c_addr = PSU_I2C_MAPPING[self.psu_index]["addr"]
            self.psu_hwmon_path = PSU_HWMON_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

        # dx010 fan attributes
        # Two EMC2305s located at i2c-13-4d and i2c-13-2e
        # to control a dual-fan module.
        self.emc2305_chip_mapping = [
            {
                'device': "13-002e",
                'index_map': [2, 1, 4, 5, 3]
            },
            {
                'device': "13-004d",
                'index_map': [2, 4, 5, 3, 1]
            }
        ]
        self.dx010_fan_gpio = [
            {'base': self.__get_gpio_base()},
            {'prs': 11, 'dir': 16, 'color': {'red': 31, 'green': 32}},  # 1
            {'prs': 10, 'dir': 15, 'color': {'red': 29, 'green': 30}},  # 2
            {'prs': 13, 'dir': 18, 'color': {'red': 35, 'green': 36}},  # 3
            {'prs': 14, 'dir': 19, 'color': {'red': 37, 'green': 38}},  # 4
            {'prs': 12, 'dir': 17, 'color': {'red': 33, 'green': 34}},  # 5
        ]

    def __write_txt_file(self, file_path, value):
        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def __search_file_by_name(self, directory, file_name):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name in file_name:
                    return file_path
        return None

    def __get_gpio_base(self):
        for r in os.listdir(GPIO_DIR):
            label_path = os.path.join(GPIO_DIR, r, "device/name")
            if "gpiochip" in r and GPIO_LABEL in \
                    self._api_helper.read_txt_file(label_path):
                return int(r[8:], 10)
        return 216  # Reserve

    def __get_gpio_value(self, pinnum):
        gpio_base = self.dx010_fan_gpio[0]['base']
        gpio_dir = GPIO_DIR + '/gpio' + str(gpio_base+pinnum)
        gpio_file = gpio_dir + "/value"
        retval = self._api_helper.read_txt_file(gpio_file)
        return retval.rstrip('\r\n')

    def __set_gpio_value(self, pinnum, value=0):
        gpio_base = self.dx010_fan_gpio[0]['base']
        gpio_dir = GPIO_DIR + '/gpio' + str(gpio_base+pinnum)
        gpio_file = gpio_dir + "/value"
        return self.__write_txt_file(gpio_file, value)

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_EXHAUST
        if not self.is_psu_fan:
            raw = self.__get_gpio_value(
                self.dx010_fan_gpio[self.fan_tray_index+1]['dir'])

            direction = self.FAN_DIRECTION_INTAKE if int(
                raw, 10) == 0 else self.FAN_DIRECTION_EXHAUST

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            speed = pwm_in/255*100
        """
        speed = 0
        if self.is_psu_fan:
            fan_speed_sysfs_name = "fan{}_input".format(self.fan_index+1)
            fan_speed_sysfs_path = self.__search_file_by_name(
                self.psu_hwmon_path, fan_speed_sysfs_name)
            fan_speed_rpm = self._api_helper.read_txt_file(
                fan_speed_sysfs_path) or 0
            speed = math.ceil(float(fan_speed_rpm) * 100 / PSU_FAN_MAX_RPM)
        elif self.get_presence():
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, EMC2305_FAN_INPUT)
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            raw = self._api_helper.read_txt_file(sysfs_path).strip('\r\n')
            pwm = int(raw, 10) if raw else 0
            speed = math.ceil(float(pwm * 100 / EMC2305_MAX_PWM))

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
        target = NULL_VAL

        if not self.is_psu_fan:
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, EMC2305_FAN_PWM)
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            pwm = self._api_helper.read_txt_file(sysfs_path)
            target = round(int(pwm) / 255 * 100.0)

        return target

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return FAN_SPEED_TOLERANCE

    def set_speed(self, speed):
        """
        Sets the fan speed
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)
        Returns:
            A boolean, True if speed is set successfully, False if not

        Note:
            Depends on pwm or target mode is selected:
            1) pwm = speed_pc * 255             <-- Currently use this mode.
            2) target_pwm = speed_pc * 100 / 255
             2.1) set pwm{}_enable to 3

        """
        pwm = speed * 255 / 100
        if not self.is_psu_fan and self.get_presence():
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, EMC2305_FAN_PWM)
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            return self.__write_txt_file(sysfs_path, int(pwm))

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
        set_status_led = False

        s1_gpio = self.dx010_fan_gpio[self.fan_tray_index+1]['color']['red']
        s2_gpio = self.dx010_fan_gpio[self.fan_tray_index+1]['color']['green']

        if not self.is_psu_fan:
            try:
                if color == self.STATUS_LED_COLOR_GREEN:
                    s1 = self.__set_gpio_value(s1_gpio, 1)
                    s2 = self.__set_gpio_value(s2_gpio, 0)

                elif color == self.STATUS_LED_COLOR_RED:
                    s1 = self.__set_gpio_value(s1_gpio, 0)
                    s2 = self.__set_gpio_value(s2_gpio, 1)

                elif color == self.STATUS_LED_COLOR_OFF:
                    s1 = self.__set_gpio_value(s1_gpio, 1)
                    s2 = self.__set_gpio_value(s2_gpio, 1)

                elif color == self.STATUS_LED_COLOR_AMBER:
                    s1 = self.__set_gpio_value(s1_gpio, 0)
                    s2 = self.__set_gpio_value(s2_gpio, 0)
                else:
                    s1, s2 = True, True

                set_status_led = s1 and s2

            except IOError:
                return False

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        s1 = self.__get_gpio_value(
            self.dx010_fan_gpio[self.fan_tray_index+1]['color']['red'])
        s2 = self.__get_gpio_value(
            self.dx010_fan_gpio[self.fan_tray_index+1]['color']['green'])

        return {
            '10': self.STATUS_LED_COLOR_GREEN,
            '01': self.STATUS_LED_COLOR_RED,
            '00': self.STATUS_LED_COLOR_AMBER
        }.get(s1+s2, self.STATUS_LED_COLOR_OFF)

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

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
        present_str = self.__get_gpio_value(
            self.dx010_fan_gpio[self.fan_tray_index+1]['prs'])

        return int(present_str, 10) == 0 if not self.is_psu_fan else True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            return NULL_VAL

        model = NULL_VAL
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if self.is_psu_fan:
            return NULL_VAL

        serial = NULL_VAL
        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = 1
        if self.is_psu_fan:
            fan_fault_sysfs_name = "fan1_fault"
            fan_fault_sysfs_path = self.__search_file_by_name(
                self.psu_hwmon_path, fan_fault_sysfs_name)
            status = self._api_helper.read_one_line_file(fan_fault_sysfs_path)

        elif self.get_presence():
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, 'fan{}_fault')
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            status = self._api_helper.read_one_line_file(sysfs_path)

        return False if int(status) != 0 else True

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
        return (self.fan_tray_index*2 + self.fan_index + 1) \
            if not self.is_psu_fan else (self.fan_index+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False

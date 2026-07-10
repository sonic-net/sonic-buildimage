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
    from .common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

EMC2305_PATH = "/sys/bus/i2c/drivers/emc2305/"
FAN_PATH = "/sys/devices/platform/e1031.smc/"
EMC2305_MAX_PWM = 255
EMC2305_FAN_PWM = "pwm{}"
EMC2305_FAN_TARGET = "fan{}_target"
EMC2305_FAN_PWM_MODE = "pwm{}_enable"
EMC2305_FAN_INPUT = "pwm{}"
FAN_NAME_LIST = ["FAN-1", "FAN-2", "FAN-3"]
FAN_SPEED_TOLERANCE = 10
PSU_FAN_MAX_RPM = 11000
PSU_HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
PSU_I2C_MAPPING = {
    0: {
        "num": 13,
        "addr": "5b"
    },
    1: {
        "num": 12,
        "addr": "5a"
    },
}
NULL_VAL = 'N/A'


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        FanBase.__init__(self)

        self._api_common = Common()
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.psu_i2c_num = PSU_I2C_MAPPING[self.psu_index]["num"]
            self.psu_i2c_addr = PSU_I2C_MAPPING[self.psu_index]["addr"]
            self.psu_hwmon_path = PSU_HWMON_PATH.format(
                self.psu_i2c_num, self.psu_i2c_addr)

        # e1031 fan attributes
        # Single emc2305 chip located at i2c-23-4d
        # to control a fan module
        self.emc2305_chip_mapping = [
            {
                'device': "23-004d",
                'index_map': [1, 2, 4]
            }
        ]
        self.fan_e1031_presence = "fan{}_prs"
        self.fan_e1031_direction = "fan{}_dir"
        self.fan_e1031_led = "fan{}_led"
        self.fan_e1031_led_col_map = {
            self.STATUS_LED_COLOR_GREEN: "green",
            self.STATUS_LED_COLOR_AMBER: "amber",
            self.STATUS_LED_COLOR_OFF: "off"
        }

    def __search_file_by_name(self, directory, file_name):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name in file_name:
                    return file_path
        return None

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_EXHAUST
        if not self.is_psu_fan:
            fan_direction_file = (FAN_PATH +
                                  self.fan_e1031_direction.format(self.fan_tray_index+1))
            raw = self._api_common.read_txt_file(
                fan_direction_file).strip('\r\n')
            direction = self.FAN_DIRECTION_INTAKE if str(
                raw).upper() == "F2B" else self.FAN_DIRECTION_EXHAUST

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
            fan_speed_rpm = self._api_common.read_txt_file(
                fan_speed_sysfs_path) or 0
            speed = math.ceil(float(fan_speed_rpm) * 100 / PSU_FAN_MAX_RPM)
        elif self.get_presence():
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, EMC2305_FAN_INPUT)
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            raw = self._api_common.read_txt_file(sysfs_path).strip('\r\n')
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
        target = 0
        if not self.is_psu_fan:
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']

            enable_path = "%s%s/%s" % (
                EMC2305_PATH, device, EMC2305_FAN_PWM_MODE)
            enable = self._api_common.read_txt_file(
                enable_path.format(fan_index[self.fan_tray_index]))

            target_mode = EMC2305_FAN_TARGET if enable != "0" else EMC2305_FAN_PWM
            target_path = "%s%s/%s" % (EMC2305_PATH, device,
                                       target_mode.format(fan_index[self.fan_tray_index]))

            raw = self._api_common.read_txt_file(target_path)
            pwm = int(raw, 10) if raw else 0
            target = math.ceil(float(pwm) * 100 / EMC2305_MAX_PWM)

        return int(target)

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
            return self._api_common.write_txt_file(sysfs_path, int(pwm))

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
        if not self.is_psu_fan:
            fan_led_file = (FAN_PATH +
                            self.fan_e1031_led.format(self.fan_tray_index+1))

            set_status_led = self._api_common.write_txt_file(
                fan_led_file, self.fan_e1031_led_col_map[color]) if self.get_presence() else False

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        led = self.STATUS_LED_COLOR_GREEN
        if not self.is_psu_fan:
            fan_led_file = (FAN_PATH +
                            self.fan_e1031_led.format(self.fan_tray_index+1))

            led = self._api_common.read_txt_file(fan_led_file)
        return {
            'green': self.STATUS_LED_COLOR_GREEN,
            'off': self.STATUS_LED_COLOR_OFF,
            'amber': self.STATUS_LED_COLOR_AMBER
        }.get(led, self.STATUS_LED_COLOR_OFF)

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        fan_name = FAN_NAME_LIST[self.fan_tray_index] if not self.is_psu_fan else "PSU-{} FAN-{}".format(
            self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        fan_direction_file = (FAN_PATH +
                              self.fan_e1031_presence.format(self.fan_tray_index+1))
        present_str = self._api_common.read_txt_file(fan_direction_file) or '1'

        return int(present_str) == 0 if not self.is_psu_fan else True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return NULL_VAL

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return NULL_VAL

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
            status = self._api_common.read_one_line_file(fan_fault_sysfs_path)

        elif self.get_presence():
            chip = self.emc2305_chip_mapping[self.fan_index]
            device = chip['device']
            fan_index = chip['index_map']
            sysfs_path = "%s%s/%s" % (
                EMC2305_PATH, device, 'fan{}_fault')
            sysfs_path = sysfs_path.format(fan_index[self.fan_tray_index])
            status = self._api_common.read_one_line_file(sysfs_path)

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

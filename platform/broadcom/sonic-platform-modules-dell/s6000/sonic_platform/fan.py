#!/usr/bin/env python

########################################################################
# DellEMC S6000
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform
#
########################################################################


try:
    import os
    import glob
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_S6000_PSU_FAN_SPEED = 18000
MAX_S6000_FAN_SPEED = 19000
MAX_S6000_FAN_TARGET_SPEED = 18900

# Each element corresponds to required speed (in RPM)
# for a given system thermal level
THERMAL_LEVEL_PSU_FAN_SPEED = (7200, 10800, 14400, 16200, 18000)
THERMAL_LEVEL_FAN_SPEED = (7000, 10000, 13000, 16000, 19000)


class Fan(FanBase):
    """DellEMC Platform-specific Fan class"""

    I2C_DIR = "/sys/bus/i2c/devices/"
    FAN_DEV_MAPPING = {
        1: {1: ("i2c-11/11-002a", 1), 2: ("i2c-11/11-002a", 2)},
        2: {1: ("i2c-11/11-0029", 3), 2: ("i2c-11/11-0029", 4)},
        3: {1: ("i2c-11/11-0029", 1), 2: ("i2c-11/11-0029", 2)}
    }

    def __init__(self, fantray_index=1, fan_index=1,
                 psu_index=1, psu_fan=False, dependency=None):
        FanBase.__init__(self)
        self._target_speed = None
        self.is_psu_fan = psu_fan
        self.is_driver_initialized = True

        if not self.is_psu_fan:
            # Fan is 1-based in DellEMC platforms
            self.fantray_index = fantray_index
            self.index = fan_index
            self.dependency = dependency

            hwmon_dir = self.I2C_DIR +\
                    "{}/hwmon/".format(self.FAN_DEV_MAPPING[fantray_index][fan_index][0])
            hwmon_node = os.listdir(hwmon_dir)[0]
            self.fan_status_reg = hwmon_dir + hwmon_node +\
                    "/fan{}_alarm".format(self.FAN_DEV_MAPPING[fantray_index][fan_index][1])
            self.get_fan_speed_reg = hwmon_dir + hwmon_node +\
                    "/fan{}_input".format(self.FAN_DEV_MAPPING[fantray_index][fan_index][1])
            self.set_fan_speed_reg = hwmon_dir + hwmon_node +\
                    "/fan{}_target".format(self.FAN_DEV_MAPPING[fantray_index][fan_index][1])
            self.max_fan_speed = MAX_S6000_FAN_SPEED
            self.thermal_level_to_speed = THERMAL_LEVEL_FAN_SPEED
        else:
            self.psu_index = psu_index
            self.index = 1
            self.dependency = dependency
            self.set_fan_speed_reg = self.I2C_DIR +\
                    "i2c-1/1-005{}/fan1_target".format(10 - self.psu_index)

            hwmon_dir = self.I2C_DIR +\
                    "i2c-1/1-005{}/hwmon/".format(10 - self.psu_index)
            try:
                hwmon_node = os.listdir(hwmon_dir)[0]
            except OSError:
                hwmon_node = "hwmon*"
                self.is_driver_initialized = False

            self.get_fan_speed_reg = hwmon_dir + hwmon_node + '/fan1_input'
            self.max_fan_speed = MAX_S6000_PSU_FAN_SPEED
            self.thermal_level_to_speed = THERMAL_LEVEL_PSU_FAN_SPEED

    def _get_i2c_register(self, reg_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if not self.is_driver_initialized:
            reg_file_path = glob.glob(reg_file)
            if len(reg_file_path):
                reg_file = reg_file_path[0]
                self._get_sysfs_path()
            else:
                return rv

        if (not os.path.isfile(reg_file)):
            return rv

        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _set_i2c_register(self, reg_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv

        try:
            with open(reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except:
            rv = 'ERR'

        return rv

    def _get_sysfs_path(self):
        fan_speed_reg = glob.glob(self.get_fan_speed_reg)

        if len(fan_speed_reg):
            self.get_fan_speed_reg = fan_speed_reg[0]
            self.is_driver_initialized = True

    def _get_speed_to_percentage(self, speed):
        speed_percent = (100 * speed) // self.max_fan_speed
        return speed_percent if speed_percent <= 100 else 100

    def _get_target_speed_rpm(self):
        target_speed_rpm = self._get_i2c_register(self.set_fan_speed_reg)
        if (target_speed_rpm != 'ERR') and self.get_presence():
            target_speed_rpm = int(target_speed_rpm, 10)
        else:
            target_speed_rpm = 0

        return target_speed_rpm

    def _set_speed_rpm(self, speed):
        if not self.is_psu_fan:
            if speed > MAX_S6000_FAN_TARGET_SPEED:
                speed = MAX_S6000_FAN_TARGET_SPEED
            self._target_speed = speed

        rv = self._set_i2c_register(self.set_fan_speed_reg, speed)
        if (rv != 'ERR'):
            return True
        else:
            return False

    def get_name(self):
        """
        Retrieves the name of the Fan

        Returns:
            string: The name of the Fan
        """
        if not self.is_psu_fan:
            return "FanTray{}-Fan{}".format(self.fantray_index, self.index)
        else:
            return "PSU{} Fan".format(self.psu_index)

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        return self.dependency.get_presence()

    def get_model(self):
        """
        Retrieves the part number of the Fan
        Returns:
            string: Part number of Fan
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the Fan
        Returns:
            string: Serial number of Fan
        """
        return 'NA'

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """
        status = False
        if self.is_psu_fan:
            fan_speed = self._get_i2c_register(self.get_fan_speed_reg)
            if (fan_speed != 'ERR'):
                if (int(fan_speed) > 1000):
                    status = True
        else:
            fan_status = self._get_i2c_register(self.fan_status_reg)
            if (fan_status != 'ERR'):
                fan_status = int(fan_status, 10)
                if ~fan_status & 0b1:
                    status = True

        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_direction(self):
        """
        Retrieves the fan airflow direction

        Returns:
            A string, either FAN_DIRECTION_INTAKE or
            FAN_DIRECTION_EXHAUST depending on fan direction

        Notes:
            In DellEMC platforms,
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        if self.is_psu_fan:
            direction = {1: self.FAN_DIRECTION_EXHAUST, 2: self.FAN_DIRECTION_INTAKE,
                         3: self.FAN_DIRECTION_EXHAUST, 4: self.FAN_DIRECTION_INTAKE}
        else:
            direction = {1: self.FAN_DIRECTION_EXHAUST, 2: self.FAN_DIRECTION_INTAKE}

        fan_direction = self.dependency.eeprom.airflow_fan_type()
        return direction.get(fan_direction, self.FAN_DIRECTION_NOT_APPLICABLE)

    def get_speed(self):
        """
        Retrieves the speed of fan

        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = self._get_i2c_register(self.get_fan_speed_reg)
        if (fan_speed != 'ERR') and self.get_presence():
            speed = self._get_speed_to_percentage(int(fan_speed, 10))
        else:
            speed = 0

        return speed

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed
            which is considered tolerable
        """
        if self.get_presence():
            # The tolerance value is fixed as 20% for all the DellEmc platform
            tolerance = 20
        else:
            tolerance = 0

        return tolerance

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set
            fan to, in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        fan_set = (speed * self.max_fan_speed) // 100
        return self._set_speed_rpm(fan_set)

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # No LED available for FanTray and PSU Fan
        # Return True to avoid thermalctld alarm.
        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        # No LED available for FanTray and PSU Fan
        return None

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        target_speed_rpm = self._get_target_speed_rpm()

        if not self.is_psu_fan and self._target_speed:
            # Handle max6620 driver approximation
            max6620_conv_factor = (60 * 8192 * 4) / 2
            expected_speed_rpm = max6620_conv_factor // (max6620_conv_factor // self._target_speed)

            if expected_speed_rpm == target_speed_rpm:
                if self._target_speed >= MAX_S6000_FAN_TARGET_SPEED:
                    return 100
                else:
                    return self._get_speed_to_percentage(self._target_speed)

        return self._get_speed_to_percentage(target_speed_rpm)

    def set_speed_for_thermal_level(self, thermal_level):

        req_speed_rpm = self.thermal_level_to_speed[thermal_level]
        req_speed = self._get_speed_to_percentage(req_speed_rpm)
        target_speed = self.get_target_speed()

        if req_speed != target_speed:
            self._set_speed_rpm(req_speed_rpm)

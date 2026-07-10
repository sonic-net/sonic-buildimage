#!/usr/bin/env python

########################################################################
# DellEMC E3224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################
try:
    import os
    from sonic_platform_base.fan_base import FanBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(FanBase):
    """DellEMC Platform-specific Fan class"""

    def __init__(self, fantray_index=0, fan_index=0, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan

        if not self.is_psu_fan:
            # API index is starting from 0, DellEMC platform index is
            # starting from 1
            self.presence_reg = "fan{}_prs".format(fantray_index)
            self.dir_reg = "fan{}_dir".format(fantray_index)
            self.rpm_file = "/sys/bus/i2c/devices/7-002c/fan{}_input".format(fantray_index+1)
            self.status_file = "/sys/bus/i2c/devices/7-002c/fan{}_fault".format(fantray_index+1)
            self.eeprom = "/sys/bus/i2c/devices/{}-0050/eeprom".format(15 + fantray_index)
            self.fantray_index = fantray_index
            self.fan_index = fan_index
        else:
            self.psu_index = fan_index - 1
            self.dependancy = dependency
            self.presence_reg = "psu{}_prs".format(self.psu_index)
            self.status_reg = "psu{}_status".format(self.psu_index)
            self.dir_reg = ""
            self.dps_hwmon = "/sys/bus/i2c/devices/{}-005e/hwmon/".format(self.psu_index + 10)
            self.eeprom = "/sys/bus/i2c/devices/{}-0056/eeprom".format(self.psu_index + 10)
            self.fan_index = fan_index
        self.max_speed = 0

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_dir = "/sys/devices/platform/dell-e3224f-cpld.0/"
        cpld_reg_file = cpld_dir + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r')as fd:
                buf = fd.read()
        except (IOError, AttributeError):
            return 'ERR'
        return buf.strip('\r\n').lstrip(' ')

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            String: The name of the device
        """
        if self.is_psu_fan:
            return "PSU{} Fan".format(self.psu_index+1)
        else:
            return "FanTray{}-Fan{}".format(self.fantray_index+1, self.fan_index+1)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            String: Part number of FAN
        """
        try:
            val = open(self.eeprom, "rb").read()[13:19]
        except Exception:
            val = None
        return val.decode()

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        try:
            val = open(self.eeprom, "rb").read()[21:41]
        except Exception:
            val = None
        return val.decode()

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """

        presence = self._get_cpld_register(self.presence_reg)
        if presence == 'ERR':
            return False
        if int(presence,0) == 1:
            return True
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.is_psu_fan:
            status = open(self.status_file, "rb").read()
            if int(status, 0) == 1:
                return False
            else:
                return True
        else:
            status = self._get_cpld_register(self.status_reg)
            if status == 'ERR':
                return False
            if int(status, 0) == 1:
                return True
            else:
                return False

    def get_direction(self):
        """
        Retrieves the fan airfow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            In DellEMC platforms,
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        if not self.is_psu_fan:
            val = self._get_cpld_register(self.dir_reg)
            direction = 'exhaust' if val == 'F2B' else 'intake'
            if direction == 'ERR':
                return None
        else:
            try:
                val = open(self.eeprom, "rb").read()[0xe1:0xe8]
            except Exception:
                return None
            direction = 'exhaust' if val == 'FORWARD' else 'intake'
        return direction

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        if self.max_speed == 0:
            self.max_speed = 23500
        fan_speed = 0
        try:
            if not self.is_psu_fan:
                rpm_file = self.rpm_file
            else:
                dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
                rpm_file = dps_dir + '/' + 'fan1_input'
            fan_speed = int(open(rpm_file, "rb").read())
        except Exception:
            return None
        speed = (100 * fan_speed)//self.max_speed
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = 0
        try:
            if not self.is_psu_fan:
                rpm_file = self.rpm_file
            else:
                dps_dir = self.dps_hwmon + '/' + os.listdir(self.dps_hwmon)[0]
                rpm_file = dps_dir + '/' + 'fan1_input'
            fan_speed = int(open(rpm_file, "rb").read())
        except Exception:
            return None
        return fan_speed

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.fan_index

    def is_replaceable(self):
        """
        Indicate whether Fan is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
            considered tolerable
        """
        if self.get_presence():
            # The tolerance value is fixed as 20% for all the DellEMC platforms
            tolerance = 20
        else:
            tolerance = 0

        return tolerance

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
           color: A string representing the color with which to set the
                 fan status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # E3224F has led only on FanTray and not available for seperate fans
        return True

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """

        return None

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        # Return current speed to avoid false thermalctld alarm.
        return self.get_speed()


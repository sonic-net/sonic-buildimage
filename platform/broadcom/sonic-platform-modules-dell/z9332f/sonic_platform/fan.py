#!/usr/bin/env python

########################################################################
# DellEMC Z9332F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################
try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.ipmihelper import IpmiSensor, get_ipmitool_raw_output
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Fan(FanBase):
    """DellEMC Platform-specific Fan class"""
    # { FAN-ID: { Sensor-Name: Sensor-ID } }
    FAN_SENSOR_MAPPING = { 1: {"Prsnt": 0x6, "State": 0x6, "Speed": 0xd},
                           2: {"Prsnt": 0x6, "State": 0x6, "Speed": 0x45},
                           3: {"Prsnt": 0x7, "State": 0x7, "Speed": 0xe},
                           4: {"Prsnt": 0x7, "State": 0x7, "Speed": 0x46},
                           5: {"Prsnt": 0x8, "State": 0x8, "Speed": 0xf},
                           6: {"Prsnt": 0x8, "State": 0x8, "Speed": 0x47},
                           7: {"Prsnt": 0x9, "State": 0x9, "Speed": 0x10},
                           8: {"Prsnt": 0x9, "State": 0x9, "Speed": 0x48},
                           9: {"Prsnt": 0xa, "State": 0xa, "Speed": 0x11},
                           10: {"Prsnt": 0xa, "State": 0xa, "Speed": 0x49},
                           11: {"Prsnt": 0xb, "State": 0xb, "Speed": 0x12},
                           12: {"Prsnt": 0xb, "State": 0xb, "Speed": 0x4a},
                           13: {"Prsnt": 0xc, "State": 0xc, "Speed": 0x13},
                           14: {"Prsnt": 0xc, "State": 0xc, "Speed": 0x4b} }
    PSU_FAN_SENSOR_MAPPING = { 1: {"State": 0x2f, "Speed": 0x33},
                               2: {"State": 0x39, "Speed": 0x3d} }

    def __init__(self, fantray_index=1, fan_index=1, psu_fan=False, dependency=None):
        FanBase.__init__(self)
        self.is_psu_fan = psu_fan
        if not self.is_psu_fan:
            # API index is starting from 0, DellEMC platform index is
            # starting from 1
            self.fantrayindex = fantray_index + 1
            self.fanindex = fan_index + 1
            self.index = (self.fantrayindex - 1) * 2 + self.fanindex
            self.prsnt_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["Prsnt"],
                                           is_discrete=True)
            self.state_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["State"],
                                           is_discrete=True)
            self.speed_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["Speed"])
            self.fan_dir_raw_cmd = "0x3a 0x0a {}".format(fantray_index)
            if self.fanindex == 1:
                self.max_speed = 24700
            else:
                self.max_speed = 29700
        else:
            self.dependency = dependency
            self.fanindex = fan_index
            self.state_sensor = IpmiSensor(self.PSU_FAN_SENSOR_MAPPING[self.fanindex]["State"],
                                           is_discrete=True)
            self.speed_sensor = IpmiSensor(self.PSU_FAN_SENSOR_MAPPING[self.fanindex]["Speed"])
            self.fan_dir_raw_cmd = "0x3a 0x0a {}".format(7+(fan_index-1))
            self.max_speed = 26500

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            String: The name of the device
        """
        if self.is_psu_fan:
            return "PSU{} Fan".format(self.fanindex)
        else:
            return "FanTray{}-Fan{}".format(self.fantrayindex, self.fanindex)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            String: Part number of FAN
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        return 'NA'

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        presence = False
        if self.is_psu_fan:
            return self.dependency.get_presence()
        else:
            is_valid, state = self.prsnt_sensor.get_reading()
            if is_valid:
                if (state & 0b1):
                    presence = True
            return presence

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        status = False
        is_valid, state = self.state_sensor.get_reading()
        if is_valid:
            if state & 0b1:
                status = True
        return status

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
        direction = [self.FAN_DIRECTION_EXHAUST, self.FAN_DIRECTION_INTAKE]
        fan_status = self.get_presence()
        if not fan_status:
            return None
        dir_res = get_ipmitool_raw_output(self.fan_dir_raw_cmd)
        if dir_res is not None and len(dir_res) == 1 :
            return direction[dir_res[0]]
        else:
            return None

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        if self.max_speed == 0:
            self.max_speed = 23500
        is_valid, fan_speed = self.speed_sensor.get_reading()
        if not is_valid or self.max_speed == 0:
            return None
        else:
            speed = (100 * fan_speed)//self.max_speed
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        is_valid, fan_speed = self.speed_sensor.get_reading()
        return fan_speed if is_valid else None

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.fanindex

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
        # Fan tray status LED controlled by HW
        # Return True to avoid thermalctld alarm
        return True

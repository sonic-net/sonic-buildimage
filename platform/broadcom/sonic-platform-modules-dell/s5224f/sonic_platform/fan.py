#!/usr/bin/env python

########################################################################
# DellEMC S5224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################

try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.ipmihelper import IpmiSensor, IpmiFru
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN1_MAX_SPEED_OFFSET = 71
FAN2_MAX_SPEED_OFFSET = 73
PSU_FAN_MAX_SPEED_OFFSET = 50
FAN_DIRECTION_OFFSET = 69
PSU_FAN_DIRECTION_OFFSET = 47


class Fan(FanBase):
    """DellEMC Platform-specific Fan class"""
    # { FAN-ID: { Sensor-Name: Sensor-ID } }
    # System rev X01, BMC firmware rev 1.02
    FAN_SENSOR_MAPPING = { 1: {"Prsnt": 0x53, "State": 0x57, "Speed": 0x24},
                           2: {"Prsnt": 0x53, "State": 0x5b, "Speed": 0x20},
                           3: {"Prsnt": 0x54, "State": 0x58, "Speed": 0x25},
                           4: {"Prsnt": 0x54, "State": 0x5c, "Speed": 0x21},
                           5: {"Prsnt": 0x55, "State": 0x59, "Speed": 0x26},
                           6: {"Prsnt": 0x55, "State": 0x5d, "Speed": 0x22},
                           7: {"Prsnt": 0x56, "State": 0x5a, "Speed": 0x27},
                           8: {"Prsnt": 0x56, "State": 0x5e, "Speed": 0x23}
    }
    PSU_FAN_SENSOR_MAPPING = { 1: {"State": 0x31, "Speed": 0x28},
                               2: {"State": 0x32, "Speed": 0x29} }

    # { FANTRAY-ID: FRU-ID }
    FAN_FRU_MAPPING = { 1: 3, 2: 4, 3: 5, 4: 6 }
    PSU_FRU_MAPPING = { 1: 1, 2: 2 }

    def __init__(self, fantray_index=1, fan_index=1, psu_fan=False,
            dependency=None):
        FanBase.__init__(self)
        self.is_psu_fan = psu_fan
        if not self.is_psu_fan:
            # API index is starting from 0, DellEMC platform index is
            # starting from 1
            self.fantrayindex = fantray_index + 1
            self.fanindex = fan_index + 1
            if (self.fanindex == 1):
                self.max_speed_offset = FAN1_MAX_SPEED_OFFSET
            else:
                self.max_speed_offset = FAN2_MAX_SPEED_OFFSET
            self.fan_direction_offset = FAN_DIRECTION_OFFSET
            self.index = (self.fantrayindex - 1) * 2 + self.fanindex
            self.prsnt_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["Prsnt"],
                                           is_discrete=True)
            self.state_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["State"],
                                           is_discrete=True)
            self.speed_sensor = IpmiSensor(self.FAN_SENSOR_MAPPING[self.index]["Speed"])
            self.fru = IpmiFru(self.FAN_FRU_MAPPING[self.fantrayindex])
        else:
            self.dependency = dependency
            self.fanindex = fan_index
            self.state_sensor = IpmiSensor(self.PSU_FAN_SENSOR_MAPPING[self.fanindex]["State"],
                                           is_discrete=True)
            self.speed_sensor = IpmiSensor(self.PSU_FAN_SENSOR_MAPPING[self.fanindex]["Speed"])
            self.fru = IpmiFru(self.PSU_FRU_MAPPING[self.fanindex])
            self.max_speed_offset = PSU_FAN_MAX_SPEED_OFFSET
            self.fan_direction_offset = PSU_FAN_DIRECTION_OFFSET
        self.max_speed = 0

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
        return self.fru.get_board_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        return self.fru.get_board_serial()

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
            if self.is_psu_fan:
                if not state > 1:
                    status = True
            else:
                if state == 0x00:
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
        is_valid, fan_direction = self.fru.get_fru_data(self.fan_direction_offset)
        if is_valid and fan_direction[0] < len(direction):
            return direction[fan_direction[0]]
        else:
            return None

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        if self.max_speed == 0:
            is_valid, max_speed = self.fru.get_fru_data(self.max_speed_offset,2)
            if not is_valid:
                return 0
            self.max_speed = max_speed[1]
            self.max_speed = max_speed[1] << 8 | max_speed[0]
        is_valid, fan_speed = self.speed_sensor.get_reading()
        if not is_valid or self.max_speed == 0:
            speed = 0
        else:
            speed = (100 * fan_speed)//self.max_speed
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = 0
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

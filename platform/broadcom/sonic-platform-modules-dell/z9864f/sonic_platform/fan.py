#!/usr/bin/env python
"""
########################################################################
# DELL Z9864F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform.
#
########################################################################
"""

try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.ipmihelper import IpmiSensor, IpmiFru
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

FAN_DIRECTION_OFFSET = 69
PSU_FAN_DIRECTION_OFFSET = 47
PSU_FAN_FAULT = 0x03
FAN_FAULT = 0x07

class Fan(FanBase):
    """DELL Platform-specific Fan class"""
    # { FAN-ID: { Sensor-Name: Sensor-ID } }
    FAN_SENSOR_MAPPING = {1: {"Prsnt": 0x51, "State": 0x20, "Speed": 0x1A},
                          2: {"Prsnt": 0x51, "State": 0x21, "Speed": 0x1B},
                          3: {"Prsnt": 0x52, "State": 0x22, "Speed": 0x1C},
                          4: {"Prsnt": 0x52, "State": 0x23, "Speed": 0x1D},
                          5: {"Prsnt": 0x53, "State": 0x24, "Speed": 0x17},
                          6: {"Prsnt": 0x53, "State": 0x25, "Speed": 0x14},
                          7: {"Prsnt": 0x54, "State": 0x26, "Speed": 0x19},
                          8: {"Prsnt": 0x54, "State": 0x27, "Speed": 0x16}}
    PSU_FAN_SENSOR_MAPPING = {1: {"State": 0x41, "Speed": 0x45},
                              2: {"State": 0x31, "Speed": 0x35}}

    # { FANTRAY-ID: FRU-ID }
    FAN_FRU_MAPPING = {1: 3, 2: 4, 3: 5, 4: 6}
    PSU_FRU_MAPPING = {1: 1, 2: 2}

    def __init__(self, fantray_index=1, fan_index=1, psu_fan=False,
                 dependency=None):
        self.is_psu_fan = psu_fan
        self.max_speed = 14600
        if not self.is_psu_fan:
            # API index is starting from 0, DELL platform index is
            # starting from 1
            self.fantrayindex = fantray_index + 1
            self.fanindex = fan_index + 1
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
            self.fan_direction_offset = PSU_FAN_DIRECTION_OFFSET

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
        if self.is_psu_fan:
            return None
        else:
            return self.fru.get_board_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            String: Serial number of FAN
        """
        if self.is_psu_fan:
            return None
        else:
            return self.fru.get_board_serial()

    def _get_tray_presence(self):
        if self.is_psu_fan:
            return self.dependency.get_presence()
        else:
            is_valid, state = self.prsnt_sensor.get_reading()
            return ((state & 0b1) == 1) if is_valid else False

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        return self._get_tray_presence()

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        status = False
        fan_speed_rpm = self.get_speed_rpm()

        if self.is_psu_fan and not self.dependency.get_status() or \
            fan_speed_rpm == 0:
            return status

        is_valid, state = self.state_sensor.get_reading()
        if is_valid:
            if self.is_psu_fan and state & PSU_FAN_FAULT == 0:
                status = True
            elif state & FAN_FAULT == 0:
                status = True
        return status

    def get_direction(self):
        """
        Retrieves the fan airfow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            In DELL platforms,
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        direction = [self.FAN_DIRECTION_EXHAUST, self.FAN_DIRECTION_INTAKE]
        fan_presence = self.get_presence()
        if not fan_presence:
            return None
        is_valid, fan_direction = self.fru.get_fru_data(self.fan_direction_offset)
        if is_valid:
            try:
                return direction[fan_direction[0]]
            except IndexError:
                pass
        return None

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        is_valid, fan_speed = self.speed_sensor.get_reading()
        if not is_valid:
            return None
        else:
            speed = (100 * fan_speed)//self.max_speed
        return int(speed)

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: fan speed in RPM
        """
        is_valid, fan_speed = self.speed_sensor.get_reading()
        return fan_speed if is_valid else None

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
            fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        # Fan status LED controlled by BMC
        # Return True to avoid thermalctld alarm
        return True

    def get_status_led(self):
        """
        Gets the state of the fan LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        # Fan tray status LED controlled by BMC
        return self.STATUS_LED_COLOR_OFF

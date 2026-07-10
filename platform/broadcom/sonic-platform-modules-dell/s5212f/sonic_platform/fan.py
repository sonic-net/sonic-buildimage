#!/usr/bin/env python

########################################################################
# DellEMC S5212F
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

switch_sku = {
  "0K6MG9":('AC', 'exhaust'),
  "0GKK8W":('AC', 'intake'),
  "0VK93C":('AC', 'exhaust'),
  "05JHDM":('AC', 'intake'),
  "0D72R7":('AC', 'exhaust'),
  "02PC9F":('AC', 'exhaust'),
  "0JM5DX":('AC', 'intake'),
  "0TPDP8":('AC', 'exhaust'),
  "0WND1V":('AC', 'exhaust'),
  "05672M":('DC', 'intake'),
  "0CJV4K":('DC', 'intake'),
  "0X41RN":('AC', 'exhaust'),
  "0Y3N82":('AC', 'intake'),
  "0W4CMG":('DC', 'exhaust'),
  "04T94Y":('DC', 'intake')
}


class Fan(FanBase):
    """DellEMC Platform-specific Fan class"""
    # { FAN-ID: { Sensor-Name: Sensor-ID } }
    FAN_SENSOR_MAPPING = { 1: {"Prsnt": 0x57, "State": 0x57, "Speed": 0x24},
                           2: {"Prsnt": 0x5b, "State": 0x5b, "Speed": 0x20},
                           3: {"Prsnt": 0x58, "State": 0x58, "Speed": 0x25},
                           4: {"Prsnt": 0x5c, "State": 0x5c, "Speed": 0x21},
                           5: {"Prsnt": 0x57, "State": 0x59, "Speed": 0x26},
                           6: {"Prsnt": 0x59, "State": 0x5d, "Speed": 0x22},
                           7: {"Prsnt": 0x5a, "State": 0x5a, "Speed": 0x27},
                           8: {"Prsnt": 0x5e, "State": 0x5e, "Speed": 0x23} }
    PSU_FAN_SENSOR_MAPPING = { 1: {"State": 0x31, "Speed": 0x2e},
                               2: {"State": 0x32, "Speed": 0x2f} }

    # { FANTRAY-ID: FRU-ID }
    FAN_FRU_MAPPING = { 1: 0, 2: 0, 3: 0, 4: 0 }
    PSU_FRU_MAPPING = { 1: 0, 2: 0 }

    def __init__(self, fantray_index=1, fan_index=1, psu_fan=False,
            dependency=None):
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
        self.max_speed = 16000

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
        if self.is_psu_fan:
            return self.dependency.get_presence()
        else:
            # In S5212F, Fans are fixed
            return True

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
        board_info = self.fru.get_board_part_number()
        if board_info is not None :
            board_part_no = board_info[0:6]
            if board_part_no in switch_sku:
                return switch_sku[board_part_no][1]
        return None

    def get_speed(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        speed = None
        if not self.is_psu_fan:
            if self.max_speed == 0:
                self.max_speed = self.fru.get_fru_data(self.max_speed_offset,2)[1]
                self.max_speed = self.max_speed[1] << 8 | self.max_speed[0]
            is_valid, fan_speed = self.speed_sensor.get_reading()
            if is_valid and self.max_speed > 0:
                speed = (100 * fan_speed)//self.max_speed
        return speed

    def get_speed_rpm(self):
        """
        Retrieves the speed of the fan
        Returns:
            int: percentage of the max fan speed
        """
        fan_speed = None
        if not self.is_psu_fan :
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

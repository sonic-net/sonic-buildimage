#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

import json
import math
import os.path

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


IPMI_OEM_NETFN = "0x3A"
IPMI_SENSOR_NETFN = "0x04"
IPMI_FAN_SPEED_CMD = "0x2D {}"
IPMI_AIR_FLOW_CMD = "0x0A {}"
IPMI_FAN_PRESENT_CMD = "0x06 0x03 {}"
IPMI_FAN_SPEED_CPLD_CMD = "0x03 0x01 0x01 {}"
IPMI_SET_FAN_LED_CMD = "0x07 {} {}"
IPMI_GET_FAN_LED_CMD = "0x08 {}"

FAN_NAME_TEMPLATE = "{}Fan{}{}"

FAN_FRONT_MAX_SPEED = 24700
FAN_REAR_MAX_SPEED = 29700
FAN_PSU_MAX_SPEED = 26500
FAN_SPEED_TOLERANCE = 25

CPLD_REG_FANTRAY_BASE = 0x22
FAN_FRONT_SID_BASE = 0x0D # IPMI Sensor ID
FAN_REAR_SID_BASE = 0x45 # IPMI Sensor ID
FAN_FRU_ID_BASE = 6
PSU_FRU_ID_BASE = 3
PSU_FAN_SID_BASE = {\
    0: 0x33,\
    1: 0x3D\
}
FAN_LED_OFF_CMD = "0x00"
FAN_LED_GREEN_CMD = "0x01"
FAN_LED_RED_CMD = "0x02"
FAN1_LED_CMD = "0x04"
NUM_FAN_TRAY = 7


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        FanBase.__init__(self)
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        self.name = None
        if self.is_psu_fan:
            self.psu_index = psu_index
            self.index = NUM_FAN_TRAY * 2 + self.psu_index
            self.max_speed = FAN_PSU_MAX_SPEED
            self.sid_offset = PSU_FAN_SID_BASE[self.psu_index]
        else:
            self.index = self.fan_tray_index * 2 + self.fan_index
            if fan_index % 2 == 0:
                self.is_front = True
                self.sid_offset = FAN_FRONT_SID_BASE + self.fan_tray_index
                self.max_speed = FAN_FRONT_MAX_SPEED
            else:
                self.is_front = False
                self.sid_offset = FAN_REAR_SID_BASE + self.fan_tray_index
                self.max_speed = FAN_REAR_MAX_SPEED

        self._api_helper = APIHelper()

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_NOT_APPLICABLE

        if not self.is_psu_fan:
            offset = hex(self.fan_tray_index)
        else:
            offset = hex(NUM_FAN_TRAY + self.psu_index)

        status, output = self._api_helper.ipmi_raw(IPMI_OEM_NETFN,\
                                                   IPMI_AIR_FLOW_CMD.format(offset))
        if status:
            if output == "00":
                direction = self.FAN_DIRECTION_EXHAUST
            elif output == "01":
                direction = self.FAN_DIRECTION_INTAKE
            elif self.is_psu_fan:
                # Trying to deduce from PSU model
                status, output = self._api_helper.ipmi_fru_id(PSU_FRU_ID_BASE + self.fan_tray_index,
                                                              "Board Part Number")
                if status and output:
                    model = output.split(':')[-1].strip()
                    if model == "TDPS1500AB7C":
                        direction = self.FAN_DIRECTION_INTAKE
                    elif model == "TDPS1500AB6E":
                        direction = self.FAN_DIRECTION_EXHAUST

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = 0

        if not self.is_psu_fan:
            multiplier = 150.0
        else:
            multiplier = 200.0

        status, output = self._api_helper.ipmi_raw(IPMI_SENSOR_NETFN,\
                                                   IPMI_FAN_SPEED_CMD.format(self.sid_offset))
        if status:
            raw_speed = output.split()[0]
            rpm_speed = int(raw_speed, 16) * multiplier
            speed = int((rpm_speed/self.max_speed) * 100)

        return speed

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
        if self.is_psu_fan:
            # Ignored for tolerance check
            return self.get_speed()

        offset = hex(CPLD_REG_FANTRAY_BASE + (16 * self.fan_tray_index))
        status, output = self._api_helper.ipmi_raw(IPMI_OEM_NETFN,\
                                                   IPMI_FAN_SPEED_CPLD_CMD.format(offset))
        if status:
            fan_pwm = int(output, 16)
            target_speed = int(fan_pwm / 255 * 100)

        return target_speed

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
        """

        # FAN speed is controlled by BCM always
        return False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if status LED state is set successfully, False if not

        Note:
           LED setting mode must set as Manual
           manual: ipmitool raw 0x3A 0x09 0x02 0x00
           auto: ipmitool raw 0x3A 0x09 0x02 0x01
        """

        if self.is_psu_fan:
            # Not supported for PSU
            return False

        led_cmd = {
            self.STATUS_LED_COLOR_GREEN: FAN_LED_GREEN_CMD,
            self.STATUS_LED_COLOR_RED: FAN_LED_RED_CMD,
            self.STATUS_LED_COLOR_OFF: FAN_LED_OFF_CMD
        }.get(color)

        fan_selector = hex(int(FAN1_LED_CMD, 16) + self.fan_tray_index)
        status, set_led = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_SET_FAN_LED_CMD.format(fan_selector, led_cmd))
        set_status_led = False if not status else True

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_RED = "red"
            STATUS_LED_COLOR_OFF = "off"
        """
        if not self.get_presence() or self.is_psu_fan:
            # Not supported for PSU
            return "N/A"

        fan_selector = hex(int(FAN1_LED_CMD, 16) + self.fan_tray_index)
        status, hx_color = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_FAN_LED_CMD.format(fan_selector))

        status_led = {
            "00": self.STATUS_LED_COLOR_OFF,
            "01": self.STATUS_LED_COLOR_GREEN,
            "02": self.STATUS_LED_COLOR_RED,
        }.get(hx_color, self.STATUS_LED_COLOR_OFF)

        return status_led

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        if not self.name:
            if not self.is_psu_fan:
                psu_name = ""
                fan_id = " {}".format(self.fan_tray_index + 1)
                fan_type = " Front" if self.is_front else " Rear"
            else:
                psu_name = "PSU {} ".format(self.psu_index + 1)
                fan_id = " {}".format(self.fan_tray_index + 1)
                fan_type = ""

            self.name = FAN_NAME_TEMPLATE.format(psu_name, fan_id, fan_type)

        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """

        presence = False

        status, output = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_FAN_PRESENT_CMD.format(hex(self.index)))
        if status and output == "00":
            presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = "N/A"

        if not self.is_psu_fan:
            status, output = self._api_helper.ipmi_fru_id(FAN_FRU_ID_BASE + self.fan_tray_index,
                                                          "Board Part Number")
            if status and output:
                model = output.split(':')[-1].strip()

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = "N/A"

        if not self.is_psu_fan:
            status, output = self._api_helper.ipmi_fru_id(FAN_FRU_ID_BASE + self.fan_tray_index,
                                                          "Board Serial")
            if status and output:
                serial = output.split(':')[-1].strip()

        return serial

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and self.get_speed() > 0

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

        return self.fan_index + 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False

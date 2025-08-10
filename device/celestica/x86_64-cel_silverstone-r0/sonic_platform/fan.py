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


FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R", "FAN-3F", "FAN-3R",
                 "FAN-4F", "FAN-4R", "FAN-5F", "FAN-5R", "FAN-6F", "FAN-6R", "FAN-7F", "FAN-7R"]

IPMI_OEM_NETFN = "0x3A"
IPMI_SENSOR_NETFN = "0x04"
IPMI_FAN_SPEED_CMD = "0x2D {}"
IPMI_AIR_FLOW_CMD = "0x0A {}"
IPMI_FAN_PRESENT_CMD = "0x06 0x03 {}"
IPMI_SET_FAN_LED_CMD = "0x07 {} {}"
IPMI_GET_FAN_LED_CMD = "0x08 {}"
IPMI_SET_PWM = "0x03 0x01 0x02 {} {}"

IPMI_FRU_MODEL_KEY = "Board Part Number"
IPMI_FRU_SERIAL_KEY = "Board Serial"

MAX_OUTLET = 24700
MAX_INLET = 29700
SPEED_TOLERANCE = 10

FAN1_FRONT_SS_ID = "0x0D"
FAN1_REAR_SS_ID = "0x45"
FAN_LED_OFF_CMD = "0x00"
FAN_LED_GREEN_CMD = "0x01"
FAN_LED_RED_CMD = "0x02"
FAN1_LED_CMD = "0x04"
FAN_PWM_REGISTER_START = 0x22
FAN_PWM_REGISTER_STEP = 0x10
FAN1_FRU_ID = 6

NUM_OF_FAN_TRAY = 7
PSU_FAN1_FRONT_SS_ID = "0x33"


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        FanBase.__init__(self)
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
        self._api_helper = APIHelper()
        self.index = self.fan_tray_index * 2 + self.fan_index

    def get_direction(self):
        """
        Retrieves the direction of fan
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        direction = self.FAN_DIRECTION_EXHAUST
        fan_direction_key = hex(self.fan_tray_index) if not self.is_psu_fan else hex(
            self.psu_index + NUM_OF_FAN_TRAY)
        status, raw_flow = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_AIR_FLOW_CMD.format(fan_direction_key))
        if status and raw_flow == "01":
            direction = self.FAN_DIRECTION_INTAKE

        return direction

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)

        Note:
            M = 150
            Max F2B = 24700 RPM 
            Max B2F = 29700 RPM
        """

        max_rpm = MAX_OUTLET if self.fan_index % 2 == 0 else MAX_INLET
        fan1_ss_start = FAN1_FRONT_SS_ID if self.fan_index % 2 == 0 else FAN1_REAR_SS_ID

        ss_id = hex(int(fan1_ss_start, 16) + self.fan_tray_index) if not self.is_psu_fan else hex(
            int(PSU_FAN1_FRONT_SS_ID, 16) + self.fan_tray_index)
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_FAN_SPEED_CMD.format(ss_id))

        ss_read = raw_ss_read.split()[0]
        rpm_speed = int(ss_read, 16)*150
        speed = int(float(rpm_speed)/max_rpm * 100)

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
        return "N/A"

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
        Notes:
            pwm setting mode must set as Manual
            manual: ipmitool raw 0x3a 0x06 0x01 0x0
            auto: ipmitool raw 0x3a 0x06 0x01 0x1
        """
        # ipmitool raw 0x3a 0x03 0x01 0x02 {register} {pwm_speed}
        # register = 22 32 42 52 62 72 82

        if self.is_psu_fan:
            # TODO
            return False

        speed_hex = hex(int(float(speed)/100 * 255))
        fan_register_hex = hex(FAN_PWM_REGISTER_START +
                               (self.fan_tray_index*FAN_PWM_REGISTER_STEP))

        set_speed_cmd = IPMI_SET_PWM.format(fan_register_hex, speed_hex)
        status, set_speed_res = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, set_speed_cmd)

        set_speed = False if not status else True

        return set_speed

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
            # Not support
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
        if self.is_psu_fan:
            # Not support
            return self.STATUS_LED_COLOR_OFF

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
        fan_name = FAN_NAME_LIST[self.fan_tray_index*2 + self.fan_index] if not self.is_psu_fan else "PSU-{} FAN-{}".format(
            self.psu_index+1, self.fan_index+1)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        if self.is_psu_fan:
            return True

        presence = False
        status, raw_present = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_FAN_PRESENT_CMD.format(hex(self.index)))
        if status and raw_present == "00":
            presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            return "Unknown"

        model = "Unknown"
        ipmi_fru_idx = self.fan_tray_index + FAN1_FRU_ID
        status, raw_model = self._api_helper.ipmi_fru_id(
            ipmi_fru_idx, IPMI_FRU_MODEL_KEY)

        fru_pn_list = raw_model.split()
        if len(fru_pn_list) > 4:
            model = fru_pn_list[4]

        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        if self.is_psu_fan:
            return "Unknown"

        serial = "Unknown"
        ipmi_fru_idx = self.fan_tray_index + FAN1_FRU_ID
        status, raw_model = self._api_helper.ipmi_fru_id(
            ipmi_fru_idx, IPMI_FRU_SERIAL_KEY)

        fru_sr_list = raw_model.split()
        if len(fru_sr_list) > 3:
            serial = fru_sr_list[3]

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
        return (self.fan_tray_index*2 + self.fan_index + 1) \
            if not self.is_psu_fan else (self.fan_index+1)

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True if not self.is_psu_fan else False

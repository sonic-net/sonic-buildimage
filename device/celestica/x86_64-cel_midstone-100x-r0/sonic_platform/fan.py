#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the fan status which are available in the platform
#
#############################################################################

import math

try:
    from sonic_platform_base.fan_base import FanBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


FAN_NAME_LIST = ["FAN-1F", "FAN-1R", "FAN-2F", "FAN-2R", "FAN-3F", "FAN-3R",
                 "FAN-4F", "FAN-4R"]

IPMI_OEM_NETFN = "0x3A"
IPMI_SENSOR_NETFN = "0x3A 0x63"
IPMI_FAN_SPEED_CMD = "0x04 {}"  # fan speed
IPMI_GET_PSU1_FAN_SPEED_CMD = "0x3e 0x06 0xb0 2 0x90"
IPMI_GET_PSU2_FAN_SPEED_CMD = "0x3e 0x06 0xb2 2 0x90"
IPMI_GET_FAN_TARGET_SPEED_CMD = "0x3e 0 0x1a 1 {}"
IPMI_AIR_FLOW_CMD = "0x63 0x15 {}"   # air flow
IPMI_AIR_PSU_FLOW_CMD = "0x62 {}"   # psu air flow
IPMI_FAN_PRESENT_CMD = "0x63 0x03 {}"
IPMI_SET_FAN_LED_CMD = "0x39 0x02 {} {}"
IPMI_GET_FAN_LED_CMD = "0x39 0x01 {}"
IPMI_SET_PWM_CMD = "0x63 0x09 {} {}"
IPMI_SET_FAN_MANUAL_CMD = "0x63 0x01 0x00"
IPMI_SET_FAN_LED_MANUAL_CMD = "0x42 0x02 0x00"
IPMI_FRU_PRINT_ID = "ipmitool fru print {}"
IPMI_FRU_MODEL_KEY = "Board Part Number"
IPMI_FRU_SERIAL_KEY = "Board Serial"

MAX_OUTLET = 12600  # F2B xiqi
MAX_INLET = 10300   # B2F paiqi
SPEED_TOLERANCE = 10



FAN_LED_OFF_CMD = "0x00"
FAN_LED_GREEN_CMD = "0x01"
FAN_LED_AMBER_CMD = "0x02"
FAN1_FRU_ID = 6


class Fan(FanBase):
    """Platform-specific Fan class"""

    def __init__(self, fan_tray_index, fan_index=0, is_psu_fan=False, psu_index=0):
        self.fan_index = fan_index
        self.fan_tray_index = fan_tray_index
        self.is_psu_fan = is_psu_fan
        if self.is_psu_fan:
            self.psu_index = psu_index
        else:
            self.psu_index = 0
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

        if self.fan_tray_index < 4:
            fan_tray_index_ = self.fan_tray_index + 1
            ipmi_cmd = IPMI_AIR_FLOW_CMD
        else:
            fan_tray_index_ = self.psu_index - 1
            ipmi_cmd = IPMI_AIR_PSU_FLOW_CMD

        status, raw_flow = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, ipmi_cmd.format(hex(fan_tray_index_)))

        if status and raw_flow == b"01":
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
            Max F2B = 12600 RPM
            Max B2F = 10300 RPM
        """
        # ipmitool raw 0x3a 0x03 0x01 0x01 {register}
        # register = 22 32 42 52 62 72 82
        max_rpm = MAX_INLET if self.fan_index % 2 == 0 else MAX_OUTLET

        if self.fan_tray_index < 4:
            fan_tray_index_ = self.fan_tray_index + 1
            status, raw_ss_read = self._api_helper.ipmi_raw(IPMI_SENSOR_NETFN,IPMI_FAN_SPEED_CMD.format(fan_tray_index_))
            if self.fan_index % 2 == 0:  # self.fan_index = 0
                ss_read = raw_ss_read.split()[0]
            else:
                ss_read = raw_ss_read.split()[1]    # self.fan_index = 1
            rpm_speed = int(ss_read, 16)*60
            speed = int(float(rpm_speed) / max_rpm * 100)
        else:
            if self.psu_index == 5:
                status, raw_ss_read = self._api_helper.ipmi_raw(IPMI_OEM_NETFN,IPMI_GET_PSU1_FAN_SPEED_CMD)
            else:
                status, raw_ss_read = self._api_helper.ipmi_raw(IPMI_OEM_NETFN,IPMI_GET_PSU2_FAN_SPEED_CMD)

            raw_ss_read_reverse = raw_ss_read.split()[::-1]
            data_high = ('{0:0{1}b}'.format(int(raw_ss_read_reverse[0], 16), len(raw_ss_read_reverse[0]) * 4))
            n_bin = data_high[:5]
            n = int(n_bin, 2)
            data_low = ('{0:0{1}b}'.format(int(raw_ss_read_reverse[1], 16), len((raw_ss_read_reverse[0]) * 4)))
            y_bin = data_high[-3:] + data_low
            y = int(y_bin, 2)
            rpm_speed = float(y * 2 ** n)
            speed = int(rpm_speed / max_rpm * 100)
			
        return speed if speed <= 100 else rpm_speed

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
        if self.fan_tray_index < 4:
            ss_cmd = "0x4%s" % (hex(self.fan_tray_index * 4)[-1])
            status,raw_ss_read = self._api_helper.ipmi_raw(IPMI_OEM_NETFN,IPMI_GET_FAN_TARGET_SPEED_CMD.format(ss_cmd))
            pwm = int(raw_ss_read,16)
            target = math.ceil(float(pwm)*100/255)
        else:
            target = "N/A"
        return target

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
            manual: ipmitool raw 0x3a 0x63 0x01 0x00
            auto: ipmitool raw 0x3a 0x63 0x01 0x1
        """
        # ipmitool raw 0x3a 0x63 0x09 {fan_id} {pwm_speed}

        speed_hex = hex(int(float(speed)/100 * 255))
        if self.fan_tray_index < 4:
            fan_tray_index_ = self.fan_tray_index + 1
        else:
            fan_tray_index_ = self.psu_index
        
        status, set_manual_res = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_SET_FAN_MANUAL_CMD)
        status, set_speed_res = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_SET_PWM_CMD.format(hex(fan_tray_index_),speed_hex))

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
           manual: ipmitool raw 0x3A 0x42 0x02 0x00
           auto: ipmitool raw 0x3A 0x42 0x02 0x01
        """
        led_cmd = {
            self.STATUS_LED_COLOR_GREEN: FAN_LED_GREEN_CMD,
            self.STATUS_LED_COLOR_AMBER: FAN_LED_AMBER_CMD,
            self.STATUS_LED_COLOR_OFF: FAN_LED_OFF_CMD
        }.get(color)

        if self.fan_tray_index < 4:
            fan_tray_index_ = self.fan_tray_index + 4
        else:
            fan_tray_index_ = 2

        status, set_manual_res = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_SET_FAN_LED_MANUAL_CMD)
        status, set_led = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_SET_FAN_LED_CMD.format(fan_tray_index_, led_cmd))
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
        if self.fan_tray_index < 4:
            fan_tray_index_ = self.fan_tray_index + 4
        else:
            fan_tray_index_ = 2

        status, hx_color = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_GET_FAN_LED_CMD.format(fan_tray_index_))
        status_led = {
            "00": self.STATUS_LED_COLOR_OFF,
            "01": self.STATUS_LED_COLOR_GREEN,
            "02": self.STATUS_LED_COLOR_AMBER,
        }.get(hx_color, self.STATUS_LED_COLOR_OFF)
        return status_led

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        if not self.is_psu_fan:
            fan_name = FAN_NAME_LIST[self.fan_tray_index*2 + self.fan_index]
        else:
            fan_name = "PSU-{} FAN-1".format(self.psu_index-4)

        return fan_name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """

        presence = False
        status, raw_present = self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_FAN_PRESENT_CMD.format(hex(self.index // 2)))

        if status and raw_present == b"01":
            presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = "Unknown"
        ipmi_fru_idx = self.fan_tray_index + FAN1_FRU_ID
        status, raw_model = self._api_helper.ipmi_fru_id(ipmi_fru_idx, IPMI_FRU_MODEL_KEY)

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
        serial = "Unknown"
        ipmi_fru_idx = self.fan_tray_index + FAN1_FRU_ID
        status, raw_model = self._api_helper.ipmi_fru_id(ipmi_fru_idx, IPMI_FRU_SERIAL_KEY)

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


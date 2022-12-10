#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os
import re
import math
import sonic_platform

try:
    from sonic_platform_base.psu_base import PsuBase
    from .helper import APIHelper
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NAME_LIST = ["PSU-1", "PSU-2"]
PSU_NUM_FAN = [1, 1]

IPMI_SENSOR_NETFN = "0x04"
IPMI_OEM_NETFN = "0x3A"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_SET_PSU_LED_CMD = "0x07 0x02 {}"
IPMI_GET_PSU_LED_CMD = "0x08 0x02"
IPMI_FRU_MODEL_KEY = "Board Part Number"
IPMI_FRU_SERIAL_KEY = "Board Serial"

PSU_LED_OFF_CMD = "0x00"
PSU_LED_GREEN_CMD = "0x01"
PSU_LED_AMBER_CMD = "0x02"

PSU1_FRU_ID = 3

SS_READ_OFFSET = 0

PSU_VOUT_SS_ID = ["0x36", "0x40"]
PSU_COUT_SS_ID = ["0x37", "0x41"]
PSU_POUT_SS_ID = ["0x38", "0x42"]
PSU_STATUS_REG = ["0x39", "0x2f"]


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            self._fan_list.append(fan)
        self._api_helper = APIHelper()

    def find_value(self, in_string):
        result = re.search("^.+ ([0-9a-f]{2}) .+$", in_string)
        return result.group(1) if result else result

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        psu_vout_key = PSU_VOUT_SS_ID[self.index]
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_vout_key))
        ss_read = raw_ss_read.split()[SS_READ_OFFSET]
        # Formula: Rx1x10^-1
        psu_voltage = int(ss_read, 16) * math.pow(10, -1)

        return psu_voltage

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = 0.0
        psu_cout_key = PSU_COUT_SS_ID[self.index]
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_cout_key))
        ss_read = raw_ss_read.split()[SS_READ_OFFSET]
        # Formula: Rx5x10^-1
        psu_current = int(ss_read, 16) * 5 * math.pow(10, -1)

        return psu_current

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = 0.0
        psu_pout_key = PSU_POUT_SS_ID[self.index]
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pout_key))
        ss_read = raw_ss_read.split()[SS_READ_OFFSET]
        # Formula: Rx6x10^0
        psu_power = int(ss_read, 16) * 6
        return psu_power

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        Note
            Set manual
            ipmitool raw 0x3a 0x09 0x2 0x0
        """
        led_cmd = {
            self.STATUS_LED_COLOR_GREEN: PSU_LED_GREEN_CMD,
            self.STATUS_LED_COLOR_AMBER: PSU_LED_AMBER_CMD,
            self.STATUS_LED_COLOR_OFF: PSU_LED_OFF_CMD
        }.get(color)

        status, set_led = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_SET_PSU_LED_CMD.format(led_cmd))
        set_status_led = False if not status else True

        return set_status_led

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status, hx_color = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_PSU_LED_CMD)

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
        return PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        psu_presence = False
        psu_pstatus_key = PSU_STATUS_REG[self.index]
        status, raw_status_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pstatus_key))
        status_byte = self.find_value(raw_status_read)

        if status:
            presence_int = (int(status_byte, 16) >> 0) & 1
            psu_presence = True if presence_int else False

        return psu_presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = "Unknown"
        ipmi_fru_idx = self.index + PSU1_FRU_ID
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
        serial = "Unknown"
        ipmi_fru_idx = self.index + PSU1_FRU_ID
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
        psu_status = False
        psu_pstatus_key = PSU_STATUS_REG[self.index]
        status, raw_status_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pstatus_key))
        status_byte = self.find_value(raw_status_read)

        if status:
            failure_detected = (int(status_byte, 16) >> 1) & 1
            input_lost = (int(status_byte, 16) >> 3) & 1
            psu_status = False if (input_lost or failure_detected) else True

        return psu_status

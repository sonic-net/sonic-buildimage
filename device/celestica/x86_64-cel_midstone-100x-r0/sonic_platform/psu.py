#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import subprocess
import re
import math

try:
    from sonic_platform_base.psu_base import PsuBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NAME_LIST = ["PSU-1", "PSU-2"]

PSU_I2C_MAPPING = {
    0: {
        "num": 10,
        "addr": "5a"
    },
    1: {
        "num": 11,
        "addr": "5b"
    },
}
IPMI_OEM_NETFN = "0x39"
IPMI_SENSOR_NETFN = "0x04"
IPMI_SS_READ_CMD = "0x2D {}"
IPMI_SET_PSU_LED_CMD = "0x02 0x02 {}"
IPMI_GET_PSU_LED_CMD = "0x01 0x02"
IPMI_FRU_PRINT_ID = "ipmitool fru print {}"
IPMI_FRU_MODEL_KEY = "Product Part Number"
IPMI_FRU_SERIAL_KEY = "Product Serial "

PSU_LED_OFF_CMD = "0x00"
PSU_LED_GREEN_CMD = "0x01"
PSU_LED_AMBER_CMD = "0x02"

PSU1_VOUT_SS_ID = "0x2f"
PSU1_COUT_SS_ID = "0x30"
PSU1_POUT_SS_ID = "0x31"
PSU1_STATUS_REG = "0x29"
PSU1_TMP1_REG = "0x2d"
PSU1_TMP2_REG = "0x2e"

PSU2_VOUT_SS_ID = "0x39"
PSU2_COUT_SS_ID = "0x3a"
PSU2_POUT_SS_ID = "0x3b"
PSU2_STATUS_REG = "0x33"
PSU2_TMP1_REG = "0x37"
PSU2_TMP2_REG = "0x38"

PSU1_FRU_ID = 4

SS_READ_OFFSET = 0
PSU_MAX_POWER = 1300

class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self._api_helper = APIHelper()

        self.psu1_id = "0x29"
        self.psu2_id = "0x33"

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_vout_key = globals()['PSU{}_VOUT_SS_ID'.format(self.index + 1)]
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
        psu_cout_key = globals()['PSU{}_COUT_SS_ID'.format(self.index + 1)]
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
        psu_pout_key = globals()['PSU{}_POUT_SS_ID'.format(self.index + 1)]
        status, raw_ss_read = self._api_helper.ipmi_raw(
            IPMI_SENSOR_NETFN, IPMI_SS_READ_CMD.format(psu_pout_key))
        ss_read = raw_ss_read.split()[SS_READ_OFFSET]
        # Formula: Rx6x10^0
        psu_power = int(ss_read, 16) * 6
        return float(psu_power)

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
            ipmitool raw 0x3a 0x42 0x2 0x00
        """
        led_cmd = {
            self.STATUS_LED_COLOR_GREEN: PSU_LED_GREEN_CMD,
            self.STATUS_LED_COLOR_AMBER: PSU_LED_AMBER_CMD,
            self.STATUS_LED_COLOR_OFF: PSU_LED_OFF_CMD
        }.get(color)
        self._api_helper.ipmi_raw("0x3a 0x42 0x02 0x00")
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

    @staticmethod
    def run_command(command):
        proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
        (out, err) = proc.communicate()

        if proc.returncode != 0:
            sys.exit(proc.returncode)

        return out

    @staticmethod
    def find_value(in_string):
        result = re.search(b"^.+ ([0-9a-f]{2}) .+$", in_string)
        if result:
            return result.group(1)
        else:
            return result

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        if self.index is None:
            return False
        ipmi_raw = "ipmitool raw 0x4 0x2d"
        psu_id = self.psu1_id if self.index == 1 else self.psu2_id
        res_string = self.run_command(ipmi_raw + ' ' + psu_id)
        status_byte = self.find_value(res_string)
        
        if status_byte is None:
            return False
        
        presence = ( int(status_byte, 16) >> 0 ) & 1
        if presence:
            return True
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
            eg.ipmitool fru print 4
            Product Manufacturer  : DELTA
            Product Name          : DPS-1300AB-6 J
            Product Part Number   : DPS-1300AB-6 J
            Product Version       : S1F
            Product Serial        : JDMD2111000125
            Product Asset Tag     : S1F
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
        if self.index is None:
            return False
        ipmi_raw = "ipmitool raw 0x4 0x2d"
        psu_id = self.psu1_id if self.index == 1 else self.psu2_id
        res_string = self.run_command(ipmi_raw + ' ' + psu_id)
        status_byte = self.find_value(res_string)
        
        if status_byte is None:
            return False

        failure_detected = (int(status_byte, 16) >> 1) & 1
        input_lost = (int(status_byte, 16) >> 3) & 1
        if failure_detected or input_lost:
            return False
        else:
            return True

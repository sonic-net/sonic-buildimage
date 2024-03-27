#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_NUM_FAN = [1, 1]
PSU_INFO_MAPPING = {                                                                                              
    0: {
        "name": "PSU-L",
        "power_status_offset":3,
        "presence_offset":5,
        "fru_num":"3",
        "power":"PSUL_POut",
        "current":"PSUL_COut",
        "voltage":"PSUL_VOut",
        "led_offset":0x2
    },
    1: {
        "name": "PSU-R",
        "power_status_offset":2,
        "presence_offset":4,
        "fru_num":"4",
        "power":"PSUR_POut",
        "current":"PSUR_COut",
        "voltage":"PSUR_VOut",
        "led_offset":0x3
    }  
}

IPMI_RAW="ipmitool raw 0x3a 0x0c 0x0 0x2 0x60"
IPMI_FRU_MODEL="ipmitool fru list {} | grep 'Product Name'"
IPMI_FRU_SERIAL="ipmitool fru list {} | grep 'Product Serial'"
IPMI_SDR="ipmitool sdr | grep {}"
IPMI_GET_STATUS_LED="ipmitool raw 0x3a 0x0b {}"

class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self._api_helper = APIHelper()
        fan = Fan(0, 0, True, self.index, 'N/A')
        self._fan_list.append(fan)

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = 0.0
        if not self.get_status():
            return psu_voltage

        cmd=IPMI_SDR.format(PSU_INFO_MAPPING[self.index]["voltage"])
        status, voltage = self._api_helper.run_command(cmd)
        if status:
            return float(voltage.split()[2])
        else:
            return psu_voltage

    def get_voltage_high_threshold(self):
        return 12.6

    def get_voltage_low_threshold(self):
        return 11.4

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = 0.0

        if not self.get_status():
            return psu_current

        cmd=IPMI_SDR.format(PSU_INFO_MAPPING[self.index]["current"])
        status, current = self._api_helper.run_command(cmd)
        if status:
            return float(current.split()[2])
        else:
            return psu_current


    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = 0.0

        if not self.get_status():
            return psu_power

        cmd=IPMI_SDR.format(PSU_INFO_MAPPING[self.index]["power"])
        status, power = self._api_helper.run_command(cmd)
        if status:
            return float(power.split()[2])
        else:
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
        """
        # NOT ALLOW

        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return "blinking green"
        else:
            return self.STATUS_LED_COLOR_OFF


    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return PSU_INFO_MAPPING[self.index]["name"]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        status, psu_presence = self._api_helper.run_command(IPMI_RAW)
        if status:
            return True if (int(psu_presence,16) >> \
                PSU_INFO_MAPPING[self.index]['presence_offset'] & 1) == 0  else False
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        cmd=IPMI_FRU_MODEL.format(PSU_INFO_MAPPING[self.index]['fru_num'])
        status, model= self._api_helper.run_command(cmd)
        if status:
            return model.split(':')[1]
        else:
            return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        cmd=IPMI_FRU_SERIAL.format(PSU_INFO_MAPPING[self.index]['fru_num'])
        status, model= self._api_helper.run_command(cmd)
        if status:
            return model.split()[-1]
        else:
            return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False
        status, psu_status = self._api_helper.run_command(IPMI_RAW)
        if status:
            return True if (int(psu_status,16) >> \
                  PSU_INFO_MAPPING[self.index]['power_status_offset'] & 1) == 1  else False
        else:
            return False

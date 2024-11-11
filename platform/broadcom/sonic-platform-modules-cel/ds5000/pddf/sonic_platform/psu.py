#!/usr/bin/env python

import time

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the psu management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    PSU_IPMI_FRU_ID_MAP = {
        1: 215,
        2: 26,
        3: 128,
        4: 154
    }

    STATUS_LED_COLOR_BLUE = 'blue'

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()
        
    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        if 'name' in self.plugin_data['PSU']:
            return self.plugin_data['PSU']['name'][str(self.psu_index)]
        else:
            return "PSU{}".format(self.psu_index)

    def get_presence(self):

        idx = self.psu_index - 1
        status, result = self._api_helper.cpld_lpc_read(0xA15E)
        
        if (int(result, 16) & (1 << idx) == 0) and status == True:
            return True
        else:
            return False

    def get_powergood_status(self):

        idx = self.psu_index - 1        
        status, result = self._api_helper.cpld_lpc_read(0xA160)
        mask = (1 << idx + 4) | (1 << idx)
        
        if (int(result, 16) & mask == mask) and status == True:
            return True
        else:
            return False

    def get_type(self):
        return 'AC'

    def get_capacity(self):
        return 2000

    def get_voltage_low_threshold(self):
        return 10

    def get_voltage_high_threshold(self):
        return 14

    def set_status_led(self, color):
        return False

    def get_status_led(self):
        """
        Gets the state of the psu status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_RED = "red"
            STATUS_LED_COLOR_OFF = "off"
        """
        index = str(self.psu_index-1)
        psu_led_device = "PSU{}".format(self.psu_index) + "_LED"

        if psu_led_device not in self.pddf_obj.data.keys():
            if self.get_presence():
                if self.get_powergood_status():
                    return self.STATUS_LED_COLOR_BLUE
                else:
                    return self.STATUS_LED_COLOR_AMBER
            else:
                return self.STATUS_LED_COLOR_OFF

        device_name = self.pddf_obj.data[psu_led_device]['dev_info']['device_name']
        self.pddf_obj.create_attr('device_name', device_name,  self.pddf_obj.get_led_path())
        self.pddf_obj.create_attr('index', index, self.pddf_obj.get_led_path())
        self.pddf_obj.create_attr('dev_ops', 'get_status',  self.pddf_obj.get_led_path())
        color = self.pddf_obj.get_led_color()
        return (color)      

    def get_revision(self):
        """
        Retrieves the revision of the device
        Returns:
            string: revision of device
        """
        if not self.get_presence():
            return 'N/A'

        if self._api_helper.with_bmc():
            cmd = "ipmitool fru list {} | grep 'Product Version'".format(self.PSU_IPMI_FRU_ID_MAP.get(self.psu_index))
            status, output = self._api_helper.run_command(cmd)
            if status == True:
                rev = output.split()[-1]
                return rev
        else:
            output = self._api_helper.i2c_read(84 + self.psu_index - 1, 0x50, 0x40, 3)
            return bytes.fromhex(output.replace('0x', '').replace(" ", "")).decode("utf-8")
        return 'N/A'
    
    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_input")
        if not output:
            return 0.0

        temp1 = output['status']
        if temp1.isalpha():
            attr_value = None
        else:
            attr_value = float(temp1)

        if output['mode'] == 'bmc':
            return attr_value
        else:
            return (attr_value/float(1000))

    def get_temperature_high_threshold(self):
        return 60
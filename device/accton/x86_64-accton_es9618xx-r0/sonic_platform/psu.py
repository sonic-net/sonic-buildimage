#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os.path

try:
    from sonic_platform_base.psu_base import PsuBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


I2C_PATH ="/sys/bus/i2c/devices/{0}-00{1}/"

PSULED_FNODES= ["/sys/class/leds/es9618xx_led::psu1/brightness",
                "/sys/class/leds/es9618xx_led::psu2/brightness"]

PSU_NAME_LIST = ["PSU-1", "PSU-2"]
PSU_NUM_FAN = [1, 1]
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "59"
    },
    1: {
        "num": 2,
        "addr": "58"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "51"
    },
    1: {
        "num": 2,
        "addr": "50"
    },
}

class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index=0):
        PsuBase.__init__(self)
        self.index = psu_index
        self._api_helper = APIHelper()
        
        self.i2c_num = PSU_HWMON_I2C_MAPPING[self.index]["num"]
        self.i2c_addr = PSU_HWMON_I2C_MAPPING[self.index]["addr"]
        self.hwmon_path = I2C_PATH.format(self.i2c_num, self.i2c_addr)
        
        self.i2c_num = PSU_CPLD_I2C_MAPPING[self.index]["num"]
        self.i2c_addr = PSU_CPLD_I2C_MAPPING[self.index]["addr"]
        self.cpld_path = I2C_PATH.format(self.i2c_num, self.i2c_addr)
        self.__initialize_fan()
        self.PSULED_MODES = {
            "0" : self.STATUS_LED_COLOR_OFF,
            "1" : self.STATUS_LED_COLOR_GREEN,
            "3" : self.STATUS_LED_COLOR_AMBER
        }

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            if fan.get_presence():
                self._fan_list.append(fan)

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        vout_path = "{}{}".format(self.hwmon_path, 'psu_v_out')        
        vout_val=self._api_helper.read_txt_file(vout_path)
        return float(vout_val)/ 1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        iout_path = "{}{}".format(self.hwmon_path, 'psu_i_out')        
        val=self._api_helper.read_txt_file(iout_path)
        if val is not None:
            return float(val)/1000
        else:
            return 0

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        pout_path = "{}{}".format(self.hwmon_path, 'psu_p_out')        
        val=self._api_helper.read_txt_file(pout_path)
        if val is not None:
            return float(val)/1000
        else:
            return 0

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green, amber and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        mode = None
        for key, val in self.PSULED_MODES.items():
            if val == color:
                mode = key
                break
        if mode is None:
            return False
        else:
            return self._api_helper.write_txt_file(PSULED_FNODES[self.index], mode)

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        val = self._api_helper.read_txt_file(PSULED_FNODES[self.index])
        return self.PSULED_MODES[val] if val in self.PSULED_MODES else "UNKNOWN"

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp_path = "{}{}".format(self.hwmon_path, 'psu_temp1_input')
        val=self._api_helper.read_txt_file(temp_path)
        if val is not None:
            return float(val)/1000
        else:
            return 0

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 50.0

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return 12.6

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return 11.4

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
        presence_path="{}{}".format(self.cpld_path, 'psu_present')
        val=self._api_helper.read_txt_file(presence_path)
        if val is not None:
            return int(val, 10) == 1
        else:
            return 0

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        power_path="{}{}".format(self.cpld_path, 'psu_power_good')
        val = self._api_helper.read_txt_file(power_path)
        if val is not None:
            if int(val, 10) == 1:
                self.set_status_led(self.STATUS_LED_COLOR_GREEN)
                return True
            else:
                self.set_status_led(self.STATUS_LED_COLOR_AMBER)
                return False
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model_path="{}{}".format(self.cpld_path, 'psu_model_name')
        if not os.path.isfile(model_path):
            return "Unknown"

        val = open(model_path, encoding='utf-8', errors='ignore').read()
        if val is not None:
            val = val.strip()
            return val
        else:
            return "Unknown"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial_path="{}{}".format(self.cpld_path, 'psu_serial_number')
        if not os.path.isfile(serial_path):
            return "Unknown"

        val = open(serial_path, encoding='utf-8', errors='ignore').read()
        if val is not None:
            val = val.strip()
            return val
        else:
            return "Unknown"

    def is_replaceable(self):
        """
        Retrieves whether device is replaceable
        returns:
            bool: True if it is replaceable
        """
        return True

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def get_maximum_supplied_power(self):
        """
        Retrieves maximum supplied power of PSU
        Returns:
            float: Power in watts
        """
        return self.get_power()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.get_serial()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return (self.index + 1)


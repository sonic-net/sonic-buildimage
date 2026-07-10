#############################################################################
# Edgecore
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

#import sonic_platform

try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.thermal import Thermal
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


I2C_PATH ="/sys/bus/i2c/devices/{0}-00{1}/"

PSU_NAME_LIST = ["PSU-1", "PSU-2"]
PSU_NUM_FAN = [1, 1]
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "58"
    },
    1: {
        "num": 12,
        "addr": "5b"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "50"
    },
    1: {
        "num": 12,
        "addr": "53"
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

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        for fan_index in range(0, PSU_NUM_FAN[self.index]):
            fan = Fan(fan_index, 0, is_psu_fan=True, psu_index=self.index)
            self._fan_list.append(fan)
        
        self._thermal_list.append(Thermal(is_psu=True, psu_index=self.index))

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        vout_path = "{}{}".format(self.hwmon_path, 'psu_v_out')        
        vout_val=self._api_helper.read_txt_file(vout_path)
        if vout_val is not None:
            return float(vout_val)/ 1000
        else:
            return 0

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

        return False  #Controlled by HW

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status=self.get_status()
        if status is None:
            return  self.STATUS_LED_COLOR_OFF
        
        return {
            1: self.STATUS_LED_COLOR_GREEN,
            0: self.STATUS_LED_COLOR_RED            
        }.get(status, self.STATUS_LED_COLOR_OFF)

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
        return False #Not supported

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts, 
            e.g. 12.1 
        """
        vout_path = "{}{}".format(self.hwmon_path, 'psu_mfr_vout_max')        
        vout_val=self._api_helper.read_txt_file(vout_path)
        if vout_val is not None:
            return float(vout_val)/ 1000
        else:
            return 0

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts, 
            e.g. 12.1 
        """
        vout_path = "{}{}".format(self.hwmon_path, 'psu_mfr_vout_min')        
        vout_val=self._api_helper.read_txt_file(vout_path)
        if vout_val is not None:
            return float(vout_val)/ 1000
        else:
            return 0

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
        val=self._api_helper.read_txt_file(power_path)
        if val is not None:
            return int(val, 10) == 1
        else:
            return 0

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model_path="{}{}".format(self.cpld_path, 'psu_model_name')
        model=self._api_helper.read_txt_file(model_path)
        
        if model is None:
            return "N/A"
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial_path="{}{}".format(self.cpld_path, 'psu_serial_numer')
        serial=self._api_helper.read_txt_file(serial_path)
        
        if serial is None:
            return "N/A"
        return serial

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index+1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

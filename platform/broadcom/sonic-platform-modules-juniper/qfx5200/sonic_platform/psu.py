#!/usr/bin/env python
#
# SONiC Platform API implementation for PSUs
#
# Implementation is based on different SONiC platforms which are 
# avaialble in the community
#
try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


I2C_PATH ="/sys/bus/i2c/devices/{0}-00{1}/"
PSU_PRESENCE_PATH = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/psu-tmc.15/psu{}_present"

PSU_NAME_LIST = ["PSU-0", "PSU-1"]
PSU_NUM_FAN = [1, 1]
PSU_FPGA_I2C_MAPPING = {
    0: {
        "num": 3,
        "addr": "58"
    },
    1: {
        "num": 4,
        "addr": "58"
    },
}

def read_txt_file(file_path):
    try:
        with open(file_path, 'r') as fd:
            data = fd.read()
            return data.strip()
    except IOError:
        pass
    return None

def write_txt_file(file_path, value):
    try:
        with open(file_path, 'w') as fd:
            fd.write(str(value))
    except IOError:
        return False
    return True

class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index=0):
        PsuBase.__init__(self)
        self.index = psu_index

        self.i2c_num = PSU_FPGA_I2C_MAPPING[self.index]["num"]
        self.i2c_addr = PSU_FPGA_I2C_MAPPING[self.index]["addr"]
        self.hwmon_path = I2C_PATH.format(self.i2c_num, self.i2c_addr)

        self._fan_list.append(Fan(0, 1, True, self.index))
        self._fan_list.append(Fan(0, 2, True, self.index))

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        vout_path = "{}{}".format(self.hwmon_path, 'in3_input')        
        vout_val = read_txt_file(vout_path)
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
        iout_path = "{}{}".format(self.hwmon_path, 'curr2_input')        
        val = read_txt_file(iout_path)
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
        return ((self.get_voltage()) * (self.get_current()))        

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

        return True #Controlled by HW

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """

        return False  #Controlled by HW

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        val = 0
        temp = 0
        for idx in range(1, 5):
            temp_path = "{}temp{}_input".format(self.hwmon_path, idx)        
            val = read_txt_file(temp_path)
            if val is not None:
                temp += int(val)
            else:
                pass
        return float(temp)/(4 * 1000)

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
        return False

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts, 
            e.g. 12.1 
        """
        return False #Not supported

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
        presence_path = PSU_PRESENCE_PATH.format(self.index)
        val = read_txt_file(presence_path)
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
        return int(self.get_power()) > 0

#############################################################################
# Alphanetworks
#
# Module contains an implementation of SONiC Platform Base API and
# provides the thermal status which are available in the platform
#
#############################################################################

import glob
import os.path

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.index = thermal_index
        # driver path
        tmp_bus_num = [1, 4, 5]
        tmp_address = ["4f", "4d", "4c"]

        self.tmp_node = "/sys/bus/i2c/devices/{}-00{}/hwmon/".format(tmp_bus_num[self.index], tmp_address[self.index])

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "TMP75#{}".format(self.index + 1)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if (self.get_temperature() != None):
            return True
        else:
            return False

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        node = glob.glob(self.tmp_node + "hwmon*")
        if len(node) == 0:
            return False
        node = node[0] + "/temp1_input"
        if os.path.exists(node):
            return True
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        temp = 0
        node = glob.glob(self.tmp_node + "hwmon*")
        if len(node) == 0:
            return None
        node = node[0] + "/temp1_input"
        try:
            with open(node, 'r') as fp:
                temp = float(fp.read()) / 1000
        except IOError:
            return None
        return temp

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temp = 0
        node = glob.glob(self.tmp_node + "hwmon*")
        if len(node) == 0:
            return None
        node = node[0] + "/temp1_max"
        try:
            with open(node, 'r') as fp:
                temp = float(fp.read()) / 1000
        except IOError:
            return None
        return temp

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args : 
            temperature: A float number up to nearest thousandth of one degree Celsius, 
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        temp = temperature * 1000 
        node = glob.glob(self.tmp_node + "hwmon*")
        if len(node) == 0:
            return None
        node = node[0] + "/temp1_max"
        try:
            with open(node, 'w') as fp:
                fp.write(str(temp))
        except IOError:
            return False
        return True

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return None

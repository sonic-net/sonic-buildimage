#!/usr/bin/env python
 
#############################################################################
# Inventec d7264
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermal information
#
#############################################################################

import os

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

THERMAL_PATH = "/sys/bus/i2c/devices/0-0066/"

thermal_name_tup = (
    "FrontSide Temperature",
    "FanBoard Temperature",
    "NearASIC Temperature",
    "Center(U10) Temperature",
    "CPU Board Temperature",
    "ASIC Temperature"
)

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        self.index                  = thermal_index-1
        self.thermal_tmp            = "temp{}_input".format(self.index)


    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return thermal_name_tup[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        attr_path = THERMAL_PATH + self.thermal_tmp
        return os.path.isfile(attr_path)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        if (self.get_temperature() > 0):
            status = True

        return status

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        attr_path = THERMAL_PATH + self.thermal_tmp

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            temperature = float(attr_rv) / 1000

        return temperature


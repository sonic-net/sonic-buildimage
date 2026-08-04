#!/usr/bin/env python
#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    from sonic_platform_base.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_DIR = "/sys/bus/i2c/devices/0-0066/"

class Psu(PsuBase):
    def __init__(self, index):
        self.index                = index
        self.psu_presence_attr    = "psu{}".format(self.index-1)
        self.psu_status_attr      = "psoc_psu{}_iout".format(self.index)
        self.psu_power_in_attr    = "power{}_input".format(self.index)
        self.psu_power_out_attr   = "power{}_input".format(self.index)
        self.psu_voltage_out_attr = "psoc_psu{}_vin".format(self.index)
        self.psu_current_out_attr = "psoc_psu{}_iout".format(self.index)
        self.psu_serial_attr      = "psoc_psu{}_serial".format(self.index)
        self.psu_model_attr       = "psoc_psu{}_vender".format(self.index)

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

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = PSU_DIR+self.psu_presence_attr
        attr_normal = "0 : normal"
        
        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (attr_rv == attr_normal):
                presence = True

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        model = "N/A"
        attr_path = PSU_DIR+self.psu_model_attr
        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            model = attr_rv

        return model 

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = "N/A"
        attr_path = PSU_DIR+self.psu_serial_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            serial = attr_rv

        return serial 

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        attr_path = PSU_DIR+self.psu_status_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv) != 0):
                status = True

        return status

##############################################
# PSU methods
##############################################

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        voltage_out = 0.0
        attr_path = PSU_DIR+self.psu_voltage_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            voltage_out = float(attr_rv) / 1000

        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0
        attr_path = PSU_DIR+self.psu_current_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            current_out = float(attr_rv) / 1000

        return current_out

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0
        attr_path = PSU_DIR+self.psu_power_out_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            power_out = float(attr_rv) / 1000000

        return power_out

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.get_powergood_status():
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_OFF


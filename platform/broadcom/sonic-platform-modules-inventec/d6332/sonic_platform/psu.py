#!/usr/bin/env python
#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    import logging
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.inv_const import FanConst , PsuConst, Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

VOLTAGE_UPPER_LIMIT = 14
VOLTAGE_LOWER_LIMIT = 10

class Psu(PsuBase):

    __num_of_fans = 1
    __name_of_psus = ['PSU1','PSU2']

    def __init__(self, index):
        self.__index = index
        psu_id = self.__index + 1

        self.__psu_presence_attr    = "{}/i2c-inv_cpld/psu{}".format(Common.I2C_PREFIX,psu_id)
        self.__psu_voltage_out_attr = "{}/psu{}/in2_input".format(Common.INFO_PREFIX, psu_id)
        self.__psu_current_out_attr = "{}/psu{}/curr2_input".format(Common.INFO_PREFIX, psu_id)
        self.__psu_power_out_attr   = "{}/psu{}/power2_input".format(Common.INFO_PREFIX, psu_id)

        # Get the start index of fan list
        self.__fan_psu_start_index = self.__index + FanConst().PSU_FAN_START_INDEX

        # Overriding _fan_list class variable defined in PsuBase, to make it unique per Psu object
        self._fan_list = []

        # Initialize FAN
        for x in range(self.__fan_psu_start_index, self.__fan_psu_start_index + self.__num_of_fans):
            fan = Fan(x)
            self._fan_list.append(fan)

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
        return self.__name_of_psus[self.__index]

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False
        attr_path = self.__psu_presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (attr_rv == PsuConst.PSU_TYPE_LIST[0] or attr_rv == PsuConst.PSU_TYPE_LIST[1]):
                presence = True
        else:
            raise SyntaxError

        return presence

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        #model = 'Unknow'
        #attr_path = self.__psu_model_attr

        #attr_rv = self.__get_attr_value(attr_path)
        #if (attr_rv != 'ERR'):
        #    if (attr_rv != ''):
        #        model = attr_rv
        #else:
        #    raise SyntaxError

        #return model
        raise NotImplementedError

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        """
        serial = 'Unknow'
        attr_path = self.__psu_serial_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (attr_rv != ''):
                serial = attr_rv
        else:
            raise SyntaxError

        return serial
        """
        raise NotImplementedError

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status = False
        attr_path = self.__psu_presence_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (attr_rv == PsuConst.PSU_TYPE_LIST[1]):
                status = True
        else:
            raise SyntaxError

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
        attr_path = self.__psu_voltage_out_attr

        if(self.get_presence()):
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                voltage_out = float(attr_rv) / 1000
            else:
                raise SyntaxError

        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0
        attr_path = self.__psu_current_out_attr

        if(self.get_presence()):
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                current_out = float(attr_rv) / 1000
            else:
                raise SyntaxError

        return current_out

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0
        attr_path = self.__psu_power_out_attr

        if(self.get_presence()):
            attr_rv = self.__get_attr_value(attr_path)
            if (attr_rv != 'ERR'):
                power_out = float(attr_rv) / 1000
            else:
                raise SyntaxError

        return power_out

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        powergood_status = False
        voltage_out = self.get_voltage()

        #Check the voltage out with 12V, plus or minus 20 percentage.
        if (VOLTAGE_LOWER_LIMIT <= voltage_out and voltage_out <= VOLTAGE_UPPER_LIMIT ):
            powergood_status = True

        return powergood_status

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        raise NotImplementedError

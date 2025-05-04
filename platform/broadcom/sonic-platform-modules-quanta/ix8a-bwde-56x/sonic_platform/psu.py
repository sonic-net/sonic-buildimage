#!/usr/bin/env python
#
# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import logging
    import os
    import glob
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

HWMON_IPMI_DIR = "/sys/devices/platform/quanta_hwmon_ipmi/hwmon/hwmon*/"

class Psu(PsuBase):
    def __init__(self, index):
        PsuBase.__init__(self)
        fan = Fan(index, True)
        self._fan_list.append(fan)
        self.index = index
        hwmon_dir=glob.glob(HWMON_IPMI_DIR)[0]

        current_in_prefix  = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_CURRENT_IN".format(self.index), 'curr')
        current_out_prefix = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_CURRENT_OUT".format(self.index), 'curr')
        power_in_prefix    = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_POWER_IN".format(self.index), 'power')
        power_out_prefix   = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_POWER_OUT".format(self.index), 'power')
        voltage_in_prefix  = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_VOLTAGE_IN".format(self.index), 'in')
        voltage_out_prefix = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_VOLTAGE_OUT".format(self.index), 'in')
        temp1_prefix       = self.__get_hwmon_attr_prefix(hwmon_dir, "PSU{}_TEMP1".format(self.index), 'temp')

        self.psu_current_in_attr  = current_in_prefix + 'input'
        self.psu_current_out_attr = current_out_prefix + 'input'
        self.psu_power_in_attr    = power_in_prefix + 'input'
        self.psu_power_out_attr   = power_out_prefix + 'input'
        self.psu_voltage_in_attr  = voltage_in_prefix + 'input'
        self.psu_voltage_out_attr = voltage_out_prefix + 'input'
        self.psu_status_attr      = current_out_prefix + 'input'
        self.psu_presence_attr    = power_out_prefix + 'present'
        self.psu_serial_attr      = power_out_prefix + 'sn'
        self.psu_model_attr       = power_out_prefix + 'model'
        self.psu_mfr_id_attr      = power_out_prefix + 'mfrid'
        self.psu_capacity_attr    = power_out_prefix + 'pout_max'
        self.psu_type_attr        = power_out_prefix + 'vin_type'
        self.psu_temp_attr        = temp1_prefix + 'input'

    def __get_hwmon_attr_prefix(self, dir, label, type):

        retval = 'ERR'
        if not os.path.isdir(dir):
            return retval

        try:
            for filename in os.listdir(dir):
                if filename[-5:] == 'label' and type in filename:
                    file_path = os.path.join(dir, filename)
                    if os.path.isfile(file_path) and label == self.__get_attr_value(file_path):
                        return file_path[0:-5]
        except Exception as error:
            logging.error("Error when getting {} label path: {}".format(label, error))

        return retval

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open {} file: {}".format(attr_path, error))

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
        attr_normal = '1'

        attr_rv = self.__get_attr_value(self.psu_presence_attr)
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
        attr_rv = self.__get_attr_value(self.psu_model_attr)
        if (attr_rv != 'ERR'):
            model = attr_rv

        return model

    def get_mfr_id(self):
        """
        Retrieves the manufacturer's name (or id) of the device

        Returns:
            string: Manufacturer's id of device
        """
        mfr_id = "N/A"
        attr_rv = self.__get_attr_value(self.psu_mfr_id_attr)
        if (attr_rv != 'ERR'):
            mfr_id = attr_rv

        return mfr_id

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        serial = "N/A"
        attr_rv = self.__get_attr_value(self.psu_serial_attr)
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
        attr_rv = self.__get_attr_value(self.psu_status_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
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
        attr_rv = self.__get_attr_value(self.psu_voltage_out_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
            voltage_out = float(attr_rv) / 1000

        return voltage_out

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_out = 0.0
        attr_rv = self.__get_attr_value(self.psu_current_out_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
            current_out = float(attr_rv) / 1000

        return current_out

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        voltage_in = 0.0
        attr_rv = self.__get_attr_value(self.psu_voltage_in_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
            voltage_in = float(attr_rv) / 1000

        return voltage_in

    def get_input_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        current_in = 0.0
        attr_rv = self.__get_attr_value(self.psu_current_in_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
            current_in = float(attr_rv) / 1000

        return current_in

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        power_out = 0.0
        attr_rv = self.__get_attr_value(self.psu_power_out_attr)
        if (attr_rv != 'ERR'):
            attr_rv, dummy = attr_rv.split('.', 1)
            power_out = float(attr_rv) / 1000

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

    def get_type(self):
        """
        Gets the type of the PSU

        Returns:
            A string, the type of PSU (AC/DC)
        """
        type = "AC"
        attr_rv = self.__get_attr_value(self.psu_type_attr)
        if (attr_rv != 'ERR'):
            type = attr_rv

        return type

    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts

        Returns:
            An integer, the capacity of PSU
        """
        capacity = 0
        attr_rv = self.__get_attr_value(self.psu_capacity_attr)
        if (attr_rv != 'ERR'):
            try:
                capacity = int(attr_rv)
            except ValueError:
                capacity = 0

        return capacity

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """

        tout = 0.0
        attr_rv = self.__get_attr_value(self.psu_temp_attr)
        if (attr_rv != 'ERR'):
            tout = float(attr_rv)

        # tout is in milli degree celcius
        return float(tout/1000.0)


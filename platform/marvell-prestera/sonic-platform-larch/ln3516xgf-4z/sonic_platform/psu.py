#!/usr/bin/env python3

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

import os
import logging

try:
    from sonic_platform_base.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

TLV_ATTR_TYPE_MODEL = 2
TLV_ATTR_TYPE_SERIAL = 5

PSU_EEPROM_PATH = "/sys/bus/i2c/devices/{}-00{}/eeprom"
PSU_HWMON_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/hwmon"
PSU_I2C_MAPPING = {
    0: {
        "i2c_num": 3,
        "psu_addr": "58",
        "eeprom_addr": "50"
    },
    1: {
        "i2c_num": 4,
        "psu_addr": "58",
        "eeprom_addr": "50"
    },
}

class Psu(PsuBase):
    """Platform-specific Psu class"""

    PSU_NAME_LIST = ["PSU-1", "PSU-2"]

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.index = psu_index
        self.eeprom_path = PSU_EEPROM_PATH.format(PSU_I2C_MAPPING[self.index]['i2c_num'], PSU_I2C_MAPPING[self.index]['eeprom_addr'])
        self.hwmon_path = PSU_HWMON_PATH.format(PSU_I2C_MAPPING[self.index]['i2c_num'], PSU_I2C_MAPPING[self.index]['psu_addr'])

    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip('\r\n')

        fd.close()
        return retval

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self.get_status()

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.PSU_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        return True # TODO
    
    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True # TODO

    def get_voltage(self):
        """
        Retrieves current PSU voltage output
        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        psu_voltage = self._get_hwmon_value("in{}_input", "vout1", "in")
        return float(psu_voltage) / 1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU
        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        psu_current = self._get_hwmon_value("curr{}_input", "iout1", "cur")
        return float(psu_current) / 1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        psu_power = self._get_hwmon_value("power{}_input", "pout1", "power")
        return float(psu_power) / 1000000

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        model = self._read_eeprom(TLV_ATTR_TYPE_MODEL)
        if not model:
            return "N/A"
        return model

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        serial = self._read_eeprom(TLV_ATTR_TYPE_SERIAL)
        if not serial:
            return "N/A"
        return serial

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def _get_hwmon_value(self, name, label, start):
        value = 0.0

        label_path = self._search_file_by_contain(
            self.hwmon_path, label, start)
        if label_path:
            dir_name = os.path.dirname(label_path)
            basename = os.path.basename(label_path)
            in_num = ''.join(list(filter(str.isdigit, basename)))
            file_path = os.path.join(dir_name, name.format(in_num))
            val = self.get_attr_value(file_path)
            if val != 'ERR':
                value = float(val)
        return value

    def _search_file_by_contain(self, directory, search_str, file_start):
        for dirpath, dirnames, files in os.walk(directory):
            for name in files:
                file_path = os.path.join(dirpath, name)
                if name.startswith(file_start) and search_str in self.get_attr_value(file_path):
                    return file_path
        return None

    def _read_eeprom(self, attr_type):
        content = []
        attr_idx = 0
        attr_length = 0

        try:
            with open(self.eeprom_path, 'r', encoding='unicode_escape') as f:
                content = f.read()
            target_offset = ord(content[4])
            target_offset *= 8  # spec defined: offset are in multiples of 8 bytes

            attr_idx = target_offset + 3
            for i in range(1, attr_type):
                if attr_idx > len(content):
                    raise SyntaxError
                attr_length = (ord(content[attr_idx])) & (0x3f)
                attr_idx += (attr_length + 1)

            attr_length = (ord(content[attr_idx])) & (0x3f)
            attr_idx += 1
        except IOError:
            print("warning [PSU] EEPROM read error: {}".format(self.eeprom_path))
            return False

        return content[attr_idx:attr_idx + attr_length]

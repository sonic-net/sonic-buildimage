#
# Name: psuutil.py version: 1.0
#
# Description: Platform-specific PSU status interface for Juniper QFX5200
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#
# Notice and Disclaimer: This code is licensed to you under the GNU General
# Public License as published by the Free Software Foundation, version 3 or
# any later version. This code is not an official Juniper product. You can
# obtain a copy of the License at <https://www.gnu.org/licenses/>
#
# OSS License:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Third-Party Code: This code may depend on other components under separate
# copyright notice and license terms.  Your use of the source code for those
# components is subject to the terms and conditions of the respective license
# as noted in the Third-Party source code file.


import os.path

try:
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    PSU_DIR = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/psu-tmc.15"

    def __init__(self):
        PsuBase.__init__(self)


# Get sysfs attribute


    def get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return int(retval)

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device
        :return: An integer, the number of PSUs available on the device
         """
        MAX_PSUS = 2
        return MAX_PSUS

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index (starts from 1) of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is\
        faulty
        """
        attr_file = 'psu' + str(index-1) + '_' + 'present'
        attr_path = self.PSU_DIR + '/' + attr_file
        attr_value = self.get_attr_value(attr_path)

        return attr_value == 1

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by index <index>
        :param index: An integer, index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        attr_file = 'psu' + str(index-1) + '_' + 'present'
        attr_path = self.PSU_DIR + '/' + attr_file
        attr_value = self.get_attr_value(attr_path)
        return attr_value == 1

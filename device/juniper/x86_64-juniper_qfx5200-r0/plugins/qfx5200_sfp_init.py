#!/usr/bin/env python3
#
# Name: juniper_sfp_init.py version: 1.0
#
# Description: Platform-specific SFP Transceiver Initialization for Juniper QFX5200
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

import time
import os.path
import sfputil as jnpr_sfp
from sonic_py_common.logger import Logger
from pprint import pprint

SYSLOG_IDENTIFIER = "sfputil"

# Global logger class instance
logger = Logger(SYSLOG_IDENTIFIER)

DEBUG = False


def i2c_eeprom_dev_update(port, create_eeprom):
    eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
    i2c_path = "/sys/class/i2c-adapter/i2c-{0}"
    i2c_port = port + jnpr_sfp.SFP_I2C_OFFSET
    port_eeprom_path = eeprom_path.format(i2c_port)
    port_i2c_path = i2c_path.format(i2c_port)
    if create_eeprom:
        if not os.path.exists(port_eeprom_path):
            try:
                i2c_file = open(port_i2c_path + "/new_device", "w")
                i2c_file.write("optoe2 0x50")
            except IOError as e:
                print("Error: unable to write to i2c file: %s" % str(e))
                return
    else:
        if os.path.exists(port_eeprom_path):
            try:
                i2c_file = open(port_i2c_path + "/delete_device", "w")
                i2c_file.write("0x50")
            except IOError as e:
                print("Error: unable to write to i2c file: %s" % str(e))
                return


def gpio_sfp_init():
    jnpr_sfp.gpio_sfp_base_init()

    time.sleep(2)

    # Reset all ports
    for port in range(jnpr_sfp.GPIO_PORT_START, jnpr_sfp.GPIO_PORT_END + 1):
        logger.log_debug("GPIO SFP port {}".format(port))

        jnpr_sfp.gpio_sfp_reset_set(port, 0)
        i2c_eeprom_dev_update(port, True)

    time.sleep(1)

    # Enable optics for all ports which have XCVRs present
    for port in range(jnpr_sfp.GPIO_PORT_START, jnpr_sfp.GPIO_PORT_END + 1):
        jnpr_sfp.gpio_sfp_lpmode_set(port, 1)


if __name__ == '__main__':

    if DEBUG == True:
        print("Initializing Juniper SFP module")

    gpio_sfp_init()
    if DEBUG == True:
        print("Juniper GPIO presence pin mapping:")
        pprint(jnpr_sfp.gpio_sfp_presence)
        print("Juniper GPIO reset pin mapping:")
        pprint(jnpr_sfp.gpio_sfp_reset)
        print("Juniper GPIO lpmode pin mapping:")
        pprint(jnpr_sfp.gpio_sfp_lpmode)

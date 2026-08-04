#
# Name: led_control.py version: 1.0
#
# Description: Platform-specific LED control functionality for Juniper QFX5200
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

try:
    from sonic_led.led_control_base import LedControlBase
    import os
    import syslog
    import glob
    from socket import *
    from select import *
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


def DBG_PRINT(str):
    syslog.openlog("ledi_control")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()


class LedControl(LedControlBase):
    """Platform specific LED control class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"
    LED_MODE_OFF = 0
    LED_MODE_GREEN = 1
    PORT_START = 0
    PORT_END = 127
    port_to_gpio_pin_mapping = {}
    GPIO_SLAVE0_PORT_START = 0
    GPIO_SLAVE0_PORT_END = 63
    GPIO_SLAVE1_PORT_START = 64
    GPIO_SLAVE1_PORT_END = 127

    GPIO_LANE0_PORT_LED_OFFSET = 64
    GPIO_LANE1_PORT_LED_OFFSET = 80
    GPIO_LANE2_PORT_LED_OFFSET = 96
    GPIO_LANE3_PORT_LED_OFFSET = 112

    # Turn OFF all port leds during init
    def gpio_create_file(self, gpio_pin):
        gpio_export_path = "/sys/class/gpio/export"
        gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
        if not os.path.exists(gpio_pin_path):
            try:
                gpio_export_file = open(gpio_export_path, 'w')
                gpio_export_file.write(str(gpio_pin))
                gpio_export_file.close()
            except IOError as e:
                print("Error: unable to open export file: %s" % str(e))
                return False

        return True

    def gpio_port_led_init(self, gpio_base, port):
        port_led_pin = gpio_base + self.GPIO_LANE0_PORT_LED_OFFSET + 16*(port % 4) + ((port % 64)/4)
        if self.gpio_create_file(port_led_pin):
            self.port_to_gpio_pin_mapping[port] = port_led_pin

    def gpio_port_led_slave_init(self, gpio_base_path, gpio_port_start, gpio_port_end):
        flist = glob.glob(gpio_base_path)
        if len(flist) == 1:
            try:
                fp = open(flist[0] + "/base")
                gpio_base = int(fp.readline().rstrip())
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return
        for port in range(gpio_port_start, gpio_port_end + 1):
            self.gpio_port_led_init(gpio_base, port)

    def gpio_port_led_base_init(self):
        self.gpio_port_led_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.22/gpio/gpio*",
                                      self.GPIO_SLAVE0_PORT_START, self.GPIO_SLAVE0_PORT_END)
        self.gpio_port_led_slave_init("/sys/bus/platform/drivers/gpioslave-tmc/gpioslave-tmc.21/gpio/gpio*",
                                      self.GPIO_SLAVE1_PORT_START, self.GPIO_SLAVE1_PORT_END)

    # Write driver for port led
    def gpio_led_write(self, gpio_pin, value):
        success = False
        gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)

        try:
            gpio_file = open(gpio_pin_path + "/value", 'w')
            gpio_file.write(str(value))
            success = True
        except IOError as e:
            print("error: unable to open file: %s" % str(e))

        return success

    # Read driver for port led
    def gpio_led_read(self, gpio_pin):
        gpio_pin_path = "/sys/class/gpio/gpio" + str(gpio_pin)
        value = 0

        try:
            reg_file = open(gpio_pin_path + "/value")
            value = int(reg_file.readline().rstrip())
        except IOError as e:
            print("error: unable to open file: %s" % str(e))

        return value

    def _initDefaultConfig(self):
        DBG_PRINT("start init led")

        for port in range(self.PORT_START, self.PORT_END):
            self._port_led_mode_update(self.port_to_gpio_pin_mapping[port], self.LED_MODE_OFF)

        DBG_PRINT("init led done")

    # Helper method to map SONiC port name to index
    def _port_name_to_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        return port_idx

    # Convert state up/down to Green/OFF which is 1/0
    def _port_state_to_mode(self, state):
        if state == "up":
            return self.LED_MODE_GREEN
        else:
            return self.LED_MODE_OFF

    # Set the port led mode to Green/OFF

    def _port_led_mode_update(self, gpio_pin, ledMode):
        if ledMode == self.LED_MODE_GREEN:
            self.gpio_led_write(gpio_pin, 1)
        else:
            self.gpio_led_write(gpio_pin, 0)

    # Concrete implementation of port_link_state_change() method

    def port_link_state_change(self, portname, state):
        port_idx = self._port_name_to_index(portname)
        gpio_pin = self.port_to_gpio_pin_mapping[port_idx]

        ledMode = self._port_state_to_mode(state)
        saveMode = self.gpio_led_read(gpio_pin)

        if ledMode == saveMode:
            return

        self._port_led_mode_update(gpio_pin, ledMode)
        DBG_PRINT("update {} led mode from {} to {}".format(portname, saveMode, ledMode))

    # Constructor
    def __init__(self):
        self.gpio_port_led_base_init()
        self._initDefaultConfig()

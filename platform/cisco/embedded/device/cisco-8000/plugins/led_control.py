# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

'''
Platform-specific port LED control functionality for SONiC
'''

import cisco.pacific.platform
from sonic_led.led_control_base import LedControlBase
from sonic_py_common import daemon_base, multi_asic
from swsscommon import swsscommon

class LedControl(LedControlBase):
    '''
    Platform specific port LED control class
    '''

    LED_GREEN = 'green'
    LED_YELLOW = 'yellow'
    LED_COLORS = {
        LED_GREEN,
        LED_YELLOW
    }

    LED_ON = 'on'
    LED_OFF = 'off'
    LED_STATES = {
        LED_OFF: 0,
        LED_ON: 255
    }

    def __init__(self):
        LedControlBase.__init__(self)

        self.platform = cisco.pacific.platform.platform_mgr()

        self.interface_states = [set() for _ in range(len(self.platform.port_map))]

        led_data = self.platform.port_metadata.get('leds', {})
        self.led_sysfs_paths = {color: led_data[color] for color in self.LED_COLORS if color in led_data}

        if multi_asic.is_multi_asic():
            swsscommon.SonicDBConfig.initializeGlobalConfig()

        self.port_tables = []
        for namespace in multi_asic.get_front_end_namespaces():
            self.port_tables.append(swsscommon.Table(daemon_base.db_connect('CONFIG_DB', namespace), 'PORT'))

        # Initialize: Turn all front panel port LEDs off
        for port_number in range(len(self.interface_states)):
            for led_color in self.led_sysfs_paths:
                self.sysfs_port_write(led_color, port_number, self.LED_OFF)

    def sysfs_port_write(self, led_color, port_number, led_state):
        '''
        Write LED state for specified color to sysfs
        :param led_color: A string, a valid class defined LED color
        :param port_number: An integer, the port number
        :param led_state: A string, a valid class defined LED state
        '''
        if led_color not in self.led_sysfs_paths or led_state not in self.LED_STATES:
            return
        with open(self.led_sysfs_paths[led_color].format(port_number), 'w') as led_f:
            led_f.write('{:d}'.format(self.LED_STATES[led_state]))

    def port_name_to_port_number(self, port_name):
        '''
        Find the physical port to which the named SONiC port exists
        :param port_name: A string, SONiC port name (e.g., 'Ethernet0')
        :return: Physical port number, -1 on failure
        '''
        for port_table in self.port_tables:
            found, index = port_table.hget(port_name, 'index')
            if found:
                try:
                    return int(index)
                except (KeyError, TypeError):
                    break
        return -1

    def port_link_state_change(self, port, state):
        '''
        Called when port link state changes. Updates port link state LEDs
        :param port: A string, SONiC port name (e.g., 'Ethernet0')
        :param state: A string, the port link state (either 'up' or 'down')
        '''
        port_number = self.port_name_to_port_number(port)

        if port_number < 0 or port_number >= len(self.interface_states):
            return

        if state == 'up':
            self.interface_states[port_number].add(port)
        elif state == 'down':
            self.interface_states[port_number].discard(port)
        else:
            return

        green_state = self.LED_ON if self.interface_states[port_number] else self.LED_OFF
        yellow_state = self.LED_OFF

        self.sysfs_port_write(self.LED_GREEN, port_number, green_state)
        self.sysfs_port_write(self.LED_YELLOW, port_number, yellow_state)

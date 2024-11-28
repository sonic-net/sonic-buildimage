#!/usr/bin/env python
#
# Copyright (C) 2023 Wistron Corporation
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

try:
    import sys
    import subprocess
    import os
    import time
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
NUM_POE_PORT = 48
POE_PORT_FAILED_STATUS_CMD = "poetool port get_port_status {0}"
POE_ALL_PORT_POWER_STATUS_CMD = "poetool port get_all_ports_power_state"
POE_PORT_STATUS_GROUP = [1,2,3,4,5,6]
POE_LED_BLINKING = 2
POE_LED_ENABLE = 1
POE_LED_DISABLE = 0

LED_STATE = [0] * 48

#need to define all failed value and then parse it
#POE_PORT_FAILED_STATUS_CODE = [0x7, 0x8, 0xc, 0x11, 0x12, 0x1c, 0x1e, 0x1f, 0x20, 0x21, 0x24, 0x25, 0x26, 0x36, 0x43, 0x44, 0x45, 0xa0, 0xa7]
POE_PORT_FAILED_STATUS_CODE = [0x7, 0x8, 0xc, 0x11, 0x12, 0x1e, 0x1f, 0x20, 0x21, 0x24, 0x25, 0x26, 0x36, 0x43, 0x44, 0x45, 0xa0, 0xa7]

class poe_led_monitor(object):
    def __init__(self):
        for port in range(0, NUM_POE_PORT):
            self.set_led(port, POE_LED_DISABLE)
            LED_STATE[port] = POE_LED_DISABLE

    def update_port_led(self):
        poestatus = subprocess.getoutput(POE_ALL_PORT_POWER_STATUS_CMD)
        power_status = []

        for i in POE_PORT_STATUS_GROUP:
            power_status.append(hex(int(poestatus.split("\n\n")[i].split("  ")[1])))

        for port in range(0, NUM_POE_PORT):
            if ( int(power_status[(port // 8)],base=16) & (0x1 << (port - (8 * (port // 8)))) ):
                st = POE_LED_ENABLE
            else:
                st = POE_LED_DISABLE

            if LED_STATE[port] != st:
                self.set_led(port, st)
                LED_STATE[port] = st

    def chk_port_fault(self, start_port, num_port):
        for port in range(start_port, start_port + num_port):
            poefailedstatus = subprocess.getoutput(POE_PORT_FAILED_STATUS_CMD.format(port))
            failed_code = poefailedstatus.split("\n\n")[1].split("  ")[1]
            if int(failed_code) in POE_PORT_FAILED_STATUS_CODE:
                if LED_STATE[port] != POE_LED_BLINKING:
                    self.set_led(port, POE_LED_BLINKING)
                    LED_STATE[port] = POE_LED_BLINKING

    def __write_txt_file(self, file_path, data):
        try:
            with open(file_path, 'w') as fd:
                fd.write(data)
        except IOError:
            pass
        return None

    def set_led(self, port, state):
        path= "/sys/bus/i2c/devices/0-0033/port{}_poe_led".format(port+1)
        self.__write_txt_file(path, str(state))

def main():
    monitor = poe_led_monitor()
    port_cnt = 0

    # Loop forever:
    while True:
        time.sleep(1.5)
        monitor.update_port_led()
        monitor.chk_port_fault(port_cnt, 8)
        port_cnt += 8
        if port_cnt == 48:
            port_cnt = 0

        #workaround for poe temperature
        os.system("poetool device get_dev_status 0 | grep temperature | awk '{printf $2}' > /tmp/poe_temp; docker cp /tmp/poe_temp pmon:/")

if __name__ == '__main__':
    main()

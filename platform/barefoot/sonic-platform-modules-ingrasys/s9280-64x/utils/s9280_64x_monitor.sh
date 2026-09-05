#!/bin/bash
# Copyright (C) 2016 Ingrasys, Inc.
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

INTERVAL=5
I2C_UTILS="/usr/sbin/i2c_utils.sh"


# TBD:  LED status monitor
function _led_monitor {
    ${I2C_UTILS} i2c_led_fan_status_set >/dev/null
    ${I2C_UTILS} i2c_led_psu_status_set >/dev/null
    ${I2C_UTILS} i2c_led_fan_tray_status_set >/dev/null
}

# main function
function _main {
    while true
    do
        #PSU controlled by dummy board, 
        #but fan LED and fan tray LED must controlled by this service
        _led_monitor

        # Sleep while still handling signals
        sleep $INTERVAL &
        wait $!
    done
}

_main

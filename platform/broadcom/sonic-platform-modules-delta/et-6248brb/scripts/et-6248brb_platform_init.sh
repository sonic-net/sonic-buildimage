#!/bin/bash

#platform init script for Delta et-6248brb

#fan speed monitol start
/usr/share/sonic/device/x86_64-delta_et-6248brb-r0/fancontrol.service /usr/share/sonic/device/x86_64-delta_et-6248brb-r0/fancontrol &
sh /usr/local/bin/led_status.sh &


exit 0


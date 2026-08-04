#!/bin/bash

#platform init script for Delta ag5648

#fan speed monitor start
/usr/share/sonic/device/x86_64-delta_ag5648-r0/fancontrol.service /usr/share/sonic/device/x86_64-delta_ag5648-r0/fancontrol &

#led control
/usr/local/bin/led_control &


exit 0


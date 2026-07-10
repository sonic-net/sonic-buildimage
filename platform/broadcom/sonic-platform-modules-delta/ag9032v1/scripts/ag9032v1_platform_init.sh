#!/bin/bash

#platform init script for Delta ag9032v1

#fan speed monitol start
/usr/share/sonic/device/x86_64-delta_ag9032v1-r0/fancontrol.service /usr/share/sonic/device/x86_64-delta_ag9032v1-r0/fancontrol &

#set led status
/usr/local/bin/led_status.sh &

exit 0


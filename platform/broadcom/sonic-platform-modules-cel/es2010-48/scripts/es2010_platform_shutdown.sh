#!/bin/bash

WATCHDOG_UTIL="/usr/local/bin/watchdogutil"
REBOOT_CAUSE_DIR="/host/reboot-cause"
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
REBOOT_TIME=$(date)

if [ $# -lt 1 ]; then
    echo "Require reboot type"
    exit 1
fi

if [ ! -d "$REBOOT_CAUSE_DIR" ]; then
    mkdir $REBOOT_CAUSE_DIR
fi

echo "Reason:$1,Time:${REBOOT_TIME}" > ${HW_REBOOT_CAUSE_FILE}

# Best effort to write buffered data onto the disk
sync ; sync ; sync ; sleep 3

# Set System LED to booting pattern
echo "alternate_blink_4hz" > /sys/bus/platform/devices/sys_cpld1/sys_led

# Disable platform-specific watchdog
if [ -x ${WATCHDOG_UTIL} ]; then
    echo "Disable platform-specific watchdog"
    ${WATCHDOG_UTIL} disarm
    sleep 2
fi

# CPLD CPU cold power-cycle
if [ $# -eq 2 ] && [ "$2" == "skip-poe" ]; then
    echo 0xA256 0x01 > /sys/devices/platform/sys_cpld1/setreg
    echo 0xA218 0x00 > /sys/devices/platform/sys_cpld1/setreg
else
    echo 0xA218 0x00 > /sys/devices/platform/sys_cpld1/setreg
fi

# System should reboot by now and avoid the script returning to caller
sleep 10

exit 0

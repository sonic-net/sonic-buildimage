#!/bin/bash
REBOOT_CAUSE_DIR="/host/reboot-cause"
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
REBOOT_TIME=$(date)

if [ $# -ne 1 ]; then
    echo "Require reboot type"
    exit 1
fi

if [ ! -d "$REBOOT_CAUSE_DIR" ]; then
    mkdir $REBOOT_CAUSE_DIR
fi

echo "Reason:$1,Time:${REBOOT_TIME}" > ${HW_REBOOT_CAUSE_FILE}

# Best effort to write buffered data onto the disk
sync ; sync ; sync ; sleep 3

platform=`cat /host/machine.conf | sed -n 's/onie_platform=\(.*\)/\1/p'`
bmc_present=$(cat /sys/devices/platform/sys_cpld/bmc_present_l)

# Set System LED to booting pattern
echo "alternate_blink_4hz" > /sys/bus/platform/devices/sys_cpld/sys_led

if [[ "$bmc_present" -eq 0 ]]; then
    # BMC cold power-cyle
    ipmitool chassis power cycle &> /dev/null
else
    # CPLD cold power-cyle
    echo 0XA164 0x00 > /sys/devices/platform/sys_cpld/setreg &> /dev/null
fi

# System should reboot by now and avoid the script returning to caller
sleep 10

exit 0

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

bmc_present=`cat /usr/share/sonic/device/x86_64-cel_silverstone_v2-r0/bmc_status`

if [[ "$bmc_present" == "True" ]]; then
    # Set all LEDs to BMC's control
    ipmitool raw 0x3a 0x42 0x02 0x01 &> /dev/null

    # BMC cold power-cyle
    ipmitool chassis power cycle &> /dev/null
else
    # Set System LED to booting pattern
    i2cset -f -y 100 0x0d 0x62 0x02 &> /dev/null

    # CPLD cold power-cyle
    i2cset -f -y 100 0x0d 0x64 0x00 &> /dev/null
fi

# System should reboot by now and avoid the script returning to caller
sleep 10

exit 0

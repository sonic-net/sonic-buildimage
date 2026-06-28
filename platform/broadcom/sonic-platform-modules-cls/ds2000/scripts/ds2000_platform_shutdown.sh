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

# Get BMC mode
GETREG_PATH="/sys/devices/platform/sys_cpld/getreg"
BMC_PRESENCE=`echo '0xA108' > $GETREG_PATH && cat $GETREG_PATH`
echo "BMC card ${BMC_PRESENCE}"

if [ ${BMC_PRESENCE} == "0x00" ]; then
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

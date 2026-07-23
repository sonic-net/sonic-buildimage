#!/bin/bash
REBOOT_CAUSE_DIR="/host/reboot-cause"
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
REBOOT_TIME=$(date)

if [ $# -ne 1 ] || [ -z "$1" ]; then
    echo "Error: A non-empty reboot type argument is required."
    exit 1
fi

mkdir -p "$REBOOT_CAUSE_DIR"

echo "Reason:\"$1\",Time:\"${REBOOT_TIME}\"" > "${HW_REBOOT_CAUSE_FILE}"

# Best effort to write buffered data onto the disk
sync ; sync ; sync ; sleep 3

bmc_present=$(cat /sys/devices/platform/sys_cpld/bmc_present)

# Set System LED to booting pattern
echo "alternate_blink_4hz" > /sys/bus/platform/devices/sys_cpld/sys_led

if [[ "$bmc_present" == "1" ]]; then
    # BMC cold power-cyle
    ipmitool chassis power cycle &> /dev/null
    if [ $? -ne 0 ]; then
        echo "ERROR: ipmitool power cycle command failed." >&2
        exit 1
    fi
else
    # CPLD CPU cold power-cycle
    i2cset -y -f 6 0x0d 0x64 0x00
    if [ $? -ne 0 ]; then
        echo "ERROR: i2cset command failed to trigger reboot." >&2
        exit 1
    fi
fi

# System should reboot by now and avoid the script returning to caller
sleep 10

exit 0

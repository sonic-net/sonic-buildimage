#!/bin/bash
WATCHDOG_UTIL="/usr/local/bin/watchdogutil"
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

# re-arm to 240s for the slowly startup of BIOS after enable the PFR/secure boot feature 
${WATCHDOG_UTIL} arm -s 240

if [[ "$bmc_present" == "1" ]]; then
    # BMC cold power-cyle
    ipmitool chassis power cycle &> /dev/null
else
    # CPLD cold power-cyle
    echo 0xA64 0x00 > /sys/devices/platform/sys_cpld/setreg
fi

echo "Platform cold reboot triggered"

# System should reboot by now and avoid the script returning to caller
sleep 10

exit 0

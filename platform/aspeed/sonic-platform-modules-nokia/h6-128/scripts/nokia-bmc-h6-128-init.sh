#!/bin/bash

# Platform init script
FLAG_FILE="/tmp/nokia_bmc_h6_128_init_done"
if [ ! -f "$FLAG_FILE" ]; then
    sudo touch "$FLAG_FILE"
else
    echo "Script has already run since the last reboot. Exiting."
    exit 0
fi

file_exists() {
    # Wait 10 seconds max till file exists
    for((i=0; i<10; i++));
    do
        [[ -f $1 ]] && return 0
        sleep 1
    done
    return 1
}

assign_mac_eth0()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    if [ -n $MAC_ADDR ]; then
        sudo ifconfig eth0 down
        sudo ifconfig eth0 hw ether $MAC_ADDR
        sudo ifconfig eth0 up
        echo "Nokia-BMC-H6-128: Updating BMC eth0 mac address ${MAC_ADDR}"
    else
        echo "ERROR: MAC address is not found in SYS_EEPROM."
    fi
}

# Disable sysrq-trigger
echo 0 > /proc/sys/kernel/sysrq

assign_mac_eth0

EEPROM_PATH="/sys/bus/i2c/devices/13-0056/eeprom"
if file_exists "$EEPROM_PATH"; then
    chmod 644 "$EEPROM_PATH"
else
    echo "Error: SYSEEPROM file not found" >&2
    exit 1
fi

exit 0

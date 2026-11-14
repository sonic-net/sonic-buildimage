#!/bin/bash

# Platform init script
ln1524g4mg_6xgf_profile()
{
	MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
	if [ -z "$MAC_ADDR" ]; then
	    echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
	    echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
	    return 0
	fi
	find /usr/share/sonic/device/*ln1524g4mg-6xgf* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
	echo "ln3824g: Updating switch mac address ${MAC_ADDR}"
}

# Main
ln1524g4mg_6xgf_profile

# LOGIC to enumerate SFP eeprom devices - send 0x50 to kernel i2c driver - initialize devices
# the mux may be enumerated at number 4 or 5 so we check for the mux and skip if needed
# Get list of the mux channels
# Enumerate the SFP eeprom device on each mux channel
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-0/new_device
# Enumerate system eeprom
echo 24c64 0x52 > /sys/bus/i2c/devices/i2c-0/new_device
sleep 2


exit 0

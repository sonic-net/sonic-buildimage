#!/bin/bash

# Platform init script
ln3848g_profile()
{
	# Read original_platform from machine.conf
    if [ -f /etc/machine.conf ]; then
        original_platform=$(grep -E '^onie_original_platform=' /etc/machine.conf | cut -d'=' -f2)
    elif [ -f /host/machine.conf ]; then
        original_platform=$(grep -E '^onie_original_platform=' /host/machine.conf | cut -d'=' -f2)
    else
        original_platform=""
    fi

    # Apply patch only for specific platform
    if [ "$original_platform" = "test-alias_ln3848g" ]; then
        # Patch /etc/fw_env.config
        if [ -w /etc/fw_env.config ]; then
            echo "/dev/mtd0 0x003C0000 0x10000 0x10000" > /etc/fw_env.config
            echo "ln3848g: Patched /etc/fw_env.config for fw_printenv"
        else
            echo "Warning: Cannot write to /etc/fw_env.config"
        fi
    fi

	MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)

	if [ -z "$MAC_ADDR" ]; then

	    echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"

	    echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2

	    return 0

	fi
	find /usr/share/sonic/device/*ln3848g* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
	echo "ln3848g: Updating switch mac address ${MAC_ADDR}"
}

# Main
ln3848g_profile

# Set default fan speed to 100% (reg 0x01 = 0x0a) at boot.
# thermalctld will adjust based on temperature after startup.
i2cset -y 0 0x33 0x01 10

# NOTE: I2C device enumeration (PCA9548 mux, SFP EEPROMs, system EEPROM) is
# now handled by pddf-platform-init.service (pddf_util.py install) which runs
# before this script. Manual enumeration has been removed to avoid conflicts.

exit 0

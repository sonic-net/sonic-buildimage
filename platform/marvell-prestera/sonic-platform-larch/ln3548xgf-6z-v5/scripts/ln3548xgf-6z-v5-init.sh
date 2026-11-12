#!/bin/bash

# Platform init script for ln3548xgf-6z-v5

# Returns 0 if an i2c device acknowledges at the given bus/address, 1 otherwise.
# Used to detect PSU physical presence before binding a driver, avoiding i2c bus
# issues on ln3548xgf-6z-v5 hardware when a PSU is unplugged.
psu_i2c_present() {
    local bus=$1
    local addr=$2
    i2cget -y "$bus" "$addr" > /dev/null 2>&1
}

ln3548xgf-6z-v5_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*ln3548xgf-6z-v5* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln3548xgf-6z-v5: Updating switch mac address ${MAC_ADDR}"
    ifconfig eth0 hw ether $MAC_ADDR

    modprobe optoe

    # PSU-1: only create pmbus device if the PSU is physically present.
    # EEPROM (24c02) is not registered - model/serial read via PMBus vendor
    # registers (0xE0-0xEB model, 0xD4-0xDE serial) using i2ctransfer -f.
    if psu_i2c_present 2 0x3c; then
        echo "ln3548xgf-6z-v5: PSU-1 present, creating i2c device"
        echo pmbus 0x3c > /sys/bus/i2c/devices/i2c-2/new_device
    else
        echo "ln3548xgf-6z-v5: PSU-1 not present, skipping i2c device creation"
    fi

    # PSU-2: same guard as PSU-1.
    if psu_i2c_present 2 0x3d; then
        echo "ln3548xgf-6z-v5: PSU-2 present, creating i2c devices"
        echo pmbus 0x3d > /sys/bus/i2c/devices/i2c-2/new_device
    else
        echo "ln3548xgf-6z-v5: PSU-2 not present, skipping i2c device creation"
    fi

    #echo max31785 0x54 > /sys/bus/i2c/devices/i2c-2/new_device
    echo pca9555 0x21 > /sys/bus/i2c/devices/i2c-2/new_device

    # modprobe hid_mcp2221
}

# - Main entry
ln3548xgf-6z-v5_profile

exit 0

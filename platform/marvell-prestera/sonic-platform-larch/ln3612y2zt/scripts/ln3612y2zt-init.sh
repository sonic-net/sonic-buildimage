#!/bin/bash

# Platform init script for ln3612y2zt

ln3612y2zt_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$MAC_ADDR" ]; then
        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
        return 0
    fi
    find /usr/share/sonic/device/*ln3612y2zt* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln3612y2zt: Updating switch mac address ${MAC_ADDR}"
    sudo ifconfig eth0 hw ether $MAC_ADDR
    sudo modprobe optoe
}

# - Main entry
ln3612y2zt_profile

# enable channel deselection on the muxes
echo -2 | tee /sys/bus/i2c/drivers/pca954x/*-00*/idle_state

SFP_MUX_START=30
# LOGIC to enumerate SFP eeprom devices - send 0x50 to kernel i2c driver - initialize devices
# Enumerate the SFP eeprom device on each mux channel
for i in {1..12}
do
    echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$((SFP_MUX_START + i))/new_device
done

# QSFP28 uses optoe1 (SFF-8636); bare "optoe" does not bind and creates no eeprom sysfs.
for i in {13..14}
do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-$((SFP_MUX_START + i))/new_device
done

exit 0

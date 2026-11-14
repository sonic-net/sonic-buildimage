#!/bin/bash

# Platform init script for ln3524xgf-6z-v1

ln3524xgf-6z-v1_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*ln3524xgf-6z-v1* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln3524xgf-6z-v1: Updating switch mac address ${MAC_ADDR}"
    sudo ifconfig eth0 hw ether $MAC_ADDR

    sudo modprobe optoe
}

sfp_init()
{
    # Bind optoe2 (SFP, dual-address) to 10G SFP ports 1-24 (I2C buses 11-34)
    for port in $(seq 1 24); do
        bus=$((port + 10))
        if [ -d /sys/bus/i2c/devices/i2c-${bus} ] && [ ! -e /sys/bus/i2c/devices/${bus}-0050 ]; then
            echo "optoe2 0x50" > /sys/bus/i2c/devices/i2c-${bus}/new_device || true
        fi
    done

    # Bind optoe1 (QSFP, single-address) to 100G QSFP ports 25-30 (I2C buses 35-40)
    for port in $(seq 25 30); do
        bus=$((port + 10))
        if [ -d /sys/bus/i2c/devices/i2c-${bus} ] && [ ! -e /sys/bus/i2c/devices/${bus}-0050 ]; then
            echo "optoe1 0x50" > /sys/bus/i2c/devices/i2c-${bus}/new_device || true
        fi
    done
}

i2c_init()
{
    # Enumerate the system PCA9548 mux on CP_I2C0 (bus 2).
    # This creates virtual buses 3-10 used by PSU, thermals, and FPGA.
    if [ ! -d /sys/bus/i2c/devices/i2c-7 ]; then
        echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-2/new_device || true
        sleep 1
    fi

    # PSU A: PMBus on bus 3 (PCA9548 channel 0)
    if [ -d /sys/bus/i2c/devices/i2c-3 ] && [ ! -e /sys/bus/i2c/devices/3-0058 ]; then
        echo pmbus 0x58 > /sys/bus/i2c/devices/i2c-3/new_device || true
    fi
    # PSU A: EEPROM on bus 3
    if [ -d /sys/bus/i2c/devices/i2c-3 ] && [ ! -e /sys/bus/i2c/devices/3-0050 ]; then
        echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-3/new_device || true
    fi

    # PSU B: PMBus on bus 4 (PCA9548 channel 1)
    if [ -d /sys/bus/i2c/devices/i2c-4 ] && [ ! -e /sys/bus/i2c/devices/4-0058 ]; then
        echo pmbus 0x58 > /sys/bus/i2c/devices/i2c-4/new_device || true
    fi
    # PSU B: EEPROM on bus 4
    if [ -d /sys/bus/i2c/devices/i2c-4 ] && [ ! -e /sys/bus/i2c/devices/4-0050 ]; then
        echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-4/new_device || true
    fi

    # Temperature sensors on bus 5 (PCA9548 channel 2)
    if [ -d /sys/bus/i2c/devices/i2c-5 ]; then
        [ ! -e /sys/bus/i2c/devices/5-0048 ] && echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-5/new_device || true
        [ ! -e /sys/bus/i2c/devices/5-0049 ] && echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-5/new_device || true
    fi
}

# - Main entry
ln3524xgf-6z-v1_profile
i2c_init
sfp_init

exit 0

#!/bin/bash

# Platform init script

# Load required kernel-mode drivers
load_kernel_drivers() {
#placeholder for now
	echo "Loading Kernel Drivers"
    sudo insmod /lib/modules/5.10.0-18-2-arm64/nokia_7215_ixs_a1_cpld.ko
}

nokia_7215_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/arm64-nokia_ixs7215_52xb-r0/Nokia-7215-A1/profile.ini
    sudo ifconfig eth0 hw ether $MAC_ADDR
    echo "Nokia-7215-A1: Updating switch mac address ${MAC_ADDR}"
}

# - Main entry

# Install kernel drivers required for i2c bus access
load_kernel_drivers


# LOGIC to enumerate SFP eeprom devices - send 0x50 to kernel i2c driver - initialize devices
# the mux may be enumerated at number 4 or 5 so we check for the mux and skip if needed
# Get list of the mux channels
# Enumerate the SFP eeprom device on each mux channel
#echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate system eeprom
echo 24c64 0x53 > /sys/class/i2c-adapter/i2c-0/new_device
sleep 2
chmod 644 /sys/class/i2c-adapter/i2c-0/0-0053/eeprom

# Enumerate power monitor
echo ina230 0x40 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate fan
echo emc2305 0x2e > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate RTC
echo m41t11 0x68 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate Thermals

echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-0/new_device

echo nokia_7215_a1_cpld 0x41 > /sys/bus/i2c/devices/i2c-0/new_device

# Ensure switch is programmed with base MAC addr
nokia_7215_profile

exit 0
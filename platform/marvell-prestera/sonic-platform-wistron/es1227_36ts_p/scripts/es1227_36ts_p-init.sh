#!/bin/bash

# Platform init script

# Load required kernel-mode drivers
load_kernel_drivers() {
    # Remove modules loaded during Linux init
    # FIX-ME: This will be removed in the future when Linux init no longer loads these
    rmmod i2c_dev
    rmmod i2c_mv64xxx

    # Carefully control the load order here to ensure consistent i2c bus numbering
    modprobe i2c_mv64xxx
    modprobe i2c_dev

    if ! lsmod | grep -q wistron_cpld; then
        insmod /usr/lib/modules/$(uname -r)/kernel/extra/wistron_cpld.ko
    fi

    if ! lsmod | grep -q wistron_eeprom; then
        insmod /usr/lib/modules/$(uname -r)/kernel/extra/wistron_eeprom.ko
    fi

    if ! lsmod | grep -q mvcpss; then
        insmod /usr/lib/modules/$(uname -r)/kernel/extra/mvcpss.ko
    fi

    modprobe optoe
    modprobe jc42
}

# - Main entry

# Install kernel drivers required for i2c bus access
load_kernel_drivers
#entropy setting
#python /etc/entropy.py

# Initialize I2C devices
echo wistron_cpld 0x33 > /sys/bus/i2c/devices/i2c-0/new_device
#echo 2227_max31790 0x2f > /sys/bus/i2c/devices/i2c-2/new_device
#echo wistron_eeprom 0x50 > /sys/bus/i2c/devices/i2c-2/new_device
#echo wistron_eeprom 0x51 > /sys/bus/i2c/devices/i2c-2/new_device

# Initialize temperature sensors
echo jc42 0x1b > /sys/bus/i2c/devices/i2c-0/new_device
# Initialize PMBus power management devices
#echo pmbus 0x58 > /sys/bus/i2c/devices/i2c-2/new_device
#echo pmbus 0x59 > /sys/bus/i2c/devices/i2c-2/new_device

#tca_detect=$(i2cget -f -y 5 0x22 0x40 1>/dev/null 2>/dev/null; echo $?)
#if [ $tca_detect -eq 0 ]; then
#	i2cset -y 5 0x22 0x54 0x22
#	i2cset -y 5 0x22 0x55 0x22
#	i2cset -y 5 0x22 0x56 0x22
#    echo pcal6524 0x22 > /sys/bus/i2c/devices/i2c-5/new_device
#else
#    echo tca6424 0x22 > /sys/bus/i2c/devices/i2c-5/new_device
#fi

# Initialize SFP+ modules
#local i
for i in {4..7};
do
    echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/new_device
done

# Set temperature monitoring thresholds
#for i in {0..2};
#do
#    echo 85000 > /sys/class/hwmon/hwmon$i/temp1_max
#    echo 80000 > /sys/class/hwmon/hwmon$i/temp1_max_hyst
#done

# Set additional temperature thresholds for hwmon4 (if it exists)
if [ -e /sys/class/hwmon/hwmon4/temp1_max ]; then
    echo 80000 > /sys/class/hwmon/hwmon4/temp1_max
    echo 85000 > /sys/class/hwmon/hwmon4/temp1_crit
fi

# Enable auto LED control on ports (via CPLD)
echo 1 > /sys/bus/i2c/devices/0-0033/port_led_auto

# Initialize PoE (Power-over-Ethernet) if script exists
#if [ -f /usr/local/bin/poe_init.sh ]; then
#    sh /usr/local/bin/poe_init.sh
#fi

exit 0

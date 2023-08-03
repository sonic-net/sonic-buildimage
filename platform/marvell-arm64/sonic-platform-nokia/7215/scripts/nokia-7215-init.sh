#!/bin/bash

# Platform init script

# Load required kernel-mode drivers
load_kernel_drivers() {
    echo "Loading Kernel Drivers"
    sudo insmod /lib/modules/5.10.0-18-2-arm64/nokia_7215_ixs_a1_cpld.ko
    sudo insmod /lib/modules/5.10.0-18-2-arm64/ac5_thermal_sensor.ko
}

nokia_7215_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/arm64-nokia_ixs7215_52xb-r0/Nokia-7215-A1/profile.ini
    sudo ifconfig eth0 hw ether $MAC_ADDR
    echo "Nokia-7215-A1: Updating switch mac address ${MAC_ADDR}"
}
file_exists() {
    # Wait 10 seconds max till file exists
    for((i=0; i<10; i++));
    do
        if [ -f $1 ]; then
            return 1
        fi
        sleep 1
    done
    return 0
 }

# - Main entry

# Install kernel drivers required for i2c bus access
load_kernel_drivers

# Enumerate RTC
echo m41t11 0x68 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate the SFP eeprom device on each mux channel
echo pca9546 0x70> /sys/bus/i2c/devices/i2c-1/new_device

# Enumerate power monitor
echo ina230 0x40 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate fan
echo emc2305 0x2e > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate Thermals
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-0/new_device

#Enumerate CPLD
echo nokia_7215_a1_cpld 0x41 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate system eeprom
echo 24c64 0x53 > /sys/class/i2c-adapter/i2c-0/new_device

file_exists /sys/class/i2c-adapter/i2c-0/0-0053/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/class/i2c-adapter/i2c-0/0-0053/eeprom
else
    echo "SYSEEPROM file not foud"
fi

# Get list of the mux channels
for((i=0; i<10; i++));
    do  
        ismux_bus=$(i2cdetect -l|grep mux|cut -f1)
        if [[ $ismux_bus ]]; then
            break;
        fi
        sleep 1
    done

# Enumerate the SFP eeprom device on each mux channel
for mux in ${ismux_bus}
do
    echo optoe2 0x50 > /sys/class/i2c-adapter/${mux}/new_device
done

# Enable optical SFP Tx
for i in {49..52}
do
    echo 0 > /sys/bus/i2c/devices/0-0041/sfp${i}_tx_disable
done

#slow down fan speed to 50 untill thermal algorithm kicks in%
i2c_path="/sys/bus/i2c/devices/0-002e/hwmon/hwmon?"  
echo 128 > $i2c_path/pwm1
echo 128 > $i2c_path/pwm2 

# Ensure switch is programmed with base MAC addr
nokia_7215_profile

exit 0

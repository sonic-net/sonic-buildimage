#!/bin/bash

# Platform init script for Nokia IXS 7215

# Load required kernel-mode drivers
load_kernel_drivers() {
    # Remove modules loaded during Linux init
    # FIX-ME: This will be removed in the future when Linux init no longer loads these
    rmmod i2c_mux_gpio
    rmmod i2c_dev
    rmmod i2c_mv64xxx

    # Carefully control the load order here to ensure consistent i2c bus numbering
    modprobe i2c_mv64xxx
    modprobe i2c_dev
    modprobe i2c_mux_gpio
    modprobe eeprom
}

load_kernel_drivers_late() {
    modprobe mvcpss
    # Override mvGpioDrv.ko built-in mrvllibsai.deb with correct one
    GOOD="/usr/lib/modules/$(uname -r)/kernel/extra/mvGpioDrv.ko"
    find /var/lib/docker/overlay2/*/diff/usr/lib/modules -type f -name mvGpioDrv.ko \
         -exec sh -c 'cp -a "$0" "$1" 2>/dev/null || true' "$GOOD" {} \;
}

nokia_7215_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/armhf-nokia_ixs7215_52x-r0/Nokia-7215/profile.ini
    echo "Nokia-7215: Updating switch mac address ${MAC_ADDR}"
}

# - Main entry

# Install kernel drivers required for i2c bus access
load_kernel_drivers

# Disable sysrq-trigger
echo 0 > /proc/sys/kernel/sysrq

# LOGIC to enumerate SFP eeprom devices - send 0x50 to kernel i2c driver - initialize devices
# the mux may be enumerated at number 4 or 5 so we check for the mux and skip if needed

# Get list of the mux channels
ismux_bus=$(i2cdetect -l|grep mux|cut -f1)

# Enumerate the SFP eeprom device on each mux channel
for mux in ${ismux_bus}
do
    echo optoe2 0x50 > /sys/bus/i2c/devices/${mux}/new_device
done

# Enumerate system eeprom
echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-0/new_device
sleep 2
chmod 644 /sys/bus/i2c/devices/i2c-0/0-0053/eeprom

# Enumerate fan eeprom devices
echo eeprom 0x55 > /sys/bus/i2c/devices/i2c-0/new_device
echo eeprom 0x56 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate PSU eeprom devices
echo eeprom 0x50 > /sys/bus/i2c/devices/i2c-1/new_device
echo eeprom 0x51 > /sys/bus/i2c/devices/i2c-1/new_device

load_kernel_drivers_late

# Enable optical SFP Tx
i2cset -y -m 0x0f 0 0x41 0x5 0x00

# Ensure switch is programmed with chassis base MAC addr
nokia_7215_profile

echo "Nokia-7215 - completed platform init script"
exit 0

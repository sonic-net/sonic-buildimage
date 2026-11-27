#!/bin/bash

BMC_PRESENCE=`cat /sys/devices/platform/sys_cpld1/bmc_present`

if [ ${BMC_PRESENCE} == "0" ]; then
    # Attach pressure sensor devices
    echo mpl3115 0x60 > /sys/bus/i2c/devices/i2c-0/new_device

    # #Attach MP2975 sensor devices
    # echo mp2975 0x7e > /sys/bus/i2c/devices/i2c-0/new_device
    # insmod for DIMM Temperature
    modprobe jc42

    # Set all FAN speed to 50%
    fan_speed_register=(0x30 0x31 0x32 0x33 0x38 0x39 0x3a 0x3b)
    for reg in "${fan_speed_register[@]}"; do
        sudo i2cset -y -f 0 0x32 $reg 0x7f
    done

fi
    # # Enable FAN WDT
    # # sudo i2cset -y -f 0 0x32 0x30 0x01

    # # Set ALL FAN LED status to GREEN
    # sudo i2cset -y -f 1 0x32 0x46 0x55

    # # Set Alarm LED status to OFF, since it is unused in SONiC
    # sudo i2cset -y -f 1 0x32 0x44 0x33

    # # Set SYS LED status to GREEN
    # sudo i2cset -y -f 1 0x32 0x40 0xec

    # # Unmasking PoE reset mask
    # sudo i2cset -y -f 1 0x32 0x56 0x00

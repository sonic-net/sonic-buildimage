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
    insmod /usr/lib/modules/$(uname -r)/kernel/extra/wistron_cpld.ko
    insmod /usr/lib/modules/$(uname -r)/kernel/extra/wistron_max31790.ko
    insmod /usr/lib/modules/$(uname -r)/kernel/extra/wistron_psu.ko
    insmod /usr/lib/modules/$(uname -r)/kernel/extra/mvcpss.ko
    modprobe optoe
    modprobe jc42
}

# - Main entry

# Install kernel drivers required for i2c bus access
load_kernel_drivers
#entropy setting
#python /etc/entropy.py
    echo wistron_cpld 0x33 > /sys/bus/i2c/devices/i2c-0/new_device
    echo 1227_max31790 0x2f > /sys/bus/i2c/devices/i2c-2/new_device

	echo jc42 0x1b > /sys/bus/i2c/devices/i2c-0/new_device
    echo wistron_psu 0x58 > /sys/bus/i2c/devices/i2c-3/new_device
    echo wistron_psu 0x59 > /sys/bus/i2c/devices/i2c-3/new_device

    tca_detect=$(i2cget -f -y 5 0x22 0x40 1>/dev/null 2>/dev/null; echo $?)
    if [ $tca_detect -eq 0 ]; then
		i2cset -y 5 0x22 0x54 0x22
		i2cset -y 5 0x22 0x55 0x22
		i2cset -y 5 0x22 0x56 0x22
        echo pcal6524 0x22 > /sys/bus/i2c/devices/i2c-5/new_device
    else
        echo tca6424 0x22 > /sys/bus/i2c/devices/i2c-5/new_device
    fi

    local i
    for i in {4..9};
    do
        echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/new_device
    done
	    local j
    for j in {472..495};
    do
        echo $j > /sys/class/gpio/export
    done
    local k
    for k in $(seq 473 4 493); do
        echo out > /sys/class/gpio/gpio$k/direction
    done

    for i in {0..2};
    do
        echo 85000 > /sys/class/hwmon/hwmon$i/temp1_max
        echo 80000 > /sys/class/hwmon/hwmon$i/temp1_max_hyst
    done

	echo 80000 > /sys/class/hwmon/hwmon4/temp1_max
	echo 85000 > /sys/class/hwmon/hwmon4/temp1_crit

    echo 1 > /sys/bus/i2c/devices/0-0033/port_led_auto

    # init oob port led
    phytool write eth0/0/22 3;
    phytool write eth0/0/17 0x44a5;
    phytool write eth0/0/22 3;
    phytool write eth0/0/16 0x0240;
    phytool write eth0/0/22 0

    # set rj port sgmii amplitude
    phytool write eth0/0/22 2;
    phytool write eth0/0/26 0x8004;
    phytool write eth0/0/22 0

    echo 496 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio496/direction
    for k in $(seq 498 499); do
        echo $k > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio$k/direction
    done

    # default set sfp txdisable to off
    echo 0 > /sys/class/gpio/gpio496/value

    a=0; b=1

    echo $a > /sys/class/gpio/gpio498/value; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 1; fi

    echo $b > /sys/class/gpio/gpio499/value; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 2; fi

    # Set auto select mode - Prefer copper medium
    phytool write eth0/0/22 2;
    phytool write eth0/0/16 0xa88;
    phytool write eth0/0/22 0;
exit 0

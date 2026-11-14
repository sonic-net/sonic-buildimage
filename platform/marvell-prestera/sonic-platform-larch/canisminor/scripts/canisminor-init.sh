#!/bin/bash

# Platform init script
canisminor_profile()
{
	MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
	if [ -z "$MAC_ADDR" ]; then
	    echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
	    echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
	    return 0
	fi
	find /usr/share/sonic/device/*canisminor* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
	echo "canisminor: Updating switch mac address ${MAC_ADDR}"
}

init_led()
{
    sudo i2cset -y 0 0x33 0x36 0x2
}

init_fans()
{
    # Enable auto-control mode
    echo 2   > /sys/class/hwmon/hwmon0/pwm3_enable

    # PWM 3 min 30%
    echo 78  > /sys/class/hwmon/hwmon0/pwm3_auto_point1_pwm

    # PWM 3 max 100%
    echo 255 > /sys/class/hwmon/hwmon0/pwm3_auto_point2_pwm

    # Fan will spin at min duty cycle even Tsens is below Tmin
    echo 1   > /sys/class/hwmon/hwmon0/pwm3_stall_disable

    # PWM 3 controlled by all three temperature channel
    echo 7   > /sys/class/hwmon/hwmon0/pwm3_auto_channels_temp

    # Set all Tmin 30 and Tmax 40
    echo 30000 > /sys/class/hwmon/hwmon0/temp1_auto_point1_temp
    echo 30000 > /sys/class/hwmon/hwmon0/temp2_auto_point1_temp
    echo 30000 > /sys/class/hwmon/hwmon0/temp3_auto_point1_temp
    echo 40000 > /sys/class/hwmon/hwmon0/temp1_auto_point2_temp
    echo 40000 > /sys/class/hwmon/hwmon0/temp2_auto_point2_temp
    echo 40000 > /sys/class/hwmon/hwmon0/temp3_auto_point2_temp
}


turn_on_eth_phys()
{
	reset_5_reg=`sudo i2cget -y 0 0x33 0x34`
	reset_5_reg=$((reset_5_reg | 0x04))
	sudo i2cset -y 0 0x33 0x34 $reset_5_reg
	sleep 5
	sudo i2cset -y 0 0x33 0x32 0x3f
}

turn_off_wdog()
{
	wd_reg=`sudo i2cget -y 0 0x33 0x40`
	wd_reg=$((wd_reg & ~0x01))
	sudo i2cset -y 0 0x33 0x40 $wd_reg
}

init_sfp()
{
    for port in {1..4}; do
        echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$port/new_device
    done
}

# Main
#turn_off_wdog TODO: need to check
turn_on_eth_phys
canisminor_profile

init_led
init_fans
init_sfp

# Enumerate system eeprom
echo 24c64 0x52 > /sys/bus/i2c/devices/i2c-0/new_device
sleep 2


exit 0

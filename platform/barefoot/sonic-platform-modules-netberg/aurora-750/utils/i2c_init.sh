#!/bin/bash


# vdd value for mac
rov_val_array=( 0.85 0.82 0.77 0.87 0.74 0.84 0.79 0.89 )
rov_reg_array=( 0x24 0x73 0x69 0x7D 0x63 0x77 0x6D 0x81 )

#GPIO Offset
GPIO_OFFSET=768

# fp port to phy port mapping
fp2phy_array=( 0  1  4  5  8  9 12 13 16 17 20 21 24 25 28 29
               32 33 36 37 40 41 44 45 48 49 52 53 56 57 60 61
               2  3  6  7 10 11 14 15 18 19 22 23 26 27 30 31
               34 35 38 39 42 43 46 47 50 51 54 55 58 59 62 63)

#IO Expander Init
function _i2c_io_exp_init {
    # _i2c_hwm_init  
    i2cset -y -r 16 0x2f 0x00 0x80
    i2cset -y -r 16 0x2f 0x01 0x9C
    i2cset -y -r 16 0x2f 0x04 0x00
    i2cset -y -r 16 0x2f 0x06 0xFF
    i2cset -y -r 16 0x2f 0x07 0x00
    i2cset -y -r 16 0x2f 0x01 0x1C
    i2cset -y -r 16 0x2f 0x00 0x82
    i2cset -y -r 16 0x2f 0x0F 0x00
    i2cset -y -r 16 0x2f 0x18 0x84
    i2cset -y -r 16 0x2f 0x19 0x84

    # _i2c_io_exp_init 
    # need to init BMC io expander first due to some io expander are reset default
    # Init BMC INT & HW ID IO Expander
    i2cset -y -r 0 0x24 6 0xFF
    i2cset -y -r 0 0x24 7 0xFF
    i2cset -y -r 0 0x24 4 0x00
    i2cset -y -r 0 0x24 5 0x00

    # Init BMC PSU status IO Expander
    i2cset -y -r 0 0x25 6 0xFF
    i2cset -y -r 0 0x25 7 0xFF
    i2cset -y -r 0 0x25 4 0x00
    i2cset -y -r 0 0x25 5 0x00
 
    # Init BMC RST and SEL IO Expander
    i2cset -y -r 0 0x26 2 0x3F
    i2cset -y -r 0 0x26 3 0x1F
    i2cset -y -r 0 0x26 6 0xD0
    i2cset -y -r 0 0x26 7 0x00
    i2cset -y -r 0 0x26 4 0x00
    i2cset -y -r 0 0x26 5 0x00

    # Init System LED & HW ID IO Expander
    i2cset -y -r 10 0x76 2 0x00
    i2cset -y -r 10 0x76 6 0x00
    i2cset -y -r 10 0x76 7 0xFF
    i2cset -y -r 10 0x76 4 0x00
    i2cset -y -r 10 0x76 5 0x00

    # Init FAN Board Status IO Expander
    i2cset -y -r 0 0x20 2 0x11
    i2cset -y -r 0 0x20 3 0x11
    i2cset -y -r 0 0x20 6 0xCC
    i2cset -y -r 0 0x20 7 0xCC
    i2cset -y -r 0 0x20 4 0x00
    i2cset -y -r 0 0x20 5 0x00

    # Init System SEL and RST IO Expander
    i2cset -y -r 32 0x76 2 0x04
    i2cset -y -r 32 0x76 3 0xDF
    i2cset -y -r 32 0x76 6 0x09
    i2cset -y -r 32 0x76 7 0x3F
    i2cset -y -r 32 0x76 4 0x00
    i2cset -y -r 32 0x76 5 0x00
}

function mac_vdd_init {
    # read mac vid register value from CPLD
    val=`cat /sys/bus/i2c/devices/1-0033/cpld_rov_status`

    # get vid form register value [0:2]
    vid=$(($val & 0x7))

    # get rov val and reg according to vid
    rov_val=${rov_val_array[$vid]}
    rov_reg=${rov_reg_array[$vid]}
    echo "vid=${vid}, rov_val=${rov_val}, rov_reg=${rov_reg}"

    # write the rov reg to rov
    i2cset -y -r 15 0x76 0x21 ${rov_reg} w

    if [ $? -eq 0 ]; then
        echo "set ROV for mac vdd done"
    else
        echo "set ROV for mac vdd fail"
    fi
}

rmmod i2c_i801
modprobe i2c_i801
modprobe i2c_dev
modprobe i2c_mux_pca954x
echo 'pca9548 0x70' > /sys/bus/i2c/devices/i2c-0/new_device
echo 'pca9548 0x73' > /sys/bus/i2c/devices/i2c-0/new_device
echo 'pca9546 0x72' > /sys/bus/i2c/devices/i2c-0/new_device
sleep 1
#todo: switch to channels 
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-19/new_device
echo 'pca9546 0x71' > /sys/bus/i2c/devices/i2c-20/new_device

echo 'pca9548 0x75' > /sys/bus/i2c/devices/i2c-0/new_device
sleep 1
for I in {21..28} 
do
    	echo 'pca9548 0x74' > /sys/bus/i2c/devices/i2c-$I/new_device
done

echo '-2' > /sys/bus/i2c/devices/0-0070/idle_state
echo '-2' > /sys/bus/i2c/devices/0-0073/idle_state
echo '-2' > /sys/bus/i2c/devices/0-0072/idle_state
echo '-2' > /sys/bus/i2c/devices/19-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/20-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/0-0075/idle_state

for I in {21..28}
do
        echo '-2' > /sys/bus/i2c/devices/$I-0074/idle_state
done

_i2c_io_exp_init 

# i2cget -y 44 0x74 2

echo 'w83795adg 0x2F' > /sys/bus/i2c/devices/i2c-16/new_device

#TMP75 Init
echo "lm75 0x4D" > /sys/bus/i2c/devices/i2c-6/new_device  # lm75_1 Rear Panel, hwmon2
echo "lm75 0x4E" > /sys/bus/i2c/devices/i2c-6/new_device  # lm75_2 Rear MAC, hwmon3
echo "lm86 0x4C" > /sys/bus/i2c/devices/i2c-6/new_device  # lm86 , hwmon4
echo "lm75 0x4D" > /sys/bus/i2c/devices/i2c-7/new_device  # lm75_3 Front Panel, hwmon5
echo "lm75 0x4E" > /sys/bus/i2c/devices/i2c-7/new_device  # lm75_4 Front MAC, hwmon6
echo "lm75 0x4A" > /sys/bus/i2c/devices/i2c-16/new_device # tmp75 BMC board thermal, hwmon7
echo "lm75 0x4F" > /sys/bus/i2c/devices/i2c-0/new_device  # tmp75 CPU board thermal, hwmon8


modprobe netberg_nba750_64x_i2c_cpld

for I in {1..5}
do
        echo "netberg_cpld$I 0x33"> /sys/bus/i2c/devices/i2c-$I/new_device
done

modprobe at24
#Init MB EEPROM
echo "24c32 0x55" > /sys/bus/i2c/devices/i2c-0/new_device

# modprobe eeprom
# PS EEPROM
echo "spd 0x50" > /sys/bus/i2c/devices/i2c-18/new_device #PS0
echo "spd 0x50" > /sys/bus/i2c/devices/i2c-17/new_device #PS1

#modprobe pmbus

# PS PMBUS
echo "pmbus 0x58" > /sys/bus/i2c/devices/i2c-18/new_device #PS0
echo "pmbus 0x58" > /sys/bus/i2c/devices/i2c-17/new_device #PS1

modprobe optoe

#QSFP EEPROM
for I in {1..64}
do
    declare -i phy_port=${fp2phy_array[$I-1]}+1
    declare -i port_group=$phy_port/8
    declare -i eeprom_busbase=41+$port_group*8
    declare -i eeprom_busshift=$phy_port%8
    declare -i eeprom_bus=$eeprom_busbase+$eeprom_busshift-1
    echo "optoe1 0x50" > /sys/bus/i2c/devices/i2c-$eeprom_bus/new_device
done

# init SFP0/1 EEPROM
echo "sff8436 0x50" > /sys/bus/i2c/devices/i2c-29/new_device
echo "sff8436 0x50" > /sys/bus/i2c/devices/i2c-30/new_device

sleep 1
mac_vdd_init

# Set  fan init speed
echo 120 > /sys/bus/i2c/devices/i2c-16/16-002f/pwm1

# _util_port_led_clear
i2cset -m 0x04 -y -r 32 0x76 2 0x0
sleep 1
i2cset -m 0x04 -y -r 32 0x76 2 0xFF

# turn on sys led
i2cset -m 0x80 -y -r 10 0x76 2 0x80

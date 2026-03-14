#!/bin/bash

SEARCH_I2C_BUS=$(ls /sys/bus/i2c/devices)
I2C_BUS=-1

for i in $SEARCH_I2C_BUS
do
	if [[ -n $(cat /sys/bus/i2c/devices/$i/name | grep i2c-mcp2221) ]]; then
		I2C_BUS=$(echo $i | sed 's/i2c-//g')
		break
	fi
done

if [[ $I2C_BUS == -1 ]]; then
	echo "Can't find i2c-mcp2221"
	exit
fi

TOTAL_MUX=8
START_NODE_NUM=$((I2C_BUS+1))

modprobe i2c_mux_pca954x force_deselect_on_exit=1

echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device
echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-$I2C_BUS/new_device

sleep 5

/sbin/modprobe wnc_cpld
/sbin/modprobe wnc_cpld3
/sbin/modprobe wnc_eeprom
/sbin/modprobe eeprom

# MUX0: i2c-3~i2c-10
# MUX1: i2c-11~i2c-18
# MUX2: i2c-19~i2c-26
# MUX3: i2c-27~i2c-34
# MUX4: i2c-35~i2c-42
# MUX5: i2c-43~i2c-50
# MUX6: i2c-51~i2c-58
# MUX7: i2c-59~i2c-66

# MUX0 channel0
CHANNEL=0
echo wnc_cpld 0x31 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel1
CHANNEL=1
echo wnc_cpld 0x32 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel2
CHANNEL=2
echo wnc_cpld3 0x33 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device
echo wnc_eeprom 0x53 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel3
CHANNEL=3
echo wnc_eeprom 0x50 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device
echo wnc_eeprom 0x51 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel4
CHANNEL=4
echo wnc_eeprom 0x54 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device
echo tmp421 0x1E > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device
sleep 1
echo tmp75 0x4E > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device
sleep 1
echo tmp421 0x4F > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel5
CHANNEL=5
echo wnc_eeprom 0x52 > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

# MUX0 channel7
CHANNEL=7
#echo wnc_eeprom 0x5B > /sys/bus/i2c/devices/i2c-$(($START_NODE_NUM+$CHANNEL))/new_device

START_PORT_NUM=$((START_NODE_NUM+8))
END_PORT_NUM=$((TOTAL_MUX*8+1))

for i in $(seq $START_PORT_NUM $END_PORT_NUM)
do
	echo wnc_eeprom 0x50 > /sys/bus/i2c/devices/i2c-$i/new_device
	echo wnc_eeprom 0x51 > /sys/bus/i2c/devices/i2c-$i/new_device
done

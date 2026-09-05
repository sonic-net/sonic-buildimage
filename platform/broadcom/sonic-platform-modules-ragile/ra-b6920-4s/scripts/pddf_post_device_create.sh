#!/bin/bash

# create tmp411 device
function create_tmp411()
{
	bus=$1
	addr=$2
	if [ -d "/sys/bus/i2c/devices/i2c-${bus}" ]
	then
		echo "tmp411 ${addr}" > /sys/bus/i2c/devices/i2c-${bus}/new_device
	fi
}

create_tmp411 "28" "0x4c"
create_tmp411 "29" "0x4c"


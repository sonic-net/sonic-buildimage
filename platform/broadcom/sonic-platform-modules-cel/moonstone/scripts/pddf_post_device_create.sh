#!/bin/bash

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE=`cat /sys/devices/platform/sys_cpld/bmc_present_l`

# bmc present
if [ ${BMC_PRESENCE} == "1" ]; then
	# attach ucdxxxx devices, bus 100, address 0x33 and 0x35
	echo ucd90320 0x33 > /sys/bus/i2c/devices/i2c-100/new_device
	echo ucd90160 0x35 > /sys/bus/i2c/devices/i2c-100/new_device

	# attach isl68137 devices, bus 103
	echo raa228228 0x20 > /sys/bus/i2c/devices/i2c-103/new_device
	echo isl68225 0x60  > /sys/bus/i2c/devices/i2c-103/new_device
	echo isl68225 0x61  > /sys/bus/i2c/devices/i2c-103/new_device
	echo isl68222 0x62  > /sys/bus/i2c/devices/i2c-103/new_device
	echo isl68222 0x63  > /sys/bus/i2c/devices/i2c-103/new_device
	echo isl68225 0x67  > /sys/bus/i2c/devices/i2c-103/new_device
fi

modprobe i2c-imc
sleep 2

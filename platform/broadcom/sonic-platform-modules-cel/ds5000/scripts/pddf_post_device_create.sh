#!/bin/bash

modprobe pddf_custom_fpga_extend

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE=`cat /sys/devices/platform/sys_cpld/bmc_present_l`
HW_SKU=`awk '{print $1}' /usr/share/sonic/device/$PLATFORM/default_sku`

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

echo 0x0010 > /sys/devices/platform/cls_sw_fpga/FPGA/getreg
let boardRev=`cat /sys/devices/platform/cls_sw_fpga/FPGA/getreg`
lanemap=/usr/share/sonic/device/$PLATFORM/$HW_SKU/LANE_MAPPING-$boardRev.yml

for file in `ls /usr/share/sonic/device/$PLATFORM/$HW_SKU/*.config.yaml`
do
	grep PC_PM_CORE $file > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		eval $(grep -n PC_PM_CORE $file | awk 'BEGIN{FS=":"} {printf("start=%d",$1)}')
		let start=start-3; let end=`awk 'END {print NR}' $file`
		sed -i "${start},${end}d" $file
	fi
	cat $lanemap >> $file
	cat /usr/share/sonic/device/$PLATFORM/$HW_SKU/common.yaml >> $file
done

sync

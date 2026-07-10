#!/bin/bash

vid=04d8
pid=00dd

#check to see if sysfs is mounted
sysfs_path=`awk '/^sysfs/{ print $2 }' < /proc/mounts`
curr_path=`pwd`

#if variable is empty, we should exit. No SYSFS found
if [[ -z $sysfs_path ]]; then
	echo "No sysfs in this system! Exiting..."
	exit 1
fi

function load_drivers
{
	modprobe i2c-dev
	if [[ $? -ne 0 ]]; then
		echo "Cannot load the \"i2c-dev\" driver! Exiting..."
		exit 1
	fi
	modprobe i2c-mcp2221
	if [[ $? -ne 0 ]]; then
		echo "Cannot load the \"i2c-mcp2221\" driver! Exiting..."
		exit 1
	fi
	echo "I2C related drivers are loaded"
}

usb_device_path=${sysfs_path}/bus/usb/devices

cd $usb_device_path

for usbdev in *; do
	idvendor=${usb_device_path}/${usbdev}/idVendor
	idproduct=${usb_device_path}/${usbdev}/idProduct
	usb_driver=${usb_device_path}/${usbdev}/${usbdev}:1.2/driver
	if [[ -f $idvendor ]]; then
		dev_vid=`grep -i $vid < $idvendor`
		dev_pid=`grep -i $pid < $idproduct`
		if [[ -n $dev_vid ]] && [[ -n $dev_pid ]]; then
			echo "I found the requested VID/PID:  $dev_vid, $dev_pid"
			load_drivers
			echo -n "${usbdev}:1.2" > ${usb_driver}/unbind
			echo -n "${usbdev}:1.2" > ${sysfs_path}/bus/usb/drivers/i2c-mcp2221/bind
		fi
	fi
done


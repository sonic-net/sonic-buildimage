#!/bin/bash

SW_READY_STAMP='/tmp/.BcmSdkReady'
SYSTEM_INIT=0
CURRENTR_LED=0

# 0 was not ready, 1 was ready
SYSTEM_READY=1

function check_sdk_ready() {
	if [ `file /var/run/docker-syncd/sswsyncd.socket | grep -c " socket"` -ne 1 ]; then
		SYSTEM_READY=0
		return 1
	else
		if [ `ss -a | grep "/var/run/sswsyncd/sswsyncd.socket" |grep -c "LISTEN"` -ne 1 ]; then
			SYSTEM_READY=0
			return 1
		fi
	fi
	return 0
}

while [ true ]
do
	SYSTEM_READY=1
	#-----check start------------
	check_sdk_ready
	#-----cech end---------------

	if [ "$SYSTEM_INIT" -eq "0" ]; then
		if [ $SYSTEM_READY -eq 1 ]; then
			SYSTEM_INIT=1
			# set SYSTEM LED to Green
			echo 0x01 > /sys/bus/i2c/devices/1-0032/system_led_fld
			CURRENTR_LED=1
		fi
	else
		if [ $SYSTEM_READY -eq 0 ]; then
			# set SYSTEM LED to Amber
			if [ "$CURRENTR_LED" -ne "2" ]; then
				echo 0x02 > /sys/bus/i2c/devices/1-0032/system_led_fld
				CURRENTR_LED=2
			fi
		else
			# set SYS LED to Green
			if [ "$CURRENTR_LED" -ne "1" ]; then
				echo 0x01 > /sys/bus/i2c/devices/1-0032/system_led_fld
				CURRENTR_LED=1
			fi
		fi
	fi
	sleep 10
done

exit 0

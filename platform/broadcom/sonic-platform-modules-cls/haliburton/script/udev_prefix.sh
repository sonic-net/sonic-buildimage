#!/bin/bash

PREV_REBOOT_CAUSE="/host/reboot-cause/"
DEVICE="/usr/share/sonic/device"
PLATFORM=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
PLATFORM_PATH=$DEVICE/$PLATFORM
FILENAME="udevprefix.conf"

if [ "$1" = "clear" ]
then
	if [ -e $PLATFORM_PATH/$FILENAME ]; then
		rm $PLATFORM_PATH/$FILENAME
	fi
else
	if [ -e $PLATFORM_PATH/$FILENAME ]; then
		: > $PLATFORM_PATH/$FILENAME
		echo -n "$1" > $PLATFORM_PATH/$FILENAME
	else
		touch $PLATFORM_PATH/$FILENMAE
		echo -n "$1" > $PLATFORM_PATH/$FILENAME
	fi
fi


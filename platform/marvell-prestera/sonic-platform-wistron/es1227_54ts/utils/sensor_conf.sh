#!/bin/bash

direction=$(decode-syseeprom -p)
dev_path=/usr/share/sonic/device/arm64-wistron_es1227_54ts-r0/

if [ $direction == "ES-1227-54TS-O" ]; then
	cp $dev_path/sensors_afo.conf $dev_path/sensors.conf
	cp $dev_path/platform_afo.json $dev_path/platform.json
fi
if [ $direction == "ES-1227-54TS-I" ]; then
	cp $dev_path/sensors_afi.conf $dev_path/sensors.conf
	cp $dev_path/platform_afi.json $dev_path/platform.json
fi

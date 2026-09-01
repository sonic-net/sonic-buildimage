#!/usr/bin/env bash

EXIT_PTP_CONFIG_FILE_NOT_FOUND=1
PTP_CONFIG_FILE=/etc/ptp4l/ptp4l.conf

echo "Starting ptp.sh script"
echo "Checking for config file: $PTP_CONFIG_FILE"

if [ ! -f "$PTP_CONFIG_FILE" ]; then
    echo "ERROR: PTP4L config file not found at $PTP_CONFIG_FILE"
    exec sleep infinity
else
    echo "Using PTP4L config file found at: $PTP_CONFIG_FILE"
    exec ptp4l -f $PTP_CONFIG_FILE -m
fi



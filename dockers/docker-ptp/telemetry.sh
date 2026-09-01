#!/usr/bin/env bash

PTP_CONFIG_FILE=/etc/ptp4l/ptp4l.conf
echo "Starting telemetry.sh script"

if [ ! -f "$PTP_CONFIG_FILE" ]; then
    echo "ERROR: PTP4L config file not found at $PTP_CONFIG_FILE"
    exec sleep infinity
else
    echo "Using PTP4L config file found at: $PTP_CONFIG_FILE"
    ptp_telemetry.py
    #echo "Not implemented yet"
    #exec sleep infinity
fi

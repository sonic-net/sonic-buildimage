#!/bin/bash

FPGA_SDK_READY_REG=0x84
FPGA_SDK_READY_BIT=$1
FPGA_VER_SYSFS_FILE=/sys/devices/platform/cls_sw_fpga/FPGA/version

# Function: wait until syncd has created the socket for bcmcmd to connect to
wait_syncd() {
    while true; do
        service syncd status | grep 'active (running)' >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            break
        fi
        sleep 1
    done

    # wait until bcm sdk is ready to get a request
    counter=0
    while true; do
        /usr/bin/bcmcmd -t 1 "show unit" | grep BCM >/dev/null 2>&1
        if [ $? -eq 0 ]; then
            sleep 10
            /usr/bin/bcmcmd -t 1 "show unit" | grep BCM >/dev/null 2>&1
            if [ $? -eq 0 ]; then
                break
            fi
        fi
        sleep 1
    done
}

if [ $FPGA_SDK_READY_BIT = '1' ]; then
	wait_syncd
fi

# wait until fpga driver is ready
while true; do
    if [ -e ${FPGA_VER_SYSFS_FILE} ]; then
        break
    fi
    sleep 1
done

let fpga_ver=`cat $FPGA_VER_SYSFS_FILE`
if [ $fpga_ver -ge 12 ]; then
    if [ $FPGA_SDK_READY_BIT = '1' ]; then
        echo "Notify the other chips that the SDK software is ready"
    else
        echo "Notify the other chips that the SDK software is not ready"
    fi
    echo $FPGA_SDK_READY_REG $FPGA_SDK_READY_BIT > /sys/devices/platform/cls_sw_fpga/FPGA/setreg
fi

exit 0
#!/usr/bin/env bash

FPGA_SDK_READY_REG=0x84
FPGA_SDK_READY_BIT=0x1
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
            counter=$((counter+10))
            /usr/bin/bcmcmd -t 1 "show unit" | grep BCM >/dev/null 2>&1
            if [ $? -eq 0 ]; then
                break
            fi
        fi
        counter=$((counter+1))
        if [ $counter -ge 360 ]; then
            echo "syncd is not ready to take commands after $counter re-tries; Exiting!"
            exit 1
        fi
        sleep 1
    done
}

wait_syncd

# wait until fpga driver is ready
while true; do
    if [ -e ${FPGA_VER_SYSFS_FILE} ]; then
        break
    fi
    sleep 1
done

let fpga_ver=`cat $FPGA_VER_SYSFS_FILE`
if [ $fpga_ver -ge 12 ]; then
    echo "Notify the other chips that the SDK software is ready"
    echo $FPGA_SDK_READY_REG $FPGA_SDK_READY_BIT > /sys/devices/platform/cls_sw_fpga/FPGA/setreg
fi

exit 0
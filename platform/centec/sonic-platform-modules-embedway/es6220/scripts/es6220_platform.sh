#!/bin/bash

#platform init script for embedway es6220

init_devnum() {
    found=0
    for devnum in 0 1; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # I801 adapter can be at either dffd0000 or dfff0000
        if [[ $devname == 'SMBus I801 adapter at '* ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "cannot find I801" && exit 1
}

init_devnum

if [ "$1" == "init" ]; then
    #install drivers and dependencies
    depmod -a
    modprobe dal

elif [ "$1" == "deinit" ]; then
    modprobe -r dal
else
     echo "e582-48x2q4z_platform : Invalid option !"
fi

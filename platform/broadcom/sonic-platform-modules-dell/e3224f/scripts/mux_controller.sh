#!/bin/bash

init_devnum() {
    found=0
    for devnum in 0 1; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # iSMT adapter can be at dff5c000
        echo $devname
        if [[ "$devname" == 'SMBus iSMT adapter at '* ]] ; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "cannot find iSMT" && exit 0
}

init_devnum
while [ 1 ]
do
    if [ ! -f /sys/class/i2c-adapter/i2c-${devnum}/${devnum}-0071/idle_state ]; then
        sleep 1
        continue
    fi
    echo -2 > /sys/class/i2c-adapter/i2c-${devnum}/${devnum}-0071/idle_state
    break
done

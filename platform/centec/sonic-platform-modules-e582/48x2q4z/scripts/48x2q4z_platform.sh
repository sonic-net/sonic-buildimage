#!/bin/bash

#platform init script for centec e582-48x2q4z

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
    modprobe i2c-i801
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe ctc-i2c-mux-pca954x force_deselect_on_exit=1
        i2cset -y 0 0x58 0x8 0x3f
    modprobe lm77
    modprobe tun
    modprobe dal
    modprobe at24
    echo 24c64 0x57 > /sys/bus/i2c/devices/i2c-0/new_device
    modprobe centec_e582_48x2q4z_platform
        i2cset -y 15 0x21 0x18 0x0
        i2cset -y 15 0x21 0x19 0x0
        i2cset -y 15 0x21 0x1a 0xff
        i2cset -y 15 0x21 0x1b 0xff
        i2cset -y 15 0x21 0x1c 0xff
        i2cset -y 15 0x21 0x8 0x0
        i2cset -y 15 0x21 0x9 0x0
        i2cset -y 15 0x22 0x18 0xff
        i2cset -y 15 0x22 0x19 0x0
        i2cset -y 15 0x22 0x1a 0x0
        i2cset -y 15 0x22 0x1b 0xff
        i2cset -y 15 0x22 0x1c 0xff
        i2cset -y 15 0x22 0x9 0x0
        i2cset -y 15 0x22 0xa 0x0
        i2cset -y 16 0x21 0x18 0x0
        i2cset -y 16 0x21 0x19 0x0
        i2cset -y 16 0x21 0x1a 0xff
        i2cset -y 16 0x21 0x1b 0xff
        i2cset -y 16 0x21 0x1c 0xff
        i2cset -y 16 0x21 0x8 0x0
        i2cset -y 16 0x21 0x9 0x0
        i2cset -y 17 0x22 0x18 0xff
        i2cset -y 17 0x22 0x19 0x0
        i2cset -y 17 0x22 0x1a 0xff
        i2cset -y 17 0x22 0x1b 0x0
        i2cset -y 17 0x22 0x1c 0xff
        i2cset -y 17 0x22 0x9 0xf0
        i2cset -y 17 0x22 0xb 0x0c

    #start platform monitor
    rm -rf /usr/bin/platform_monitor
    ln -s /usr/bin/48x2q4z_platform_monitor.py /usr/bin/platform_monitor
    python /usr/bin/platform_monitor &
elif [ "$1" == "deinit" ]; then
    kill -9 $(pidof platform_monitor) > /dev/null 2>&1
    rm -rf /usr/bin/platform_monitor
    modprobe -r centec_e582_48x2q4z_platform
    modprobe -r at24
    modprobe -r dal
    modprobe -r ctc-i2c-mux-pca954x
    modprobe -r i2c-dev
else
     echo "e582-48x2q4z_platform : Invalid option !"
fi

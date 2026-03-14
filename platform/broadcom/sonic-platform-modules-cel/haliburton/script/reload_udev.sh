#!/bin/bash

UDEV_DIR=/etc/udev/rules.d/50-ttyUSB-C0.rules
TTYUSB_DIR=/dev/ttyUSB

if [ -f "$UDEV_DIR" ]; then
        for i in {0..95}
        do
                ttydev=$TTYUSB_DIR$i
                if [ -c "$ttydev" ]; then
                        udevadm trigger -c add $ttydev
                fi
        done
fi

#!/bin/bash

#Due to the hardware design, this platform uses "eth2" instead of "eth0" as management interface.
#Rename eth2 to eth0 using udev

/etc/init.d/netfilter-persistent stop
modprobe -r ixgbe
udevadm control --reload-rules
udevadm trigger
modprobe ixgbe
/etc/init.d/netfilter-persistent start

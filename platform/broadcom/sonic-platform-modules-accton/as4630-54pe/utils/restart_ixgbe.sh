#!/bin/bash

/etc/init.d/netfilter-persistent stop
modprobe -r ixgbe
udevadm control --reload-rules
udevadm trigger
modprobe ixgbe
/etc/init.d/netfilter-persistent start


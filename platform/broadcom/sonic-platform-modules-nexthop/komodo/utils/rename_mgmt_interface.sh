#!/bin/bash

XGBE0=$(setpci -s 00:08.2 0x19.b | xargs printf '0000:%s:00.2')
XGBE1=$(setpci -s 00:08.2 0x19.b | xargs printf '0000:%s:00.3')
XGBE_PCI_VENDOR_DEVICE="1022 1458"

echo "$XGBE0" > /sys/bus/pci/drivers/amd-xgbe/unbind 2> /dev/null
echo "$XGBE_PCI_VENDOR_DEVICE" > /sys/bus/pci/drivers/pci-stub/new_id 2> /dev/null
echo "$XGBE0" > /sys/bus/pci/drivers/pci-stub/bind 2> /dev/null

if [[ $(readlink /sys/class/net/eth0/device) =~ $XGBE1 ]]; then
  echo "eth0 correctly assigned, skipping rename"
  exit 0
fi

ip link set dev eth1 down

if ! ip link show eth0 &>/dev/null; then
  ip link set dev eth1 name eth0
else
  ip link set dev eth0 down
  ip link set dev eth0 name eth1-temp
  ip link set dev eth1 name eth0
fi

ip link set dev eth0 up

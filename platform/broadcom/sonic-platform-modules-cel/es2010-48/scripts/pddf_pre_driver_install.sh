#!/bin/bash

# Has customized those drivers,so rename them to lose effect
psu_driver=pddf_psu_driver_module.ko
ker_name=$(uname -r)
driver_path=/usr/lib/modules/${ker_name}/extra/
if [ -e ${driver_path}${psu_driver} ]; then
     mv ${driver_path}${psu_driver} ${driver_path}${psu_driver}-bk
fi

# Re-install the igb and ixgbe again to make the NIC sequence follow the udev rule
modprobe -r igb
modprobe -r ixgbe

# Sleep for the PHY state machine to stop.
sleep 3

# Check for the PCIe BDF to attach PHY
pcie_bdf=$(lspci -D | grep "Ethernet controller.*1GbE" | cut -f1 -d" " | grep "\.0$" | head -n 1)
phy_addr=0
if [ x"${pcie_bdf}" != "x" ]; then
    drv_args="extphymap=\"${pcie_bdf}@${phy_addr}\""
fi

modprobe ixgbe ${drv_args}
modprobe igb

#!/bin/bash
# install custom fpga device.

sleep 3
modprobe pddf_custom_fpga_extend

platform=`cat /host/machine.conf | sed -n 's/onie_platform=\(.*\)/\1/p'`
bmc_present=`cat /usr/share/sonic/device/$platform/bmc_status`

#Set off Alarm LED
if [[ "$bmc_present" != "True" ]]; then
    i2cset -f -y 100 0x0d 0x63 0x33 &> /dev/null
fi

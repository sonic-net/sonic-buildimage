#!/bin/bash
# install custom fpga device.

sleep 3
modprobe pddf_custom_fpga_extend

bmc_present=`cat /usr/share/sonic/device/x86_64-cel_silverstone_v2-r0/bmc_status`
#Set off Alarm LED
if [[ "$bmc_present" == "True" ]]; then
    # Set all LEDs to Manual control
    ipmitool raw 0x3a 0x42 0x02 0x00 &> /dev/null

    # Set Alarm LED to OFF
    ipmitool raw 0x3a 0x39 0x02 0x01 0x00 &> /dev/null

    #Set CPLD Fan LED register to BMC control
    ipmitool raw 0x3a 0x64 0x00 0x02 0x65 0x10 &> /dev/null
else
    i2cset -f -y 100 0x0d 0x63 0x33 &> /dev/null
fi

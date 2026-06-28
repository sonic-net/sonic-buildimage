#!/bin/bash
# Set SYS_LED to Green, assuming everything came up fine.
#ipmitool raw 0x3A 0x0C 0x00 0x03 0x62 0xdc

# Enable thermal shutdown by default
#i2cset -y -f 103 0x0d 0x75 0x1

# Load fpga extend driver after fpga device created
modprobe pddf_custom_fpga_extend

SETREG_PATH="/sys/devices/platform/sys_cpld/setreg"
GETREG_PATH="/sys/devices/platform/sys_cpld/getreg"
BMC_PRESENCE=`echo '0xA108' > $GETREG_PATH && cat $GETREG_PATH`
#Set off Alarm LED
if [ ${BMC_PRESENCE} == "0x00" ]; then
    # Set all LEDs to Manual control
    ipmitool raw 0x3a 0x42 0x02 0x00 &> /dev/null

    # Set Alarm LED to OFF
    ipmitool raw 0x3a 0x39 0x02 0x01 0x00 &> /dev/null
else
    i2cset -f -y 100 0x0d 0x63 0x33 &> /dev/null
fi

# enable FPGA control port status
echo '0xA130 0x01' > ${SETREG_PATH}

#!/bin/bash

# Script to unfreeze a stuck I2C controller, by bit-banging a STOP cycle on the bus

bit_bang_recovery()
{

# Clear the ERRSTS
pcisysfs.py --set --val 0xffffffff --offset 0x018 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

#Enable I2C bit-banging
pcisysfs.py --set --val 0x80000000 --offset 0x388 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

count=1
while [ $count -le 9 ];
do
    # Bit-bang an I2C STOP cycle

    # SCL=0, SDA=0
    pcisysfs.py --set --val 0x80000000 --offset 0x388 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

    sleep 0.01

    # SCL=1, SDA=0
    pcisysfs.py --set --val 0x80000002 --offset 0x388 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

    sleep 0.01

    # SCL=1, SDA=1
    pcisysfs.py --set --val 0x80000003 --offset 0x388 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

    sleep 1

    # Check I2C DBSTS register
    mctrl=$((`pcisysfs.py --get --offset 0x108 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
    msts=$((`pcisysfs.py --get --offset 0x10c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
    dbsts=$((`pcisysfs.py --get --offset 0x38c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
    msts_ip=$((msts&0x1))
    
    mctrl=$((10#$mctrl))
    if [ $msts_ip = 0 ]; then
	logger -p NOTICE "I2C_bitbang-Bit banging done on I2C bus"
	logger -p NOTICE "After I2C_bitbang- MCTRL:$(printf "0x%x" $mctrl)","MSTS:$(printf "0x%x" $msts)","DBSTS:$(printf "0x%x" $dbsts)"
	break
    fi
    count=$(( $count + 1 ))
done

#Disable I2C bit-banging
pcisysfs.py --set --val 0x00000003 --offset 0x388 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0

}

mctrl=$((`pcisysfs.py --get --offset 0x108 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
msts=$((`pcisysfs.py --get --offset 0x10c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
dbsts=$((`pcisysfs.py --get --offset 0x38c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 | sed 's/^.*:/0x/'`))
logger -p NOTICE "Before I2C_bitbang- MCTRL:$(printf "0x%x" $mctrl)","MSTS:$(printf "0x%x" $msts)","DBSTS:$(printf "0x%x" $dbsts)"
sleep 2
bit_bang_recovery

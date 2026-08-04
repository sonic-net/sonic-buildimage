#!/bin/bash

#Usage:
# check_qsfp.sh qsfp10
# check_qsfp.sh sffp2

# There are 34 optics ports. 32 QSFP28, 2 SFP+
# QSFP28 is orgainized is 3 banks
# CPLD-2	QSFP-1-12
# CPLD-3	QSFP-13-22
# CPLD-4	QSFP-23-32
# SFF+ 2 ports on CPLD-4

optics=$1
cpld_addr="003e"

if [ `expr match $optics qsfp` -ne 0 ]
then
    type=qsfp
    id=${optics:4}

    if [ $id -le 0 -o $id -gt 32 ] 
    then
        echo "Invalid QSFP id" 
        return -1
    fi
    if [ $id -le 12 ]
    then
        #cpld=2
        cpld_bus=15
    elif [ $id -le 22 ]
    then
        #cpld=3
        cpld_bus=16
    else
        #cpld=4
        cpld_bus=17
    fi
    
    if [ $id -le 8 ]
    then
        let qsfp_bus=42+$id 
    elif [ $id -le 16 ]
    then
        let qsfp_bus=34+${id}-8
    elif [ $id -le 24 ]
    then
        let qsfp_bus=26+${id}-16
    else
        let qsfp_bus=18+${id}-24
    fi
else
    type=sff
    id=${optics:3}
    cpld=4
fi

echo "Preence:`cat /sys/class/i2c-dev/i2c-${cpld_bus}/device/${cpld_bus}-${cpld_addr}/qsfp_modprs`"
echo "Hexdump:"
cat /sys/bus/i2c/devices/${qsfp_bus}-0050/eeprom | hexdump -C

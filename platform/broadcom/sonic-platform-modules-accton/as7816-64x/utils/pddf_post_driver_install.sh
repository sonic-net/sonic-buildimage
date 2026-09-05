#!/bin/bash

dis_i2c_ir3570a()
{
    local addr=$1

    i2cset -y -a 0 $addr 0xE5 0x01 &>/dev/null
    i2cset -y -a 0 $addr 0x12 0x02 &>/dev/null
}


ir3570_check()
{
    dump=`i2cdump -y 0 0x42 s 0x9a |awk 'END {print $2}'`
    if [ $dump -eq 24 ]; then
        echo "Disabling i2c function of ir3570a"
        dis_i2c_ir3570a 0x4
    fi
}

ir3570_check

echo "AS7816 post PDDF driver install completed"

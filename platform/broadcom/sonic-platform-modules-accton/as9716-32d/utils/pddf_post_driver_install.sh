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

# Some initializations
#============================================================
# cpld_reset_stop
#i2cset -y 0 0x65 0x3 0x0

# Select the mux channels
i2cset -y 0 0x77 0x1
i2cset -y 0 0x76 0x4

# Diag LED: Boot successfully (SOLID GREEN) --- DO THIS STEP IN POST DEVICE CREATION
# i2cset -y 0 0x60 0x64 4

# 10G Merlin Ports
# SetModeXFI()
echo "Set Mux(retimer) to 2x10G XFI."
i2cset -y 0 0x76 0x20
i2cset -y 0 0x18 0x7 0x3
i2cset -y 0 0x19 0x7 0x3
i2cset -y 0 0x1a 0x7 0x3
i2cset -y 0 0x1b 0x7 0x3


# SetVOD()
#set channel B
i2cset -y 0 0x18 0xff 0x05
i2cset -y 0 0x19 0xff 0x05
#write output voltage to 800mV
i2cset -y 0 0x18 0x2d 0x82
i2cset -y 0 0x19 0x2d 0x82
#write de-emphasis to -3.5dB
i2cset -y 0 0x18 0x15 0x12
i2cset -y 0 0x19 0x15 0x12
#read output voltage
#i2cget -y 0 0x18 0x2d
#i2cget -y 0 0x19 0x2d
#read de-emphasis
#i2cget -y 0 0x18 0x15
#i2cget -y 0 0x19 0x15
#clr channel B
i2cset -y 0 0x18 0xff 0x00
i2cset -y 0 0x19 0xff 0x00

# De-select the mux channels which were selected for above configs
i2cset -y 0 0x76 0x0
i2cset -y 0 0x77 0x0

ir3570_check

echo "AS9716 post PDDF driver install completed"

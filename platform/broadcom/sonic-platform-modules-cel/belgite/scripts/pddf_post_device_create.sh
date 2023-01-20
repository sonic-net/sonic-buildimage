#!/bin/bash
# Set U60 shutdown threhold 80 
sudo i2cset -y -f 6 0x49 0x3 0x4e 0x00 i
sleep 0.1
sudo i2cset -y -f 6 0x49 0x1 0x2
sleep 0.1
#Set LM75 shutdown enable
sudo i2cset -y -f 2 0x32 0x45 0x1

# set sys led green status
sudo i2cset -y -f 2 0x32 0x43 0xec

echo -2 | tee /sys/bus/i2c/drivers/pca954x/*-00*/idle_state

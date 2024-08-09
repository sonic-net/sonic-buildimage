#!/bin/bash

# Enable FAN WDT
sudo i2cset -y -f 2 0x32 0x30 0x01

# Set all FAN speed to 100%
sudo i2cset -y -f 2 0x32 0x32 0xff
sudo i2cset -y -f 2 0x32 0x36 0xff
sudo i2cset -y -f 2 0x32 0x3a 0xff

# Set FAN LED status to GREEN
sudo i2cset -y -f 2 0x32 0x33 0x2
sudo i2cset -y -f 2 0x32 0x37 0x2
sudo i2cset -y -f 2 0x32 0x3b 0x2

# Set Alarm LED status to OFF, since it is unused in SONiC
sudo i2cset -y -f 2 0x32 0x44 0x00

# Set SYS LED status to GREEN
sudo i2cset -y -f 2 0x32 0x43 0xec

echo -2 | tee /sys/bus/i2c/drivers/pca954x/*-00*/idle_state

#!/bin/bash
#rmmod gpio_ich
modprobe -rq i2c_i801
modprobe -rq i2c_smbus
echo "Pre PDDF init steps completed successully"

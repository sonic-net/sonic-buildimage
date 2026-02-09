#!/bin/bash
#rmmod gpio_ich
if [ ! -f /tmp/._pddf_pre_driver_init_completion ]; then
    # make sure igb/i40e init in correct order
    rmmod i2c-i801
    rmmod i2c_ismt
    modprobe -r i2c-i801
    modprobe -r i2c_ismt
    date > /tmp/._pddf_pre_driver_init_completion
fi
echo "PDDF driver pre-install completed"

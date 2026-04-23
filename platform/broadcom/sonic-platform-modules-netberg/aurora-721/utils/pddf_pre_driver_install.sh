#!/bin/bash
#rmmod gpio_ich
if [ ! -f /tmp/._pddf_pre_driver_init_completion ]; then
    rmmod i2c_i801
    rmmod i2c_ismt
    date > /tmp/._pddf_pre_driver_init_completion
fi
echo "PDDF driver pre-install completed"

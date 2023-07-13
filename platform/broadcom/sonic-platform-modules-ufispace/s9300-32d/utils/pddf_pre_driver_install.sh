#!/bin/bash
#rmmod gpio_ich
if [ ! -f /tmp/._pddf_pre_driver_init_completion ]; then
    # make sure igb/i40e init in correct order
    rmmod i40e
    rmmod igb
    modprobe igb
    modprobe i40e
    date > /tmp/._pddf_pre_driver_init_completion
fi
echo "PDDF driver pre-install completed"

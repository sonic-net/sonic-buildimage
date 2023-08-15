#!/bin/bash

#disable bmc watchdog
echo "Disable BMC watchdog"
timeout 3 ipmitool mc watchdog off

pddf_ledutil setstatusled SYS_LED off
pddf_ledutil setstatusled ID_LED off

curr_led=$(pddf_ledutil getstatusled SYS_LED)
pddf_ledutil setstatusled SYS_LED green
echo "Set System $curr_led to green"

echo "PDDF device post-create completed"

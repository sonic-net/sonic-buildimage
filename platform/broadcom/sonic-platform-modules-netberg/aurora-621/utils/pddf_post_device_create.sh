#!/bin/bash

#disable bmc watchdog
echo "Disable BMC watchdog"
timeout 3 ipmitool mc watchdog off

pddf_ledutil setstatusled SYNC_LED off
pddf_ledutil setstatusled SYS_LED off
pddf_ledutil setstatusled ID_LED off

#set status led to green to indicate platform init done
curr_led=$(pddf_ledutil getstatusled SYS_LED)
pddf_ledutil setstatusled SYS_LED green
echo "Set SYS_LED from $curr_led to green"

echo "PDDF device post-create completed"

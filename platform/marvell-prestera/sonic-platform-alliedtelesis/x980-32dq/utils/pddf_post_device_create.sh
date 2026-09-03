#!/bin/bash

pddf_ledutil setstatusled SYS_LED off
pddf_ledutil setstatusled LOC_LED off

curr_led=$(pddf_ledutil getstatusled SYS_LED)
pddf_ledutil setstatusled SYS_LED green
echo "Set System $curr_led to green"

# Set the mgmt interface to the correct MAC address
python3 /usr/local/bin/get-mgmt-mac.py

echo "PDDF device post-create completed"

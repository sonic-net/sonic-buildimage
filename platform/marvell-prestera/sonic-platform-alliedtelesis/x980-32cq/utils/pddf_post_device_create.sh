#!/bin/bash

# Load the Marvell drivers. Loading from here allows the switch silicon time to power up properly
modprobe mvcpss
modprobe psample
modprobe mvSai

# Set the system LED to green
pddf_ledutil setstatusled SYS_LED green

# Set the mgmt interface to the correct MAC address
python3 /usr/local/bin/get-mgmt-mac.py

echo "PDDF device post-create completed"

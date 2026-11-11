#!/bin/bash

# Determine the running platform so we can apply platform-specific setup
platform=$(sed -n 's/^onie_platform=//p' /host/machine.conf)

case "$platform" in
    x86_64-alliedtelesis_x980_32cq-r0)
        # Load the Marvell drivers. Loading from here allows the switch
        # silicon time to power up properly
        modprobe mvcpss
        modprobe psample
        modprobe mvSai

        # Set the system LED to green
        pddf_ledutil setstatusled SYS_LED green
        ;;
    x86_64-alliedtelesis_x980_32dq-r0)
        pddf_ledutil setstatusled SYS_LED off
        pddf_ledutil setstatusled LOC_LED off

        curr_led=$(pddf_ledutil getstatusled SYS_LED)
        pddf_ledutil setstatusled SYS_LED green
        echo "Set System $curr_led to green"
        ;;
    *)
        echo "Warning: unrecognized platform '$platform', skipping platform-specific LED/driver setup" >&2
        ;;
esac

# Set the mgmt interface to the correct MAC address
python3 /usr/local/bin/get-mgmt-mac.py

echo "PDDF device post-create completed"

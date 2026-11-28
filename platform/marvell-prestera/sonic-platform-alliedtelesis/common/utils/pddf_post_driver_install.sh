#!/bin/bash

# Determine the running platform so we can apply platform-specific setup
platform=$(sed -n 's/^onie_platform=//p' /host/machine.conf)

case "$platform" in
    x86_64-alliedtelesis_x980_32dq-r0)
        # The Marvell Falcon device produces AER errors when it is reset
        # as part of a shutdown
        FALCON_ON_PEX=11ab:8400
        # get the Falcon's location as domain:bus:device.function
        FALCON_PCI_ADDR=$(lspci -D -n -d ${FALCON_ON_PEX} | cut -d' ' -f 1)
        PARENT_PATH=$(realpath /sys/bus/pci/devices/"$FALCON_PCI_ADDR"/..)
        PARENT_ADDR=$(basename "$PARENT_PATH")
        # stop the reporting of AER errors on shutdown ...
        /usr/bin/setpci -s $PARENT_ADDR ECAP_AER+0x08.L=0xffffffff
        /usr/bin/setpci -s $PARENT_ADDR ECAP_AER+0x14.L=0xffffffff
        ;;
    x86_64-alliedtelesis_x980_32cq-r0)
        # No additional post-install setup required on this platform
        ;;
    *)
        echo "Warning: unrecognized platform '$platform', skipping platform-specific post-install setup" >&2
        ;;
esac

echo "PDDF driver post-install completed"

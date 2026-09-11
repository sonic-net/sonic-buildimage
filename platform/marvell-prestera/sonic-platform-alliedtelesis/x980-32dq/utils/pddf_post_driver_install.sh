#!/bin/bash
#
# The Marvell Falcon device produces AER errors when it is reset as part of a shutdown
FALCON_ON_PEX=11ab:8400

# get the Falcon's location as domain:bus:device.function
FALCON_PCI_ADDR=$(lspci -D -n -d ${FALCON_ON_PEX} | cut -d' ' -f 1)

PARENT_PATH=$(realpath /sys/bus/pci/devices/"$FALCON_PCI_ADDR"/..)
PARENT_ADDR=$(basename "$PARENT_PATH")

# stop the reporting of AER errors on shutdown ...
/usr/bin/setpci -s $PARENT_ADDR ECAP_AER+0x08.L=0xffffffff
/usr/bin/setpci -s $PARENT_ADDR ECAP_AER+0x14.L=0xffffffff

echo "PDDF driver post-install completed"

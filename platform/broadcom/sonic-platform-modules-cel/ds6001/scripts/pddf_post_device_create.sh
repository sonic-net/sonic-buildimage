#!/bin/bash

# Get BMC mode
BMC_PRESENCE_SYS_PATH="/sys/bus/platform/devices/sys_cpld/bmc_present_l"
BMC_PRESENCE=`cat ${BMC_PRESENCE_SYS_PATH}`
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
HW_SKU=`awk '{print $1}' /usr/share/sonic/device/$PLATFORM/default_sku`

#Set SYS LED status to Green
if [ "${BMC_PRESENCE}" = "0" ]; then
    # TODO: Wait for BMC spec update.
    :
else
    :
fi

sync

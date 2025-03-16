#!/bin/bash

BMC_PRESENCE_SYSFS_PATH="/sys/devices/platform/sys_cpld/bmc_presence"

# Load the Baseboard CPLD LPC driver module
modprobe custom_lpc_basecpld
while [ ! -f $BMC_PRESENCE_SYSFS_PATH ]
do
    sleep 1
done

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE=`cat $BMC_PRESENCE_SYSFS_PATH`

# Copy pddf-device.json according to bmc mode
PDDF_JSON="pddf-device.json"
PDDF_JSON_BMC="pddf-device-bmc.json"
PDDF_JSON_NONBMC="pddf-device-nonbmc.json"
PDDF_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/pddf"

COMPONENTS_JSON="platform_components.json"
COMPONENTS_JSON_BMC="platform_components-bmc.json"
COMPONENTS_JSON_NONBMC="platform_components-nonbmc.json"
COMPONENTS_JSON_PATH="/usr/share/sonic/device/${PLATFORM}"

if [ ${BMC_PRESENCE} == "1" ]; then
    # BMC Card present"
    echo "BMC card is found! - Loading BMC variant"
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_BMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_BMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}
else
    # BMC Card absent
    echo "BMC card is not found! - Loading Non-BMC variant "
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_NONBMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_NONBMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}
fi

sync

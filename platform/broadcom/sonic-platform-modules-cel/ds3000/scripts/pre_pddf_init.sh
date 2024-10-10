#!/bin/bash

# Probe Baseboard CPLD driver
modprobe baseboard_cpld
sleep 1

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE_SYS_PATH="/sys/bus/platform/devices/baseboard/bmc_presence"
BMC_PRESENCE=`cat ${BMC_PRESENCE_SYS_PATH}`
echo "Platform ${PLATFORM} BMC card ${BMC_PRESENCE}"

# Copy pddf-device.json according to bmc mode
PDDF_JSON="pddf-device.json"
PDDF_JSON_BMC="pddf-device-bmc.json"
PDDF_JSON_NONBMC="pddf-device-nonbmc.json"
PDDF_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/pddf"
PLATFORM_COMPONENTS_FILE="/usr/share/sonic/device/${PLATFORM}/platform_components.json"
if [ ${BMC_PRESENCE} == "present" ]; then
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_BMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    # Add BMC component if BMC exists
    if ! grep -q "BMC" ${PLATFORM_COMPONENTS_FILE}; then
        sed -i '6i \                "BMC": {},' ${PLATFORM_COMPONENTS_FILE}
    fi
else
    # BMC Card absent
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_NONBMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    # Remove BMC component for NonBMC
    sed -i '/"BMC"/d' ${PLATFORM_COMPONENTS_FILE}
fi

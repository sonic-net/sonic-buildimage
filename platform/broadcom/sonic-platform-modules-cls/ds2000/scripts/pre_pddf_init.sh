#!/bin/bash

# Probe Baseboard CPLD driver
modprobe lpc_basecpld
sleep 1

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
GETREG_PATH="/sys/devices/platform/sys_cpld/getreg"
BMC_PRESENCE=`echo '0xA108' > $GETREG_PATH && cat $GETREG_PATH`
echo "Platform ${PLATFORM} BMC card ${BMC_PRESENCE}"

# Copy pddf-device.json according to bmc mode
PDDF_JSON="pddf-device.json"
PDDF_JSON_BMC="pddf-device-bmc.json"
PDDF_JSON_NONBMC="pddf-device-nonbmc.json"
PDDF_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/pddf"

COMPONENTS_JSON="platform_components.json"
COMPONENTS_JSON_BMC="platform_components-bmc.json"
COMPONENTS_JSON_NONBMC="platform_components-nonbmc.json"
COMPONENTS_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/"

if [ ${BMC_PRESENCE} == "0x00" ]; then
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_BMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_BMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}
else
    # BMC Card absent
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_NONBMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_NONBMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}
fi

#!/bin/bash

modprobe i2c-ismt
sleep 0.1
modprobe i2c-i801

# Probe Baseboard CPLD driver
modprobe lpc_baseboard_cpld1
sleep 0.1
modprobe lpc_baseboard_cpld2
sleep 1

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE=`cat /sys/devices/platform/sys_cpld1/bmc_present`
echo "Platform ${PLATFORM} BMC card present status: ${BMC_PRESENCE}"

# Copy pddf-device.json and platform_components.json according to bmc mode
PDDF_JSON="pddf-device.json"
PDDF_JSON_BMC="pddf-device-bmc.json"
PDDF_JSON_NONBMC="pddf-device-nonbmc.json"
PDDF_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/pddf"

COMPONENTS_JSON="platform_components.json"
COMPONENTS_JSON_BMC="platform_components-bmc.json"
COMPONENTS_JSON_NONBMC="platform_components-nonbmc.json"
COMPONENTS_JSON_PATH="/usr/share/sonic/device/${PLATFORM}/"

REGISTER_PATH="/sys/devices/platform/sys_cpld1"
REGISTER_ADDRESS="0xa2ac"
NONE_BMC_I2C_CONTROL="0x01"

if [ ${BMC_PRESENCE} == "1" ]; then
    # BMC Card present
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_BMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_BMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}

else
    # BMC Card absent
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_NONBMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_NONBMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}

    # Configure the I2C isolation register to enable COMe communication with
    # MUX PCA9548 (0x72 and 0x70), CPLD1 (0x32), and CPLD2 (0x33).
    echo "Setting register $REGISTER_ADDRESS to $NONE_BMC_I2C_CONTROL..."
    if ! echo "$REGISTER_ADDRESS $NONE_BMC_I2C_CONTROL" > "$REGISTER_PATH/setreg"; then
        echo "ERROR: Failed to write to $REGISTER_PATH/setreg" >&2
    fi
    sleep 3

fi

# Set lpmode as normal power
for i in {1..2}; do
    target="/sys/bus/platform/devices/sys_cpld1/qsfp${i}_lpmode"
    if [ -f "$target" ]; then
        echo "0" > "$target"
    else
        echo "Warning: Port QSFP28$i device path not found."
    fi
done

sync

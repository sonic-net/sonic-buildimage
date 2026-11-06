#!/bin/bash

# Probe Baseboard CPLD driver
modprobe custom_lpc_basecpld
sleep 1

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE=`cat /sys/devices/platform/sys_cpld/bmc_present`
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

# Finds hwmon device by name and creates a symlink in /dev/hwmon/
# Usage: create_hwmon_symlink <hwmon_name> <symlink_name> [timeout_attempts]
create_hwmon_symlink() {
    local hwmon_name="$1"
    local symlink_name="$2" # Just the name, path is fixed to /dev/hwmon/
    local timeout="${3:-3}" # Default timeout 3 attempts (1 sec per attempt)
    local symlink_path="/dev/hwmon/$symlink_name"
    local i
    local HWMON_PATH
    local found=0 # Flag to track success

    if [ -z "$hwmon_name" ] || [ -z "$symlink_name" ]; then
        echo "Usage: create_hwmon_symlink <hwmon_name> <symlink_name> [timeout_attempts]" >&2
        return 1
    fi

    echo "Searching for hwmon device '$hwmon_name' (timeout: ${timeout} attempts)..."

    for i in $(seq 1 "$timeout"); do
        for HWMON_PATH in /sys/class/hwmon/hwmon*; do
            if [ -d "$HWMON_PATH" ] && [ -f "${HWMON_PATH}/name" ]; then
                hwmon_read_name=$(cat "${HWMON_PATH}/name")
                if [ $? -eq 0 ] && [ "$hwmon_read_name" = "$hwmon_name" ]; then
                    echo "Found '$hwmon_name' at ${HWMON_PATH}"
                    mkdir -p /dev/hwmon
                    ln -sf "${HWMON_PATH}" "${symlink_path}"
                    if [ $? -eq 0 ]; then
                        echo "Created symlink: ${symlink_path} -> ${HWMON_PATH}"
                        found=1 # Mark as found
                        break 2
                    else
                        echo "Error creating symlink ${symlink_path}" >&2
                    fi
                fi
            fi
        done
        if [ "$found" -eq 1 ]; then
            break
        elif [ "$i" -lt "$timeout" ]; then # Only sleep if not the last attempt
            sleep 1
        fi
    done

    if [ "$found" -eq 0 ]; then
        echo "Error: hwmon device '$hwmon_name' not found after ${timeout} attempts." >&2
        return 1
    fi

    return 0
}





if [ ${BMC_PRESENCE} == "0" ]; then
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_BMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_BMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}
else
    # BMC Card absent
    cp ${PDDF_JSON_PATH}/${PDDF_JSON_NONBMC} ${PDDF_JSON_PATH}/${PDDF_JSON}
    cp ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON_NONBMC} ${COMPONENTS_JSON_PATH}/${COMPONENTS_JSON}

     modprobe ds5001_mp2975
    #modprobe mp2891
      # MMIO for DIMM Temperature
    modprobe ds5001_dimm_temp
    create_hwmon_symlink "dimm_temp" "dimm_temp"

fi

sync

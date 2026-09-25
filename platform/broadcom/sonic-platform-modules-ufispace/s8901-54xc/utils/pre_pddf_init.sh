#!/bin/bash

# Import SONiC environment
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

TRUE=0
FALSE=1

# Hardware ext id definitions
EXT_ID_0=0
EXT_ID_1=1

NTM_PRESENCE=0

PLATFORM=${PLATFORM:-x86_64-ufispace_s8901_54xc-r0}
HWSKU=${HWSKU:-UFISPACE-S8901-54XC}
DEV_BASE="/usr/share/sonic/device/$PLATFORM"
BCM_CONF_FILE_PATH="$DEV_BASE/$HWSKU"
PDDF_BASE="$DEV_BASE/pddf"
IO_PORT_FILE="/dev/port"

# Function to check file existence
function _check_filepath {
    local filepath=$1
    if [ -z "$filepath" ]; then
        echo "[ERR] The input string is empty!"
        return $FALSE
    elif [ ! -f "$filepath" ] && [ ! -c "$filepath" ]; then
        echo "[ERR] No such file: $filepath"
        return $FALSE
    else
        return $TRUE
    fi
}

# Function to detect hardware ext ID
function check_hw_ext_id {
    if _check_filepath "$IO_PORT_FILE"; then
        if ! REG=$(xxd -s 0x706 -p -l 1 -c 1 "$IO_PORT_FILE" 2>/dev/null); then
            echo "[ERR] Failed to read from $IO_PORT_FILE"
            HW_EXT_ID=-1
        else
            MASK=0x07
            HW_EXT_ID=$(( 0x$REG & MASK ))
        fi
    else
        HW_EXT_ID=-1
    fi
    echo "HW_EXT_ID detected: $HW_EXT_ID"
}

# Function to check NTM presence
function check_ntm_presence {
    check_hw_ext_id
    if [ "$HW_EXT_ID" -eq "$EXT_ID_0" ] || [ "$HW_EXT_ID" -eq "$EXT_ID_1" ]; then
        NTM_PRESENCE=1
    else
        NTM_PRESENCE=0
    fi
}

# Function to configure device JSON file
function config_device {
    local src
    if [ "$NTM_PRESENCE" -eq 1 ]; then
        src="$PDDF_BASE/pddf-device-ntm.json"
    else
        src="$PDDF_BASE/pddf-device-no-ntm.json"
    fi
    _check_filepath "$src" && ln -rsf "$src" "$PDDF_BASE/pddf-device.json"
    echo "pddf-device.json: $src"
}

# Execute functions
check_ntm_presence
config_device

echo "PDDF/PLATFORM/CONFIG pre-init completed"

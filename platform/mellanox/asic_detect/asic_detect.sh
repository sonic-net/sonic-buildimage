#!/usr/bin/env bash

if [[ -f "/usr/bin/asic_detect/asic_type" ]]; then
    cat /usr/bin/asic_detect/asic_type
else
    declare -A DEVICE_DICT=(
        ["cb84"]="spc1"
        ["cf6c"]="spc2"
        ["cf70"]="spc3"
        ["cf80"]="spc4"
        ["cf82"]="spc5"
        ["a2dc"]="bf3"
    )
    TYPE_UNKNOWN="unknown"
    VENDOR_ID="15b3"
    DEVICE_TYPE=$TYPE_UNKNOWN

    DEVICE_ID=$(lspci -n | awk -v vid="$VENDOR_ID" '$0 ~ vid {print $NF}' | cut -d: -f2)
    # Check if DEVICE_ID exists in the DEVICE_DICT
    if [[ -n "${DEVICE_DICT[$DEVICE_ID]}" ]]; then
        DEVICE_TYPE="${DEVICE_DICT[$DEVICE_ID]}"
    else
        DEVICE_TYPE=$TYPE_UNKNOWN
    fi

    echo "$DEVICE_TYPE" | tee >( [[ "$DEVICE_TYPE" != "$TYPE_UNKNOWN" ]] && cat > /usr/bin/asic_detect/asic_type )

fi

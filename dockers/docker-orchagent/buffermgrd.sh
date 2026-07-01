#!/usr/bin/env bash

HWSKU_DIR="/usr/share/sonic/hwsku"
PLATFORM_DIR="/usr/share/sonic/platform"
TARGET_FILE="pg_profile_lookup.ini"

HWSKU_FILE="${HWSKU_DIR}/${TARGET_FILE}"
PLATFORM_FILE="${PLATFORM_DIR}/${TARGET_FILE}"

BUFFER_CALCULATION_MODE=$(redis-cli -n 4 hget "DEVICE_METADATA|localhost" buffer_model)

if [ "$BUFFER_CALCULATION_MODE" == "dynamic" ]; then
    BUFFERMGRD_ARGS="-a /etc/sonic/asic_table.json"
    if [ -f /etc/sonic/peripheral_table.json ]; then
        BUFFERMGRD_PERIPHERIAL_ARGS=" -p /etc/sonic/peripheral_table.json"
    fi
    if [ -f /etc/sonic/zero_profiles.json ]; then
        BUFFERMGRD_ZERO_PROFILE_ARGS=" -z /etc/sonic/zero_profiles.json"
    fi
else
    if [ -f "$HWSKU_FILE" ]; then
        echo "Info: Found ${TARGET_FILE} in HWSKU directory: $HWSKU_FILE"
        BUFFERMGRD_ARGS="-l $HWSKU_FILE"
    else
        echo "Warning: ${TARGET_FILE} not found in HWSKU. Falling back to Platform directory: $PLATFORM_FILE"
        BUFFERMGRD_ARGS="-l $PLATFORM_FILE"
    fi
fi

exec /usr/bin/buffermgrd ${BUFFERMGRD_ARGS} ${BUFFERMGRD_PERIPHERIAL_ARGS} ${BUFFERMGRD_ZERO_PROFILE_ARGS}

#!/usr/bin/env bash

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
    # Look for pg_profile_lookup.ini in HWSKU directory first,
    # then fall back to the platform-level shared copy.
    if [ -f /usr/share/sonic/hwsku/pg_profile_lookup.ini ]; then
        BUFFERMGRD_ARGS="-l /usr/share/sonic/hwsku/pg_profile_lookup.ini"
    elif [ -f /usr/share/sonic/platform/pg_profile_lookup.ini ]; then
        BUFFERMGRD_ARGS="-l /usr/share/sonic/platform/pg_profile_lookup.ini"
    else
        BUFFERMGRD_ARGS="-l /usr/share/sonic/hwsku/pg_profile_lookup.ini"
    fi
fi

exec /usr/bin/buffermgrd ${BUFFERMGRD_ARGS} ${BUFFERMGRD_PERIPHERIAL_ARGS} ${BUFFERMGRD_ZERO_PROFILE_ARGS}

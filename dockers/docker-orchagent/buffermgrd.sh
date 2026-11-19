#!/usr/bin/env bash

BUFFER_CALCULATION_MODE=$(sonic-db-cli -s CONFIG_DB hget "DEVICE_METADATA|localhost" buffer_model)

if [ "$BUFFER_CALCULATION_MODE" == "dynamic" ]; then
    BUFFERMGRD_ARGS="-a /etc/sonic/asic_table.json"
    if [ -f /etc/sonic/peripheral_table.json ]; then
        BUFFERMGRD_PERIPHERIAL_ARGS=" -p /etc/sonic/peripheral_table.json"
    fi
    if [ -f /etc/sonic/zero_profiles.json ]; then
        BUFFERMGRD_ZERO_PROFILE_ARGS=" -z /etc/sonic/zero_profiles.json"
    fi
else
    # Preserve HWSKU overrides and allow Generic HWSKUs to share a platform table.
    PG_PROFILE_LOOKUP="/usr/share/sonic/hwsku/pg_profile_lookup.ini"
    if [ ! -f "$PG_PROFILE_LOOKUP" ] && [ -f /usr/share/sonic/platform/pg_profile_lookup.ini ]; then
        PG_PROFILE_LOOKUP="/usr/share/sonic/platform/pg_profile_lookup.ini"
    fi
    BUFFERMGRD_ARGS="-l $PG_PROFILE_LOOKUP"
fi

exec /usr/bin/buffermgrd ${BUFFERMGRD_ARGS} ${BUFFERMGRD_PERIPHERIAL_ARGS} ${BUFFERMGRD_ZERO_PROFILE_ARGS}

#!/bin/bash

is_chassis_supervisor() {
    if [ -f /etc/sonic/chassisdb.conf ]; then
        PLATFORM=$(sonic-db-cli CONFIG_DB HGET "DEVICE_METADATA|localhost" "platform")
        PLATFORM_ENV_CONF=/usr/share/sonic/device/$PLATFORM/platform_env.conf
        [ -f $PLATFORM_ENV_CONF ] && . $PLATFORM_ENV_CONF
        if [[ $disaggregated_chassis -ne 1 ]]; then
            true
            return
        fi
    fi
    false
    return
}

check_asic_status() {
    # Ignore services that are not started in namespace.
    if [[ -z $DEV ]]; then
        return 0
    fi

    # For chassis supervisor, wait for asic to be online
    /usr/local/bin/asic_status.py $SERVICE $DEV
    if [[ $? = 0 ]]; then
        debug "$SERVICE successfully detected asic $DEV..."
        return 0
    fi
    debug "$SERVICE failed to detect asic $DEV..."
    return 1
}

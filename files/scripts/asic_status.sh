#!/bin/bash

PLATFORM="$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)"
PLATFORM_ENV_CONF=/usr/share/sonic/device/$PLATFORM/platform_env.conf

is_chassis_supervisor() {
    if [ -f "$PLATFORM_ENV_CONF" ]; then
        source $PLATFORM_ENV_CONF
         if [ -v supervisor ]; then
            if [ $supervisor -eq 1 ]; then
                true
                return
            fi
         fi
         false
         return
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

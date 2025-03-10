#!/bin/bash

is_chassis_supervisor() {
    if [ -f /etc/sonic/chassisdb.conf ]; then
        # if /etc/sonic/chassisdb.con exists, check platform_env.conf and its supervisor=1 definition
        # to determinate if it is a Supervisor or Pizzabox with database-chassis support
        PLATFORM="$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)"
        PLATFORM_ENV_CONF=/usr/share/sonic/device/$PLATFORM/platform_env.conf
        if [ -f "$PLATFORM_ENV_CONF" ]; then
            source $PLATFORM_ENV_CONF
            if [ -v supervisor ]; then
                if [ $supervisor -eq 1 ]; then
                    true
                    return
                fi
            fi
        else
            # return true if there is no platform_env.conf file
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

#!/bin/bash

LOG_TAG="bcm_intf_init"
PLATFORM_ENV_FILE="/usr/share/sonic/platform/platform_env.conf"

# Source platform_env.conf
if [ -f "$PLATFORM_ENV_FILE" ]; then
    source "$PLATFORM_ENV_FILE"
else
    logger -t $LOG_TAG -p error "Error: $PLATFORM_ENV_FILE not found"
    exit 1
fi

if [ "$use_napi" = "1" ]; then
    logger -t $LOG_TAG "NAPI is enabled, proceeding with bcm intf init"
    for dev in bcm0 bcm1; do
        if [ -d "/sys/class/net/$dev" ]; then
            /sbin/ip link set "$dev" up
            logger -t $LOG_TAG "Interface $dev set to UP"
        else
            logger -p user.warning -t $LOG_TAG "Interface $dev not present, skipping..."
        fi
    done
else
    logger -t $LOG_TAG "NAPI is disabled, skipping bcm intf init"
fi

exit 0

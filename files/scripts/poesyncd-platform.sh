#!/bin/bash

PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}
HWSKU=${HWSKU:-`sonic-cfggen -d -v DEVICE_METADATA.localhost.hwsku`}
POECONFIG="/usr/share/sonic/device/${PLATFORM}/${HWSKU}/poe_config.json"

if [ ! -f "$POECONFIG" ]; then
    sudo config feature state poesyncd disabled
    exit 1
fi

exit 0

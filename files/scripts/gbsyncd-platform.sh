#!/bin/bash
# Check the gbsyncd platform defined on the device matching the service,
# or otherwise skip starting the service

SERVICE="$gbsyncd_platform"
PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}
DEVPATH="/usr/share/sonic/device"
CONFIGFILE="${DEVPATH}/${PLATFORM}/gbsyncd.ini"

# Skip checking the service for vs
[ "$sonic_asic_platform" = vs ] && exit 0

if [ ! -f "$CONFIGFILE" ]; then
    exit 1
fi

while IFS="=" read -r key value; do
    case "$key" in
        platform)
            if [[ "$value" = "$SERVICE"* ]]; then
                exit 0
            fi
            ;;
    esac
done < "$CONFIGFILE"

exit 1

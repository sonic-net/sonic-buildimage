#!/bin/sh

VRF_ENABLED=$(sonic-db-cli CONFIG_DB HGET "MGMT_VRF_CONFIG|vrf_global" "mgmtVrfEnabled" 2> /dev/null)
if [ "$VRF_ENABLED" = "true" ]; then
    VRF_CONFIGURED=$(sonic-db-cli CONFIG_DB HGET "NTP|global" "vrf" 2> /dev/null)
    if [ "$VRF_CONFIGURED" = "default" ]; then
        echo "Starting NTP server in default-vrf for default set as NTP vrf"
        exec /usr/sbin/chronyd $DAEMON_OPTS
    else
        echo "Starting NTP server in mgmt-vrf"
        exec ip vrf exec mgmt /usr/sbin/chronyd $DAEMON_OPTS
    fi
else
    echo "Starting NTP server in default-vrf"
    exec /usr/sbin/chronyd $DAEMON_OPTS
fi

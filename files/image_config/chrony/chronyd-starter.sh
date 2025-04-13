#!/bin/sh

VRF_ENABLED=$(/usr/local/bin/sonic-cfggen -d -v 'MGMT_VRF_CONFIG["vrf_global"]["mgmtVrfEnabled"]' 2> /dev/null)
if [ "$VRF_ENABLED" = "true" ]; then
    VRF_CONFIGURED=$(/usr/local/bin/sonic-cfggen -d -v 'NTP["global"]["vrf"]' 2> /dev/null)
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

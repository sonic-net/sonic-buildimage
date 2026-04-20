#!/bin/bash

# Start sshd in mgmt VRF if mgmt VRF is enabled, otherwise start in default VRF.
# This ensures sshd binds its listening socket inside the correct VRF namespace
# so that management traffic is routed through the mgmt VRF table without
# needing ip rule hacks based on source address matching.

VRF_ENABLED=$(sonic-db-cli CONFIG_DB hget "MGMT_VRF_CONFIG|vrf_global" "mgmtVrfEnabled" 2> /dev/null)
if [ "$VRF_ENABLED" = "true" ]; then
    echo "Starting sshd in mgmt-vrf"
    exec ip vrf exec mgmt /usr/sbin/sshd -D $SSHD_OPTS
else
    echo "Starting sshd in default-vrf"
    exec /usr/sbin/sshd -D $SSHD_OPTS
fi

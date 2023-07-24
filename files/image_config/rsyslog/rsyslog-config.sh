#!/bin/bash

PLATFORM=`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`

# Parse the device specific asic conf file, if it exists
ASIC_CONF=/usr/share/sonic/device/$PLATFORM/asic.conf
if [ -f "$ASIC_CONF" ]; then
    source $ASIC_CONF
fi

# On Multi NPU platforms we need to start  the rsyslog server on the docker0 ip address
# for the syslogs from the containers in the namespaces to work.
# on Single NPU platforms we continue to use loopback adddres

if [[ ($NUM_ASIC -gt 1) ]]; then
    udp_server_ip=$(ip -o -4 addr list docker0 | awk '{print $4}' | cut -d/ -f1)
else
    udp_server_ip=$(ip -j -4 addr list lo scope host | jq -r -M '.[0].addr_info[0].local')
fi
hostname=$(hostname)

sonic-cfggen -d -t /usr/share/sonic/templates/rsyslog.conf.j2 \
    -a "{\"udp_server_ip\": \"$udp_server_ip\", \"hostname\": \"$hostname\"}" \
    > /etc/rsyslog.conf

systemctl restart rsyslog

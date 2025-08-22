#!/bin/bash

PLATFORM=$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)

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

contain_dhcp_server=$(sonic-db-cli CONFIG_DB keys "FEATURE|dhcp_server")
if [ $contain_dhcp_server ]; then
    docker0_ip=$(ip -o -4 addr list docker0 | awk '{print $4}' | cut -d/ -f1)
fi

hostname=$(hostname)

syslog_with_osversion=$(sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "syslog_with_osversion")
if [ -z "$syslog_with_osversion" ]; then
    syslog_with_osversion="false"
fi

os_version=$(sonic-cfggen -y /etc/sonic/sonic_version.yml -v build_version)
if [ -z "$os_version" ]; then
    os_version="Unknown"
fi

syslog_counter=$(sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "syslog_counter")
if [ -z "$syslog_counter" ]; then
    syslog_counter="false"
fi

sonic-cfggen -d -t /usr/share/sonic/templates/rsyslog.conf.j2 \
    -a "{\"udp_server_ip\": \"$udp_server_ip\", \"hostname\": \"$hostname\", \"docker0_ip\": \"$docker0_ip\", \"forward_with_osversion\": \"$syslog_with_osversion\", \"os_version\": \"$os_version\", \"syslog_counter\": \"$syslog_counter\"}" \
    > /etc/rsyslog.conf

systemctl restart rsyslog

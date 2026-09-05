#!/bin/bash -e

# generate configuration
[ -d /etc/sonic ] || mkdir -p /etc/sonic

if ! ip link show eth0 &> /dev/null; then
	ip link add eth0 type dummy
fi

SYSTEM_MAC_ADDRESS=$(ip link show eth0 | grep ether | awk '{print $2}')
sonic-cfggen -a '{"DEVICE_METADATA":{"localhost": {"mac": "'$SYSTEM_MAC_ADDRESS'"}}}' --print-data > /etc/sonic/init_cfg.json

if [ -f /etc/sonic/config_db.json ]; then
    sonic-cfggen -j /etc/sonic/init_cfg.json -j /etc/sonic/config_db.json --print-data > /tmp/config_db.json
    mv /tmp/config_db.json /etc/sonic/config_db.json
else
    sonic-cfggen -j /etc/sonic/init_cfg.json --print-data > /etc/sonic/config_db.json
fi

mkdir -p /etc/swss/config.d/

rm -f /var/run/rsyslogd.pid

echo "Start rsyslogd"
supervisorctl start rsyslogd

mkdir -p /var/run/redis

echo "Start redis"
supervisorctl start redis-server

echo "Veth Setup"
veth_setup.sh > /tmp/veth_setup.log

echo "Start BM"
rm -rf bm_logs/bridge_log.*
rm -rf bm_logs/router_log.*
rm -rf log.txt
mkdir -p bm_logs
supervisorctl start bm_bridge
supervisorctl start bm_router

sleep 10
echo "BM Default config"
config_bm.sh > /tmp/config_bm.log

/usr/bin/configdb-load.sh

echo "Start syncd"
supervisorctl start syncd

echo "Start orchagent"
supervisorctl start orchagent

echo "Start portsyncd"
supervisorctl start portsyncd

echo "Start neighsyncd"
supervisorctl start neighsyncd

echo "Start fdbsyncd"
supervisorctl start fdbsyncd

echo "Start teamsyncd"
supervisorctl start teamsyncd

echo "Start fpmsyncd"
supervisorctl start fpmsyncd

echo "Start intfmgrd"
supervisorctl start intfmgrd

echo "Start vlanmgrd"
supervisorctl start vlanmgrd

echo "Start zebra"
supervisorctl start zebra

echo "Start bgpd"
supervisorctl start bgpd

if [ -f /etc/swss/config.d/default_config.json ]; then
	swssconfig /etc/swss/config.d/default_config.json
fi

# Start arp_update when VLAN exists
VLAN=`sonic-cfggen -d -v 'VLAN.keys() | join(" ") if VLAN'`
if [ "$VLAN" != "" ]; then
    echo "Start arp_update"
    supervisorctl start arp_update
fi

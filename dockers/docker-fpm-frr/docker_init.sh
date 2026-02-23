#!/usr/bin/env bash

mkdir -p /etc/frr
mkdir -p /etc/supervisor/conf.d

CFGGEN_PARAMS=" \
    -d \
    -y /etc/sonic/constants.yml \
    -t /usr/share/sonic/templates/frr_vars.j2 \
    -t /usr/share/sonic/templates/supervisord/supervisord.conf.j2,/etc/supervisor/conf.d/supervisord.conf \
    -t /usr/share/sonic/templates/supervisord/critical_processes.j2,/etc/supervisor/critical_processes \
    -t /usr/share/sonic/templates/isolate.j2,/usr/sbin/bgp-isolate \
    -t /usr/share/sonic/templates/unisolate.j2,/usr/sbin/bgp-unisolate \
"

FRR_VARS=$(sonic-cfggen $CFGGEN_PARAMS)
CONFIG_TYPE=$(echo $FRR_VARS | jq -r '.docker_routing_config_mode')

update_default_gw()
{
   IP_VER=${1}
   # FRR is not running in host namespace so we need to delete
   # default gw kernel route added by docker network via eth0 and add it back
   # with higher administrative distance so that default route learnt
   # by FRR becomes best route if/when available
   GATEWAY_IP=$(ip -${IP_VER} route show default dev eth0 | awk '{print $3}')
   #Check if docker default route is there
   if [[ ! -z "$GATEWAY_IP" ]]; then
      ip -${IP_VER} route del default dev eth0
      #Make sure route is deleted
      CHECK_GATEWAY_IP=$(ip -${IP_VER} route show default dev eth0 | awk '{print $3}')
      if [[ -z "$CHECK_GATEWAY_IP" ]]; then
         # Ref: http://docs.frrouting.org/en/latest/zebra.html#zebra-vrf
         # Zebra does treat Kernel routes as special case for the purposes of Admin Distance. \
         # Upon learning about a route that is not originated by FRR we read the metric value as a uint32_t.
         # The top byte of the value is interpreted as the Administrative Distance and
         # the low three bytes are read in as the metric.
         # so here we are programming administrative distance of 210 (210 << 24) > 200 (for routes learnt via IBGP)
         ip -${IP_VER} route add default via $GATEWAY_IP dev eth0 metric 3523215360
      fi
   fi
}

write_default_zebra_config()
{
    FILE_NAME=${1}

    grep -q '^no fpm use-next-hop-groups' $FILE_NAME || {
        echo "no fpm use-next-hop-groups" >> $FILE_NAME
        echo "fpm address 127.0.0.1" >> $FILE_NAME
    }
}

if [[ ! -z "$NAMESPACE_ID" ]]; then
   update_default_gw 4
   update_default_gw 6
fi

if [ -z "$CONFIG_TYPE" ] || [ "$CONFIG_TYPE" == "separated" ]; then
    CFGGEN_PARAMS=" \
        -d \
        -y /etc/sonic/constants.yml \
        -t /usr/share/sonic/templates/bgpd/gen_bgpd.conf.j2,/etc/frr/bgpd.conf \
        -t /usr/share/sonic/templates/zebra/zebra.conf.j2,/etc/frr/zebra.conf \
        -t /usr/share/sonic/templates/staticd/gen_staticd.conf.j2,/etc/frr/staticd.conf \
    "
    MGMT_FRAMEWORK_CONFIG=$(echo $FRR_VARS | jq -r '.frr_mgmt_framework_config')
    if [ -n "$MGMT_FRAMEWORK_CONFIG" ] && [ "$MGMT_FRAMEWORK_CONFIG" != "false" ]; then
        CFGGEN_PARAMS="$CFGGEN_PARAMS \
            -t /usr/local/sonic/frrcfgd/bfdd.conf.j2,/etc/frr/bfdd.conf \
            -t /usr/local/sonic/frrcfgd/ospfd.conf.j2,/etc/frr/ospfd.conf \
        "
    else
        rm -f /etc/frr/bfdd.conf /etc/frr/ospfd.conf
    fi
    sonic-cfggen $CFGGEN_PARAMS
    echo "no service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/frr.conf
elif [ "$CONFIG_TYPE" == "split" ]; then
    echo "no service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/frr.conf
    write_default_zebra_config /etc/frr/zebra.conf
elif [ "$CONFIG_TYPE" == "split-unified" ]; then
    echo "service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/bgpd.conf /etc/frr/zebra.conf /etc/frr/staticd.conf
    write_default_zebra_config /etc/frr/frr.conf
elif [ "$CONFIG_TYPE" == "unified" ]; then
    CFGGEN_PARAMS=" \
        -d \
        -y /etc/sonic/constants.yml \
        -T /usr/local/sonic/frrcfgd \
        -t /usr/share/sonic/templates/gen_frr.conf.j2,/etc/frr/frr.conf \
    "
    sonic-cfggen $CFGGEN_PARAMS
    echo "service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/bgpd.conf /etc/frr/zebra.conf /etc/frr/staticd.conf \
          /etc/frr/bfdd.conf /etc/frr/ospfd.conf /etc/frr/pimd.conf
fi

chown -R frr:frr /etc/frr/

# Create sr0 interface for SRv6 support
SONIC_ASIC_TYPE=$(sed -n 's/^asic_type:[[:space:]]*//p' /etc/sonic/sonic_version.yml)
if [ "$SONIC_ASIC_TYPE" == "vpp" ]; then
    if ! ip link show sr0 > /dev/null 2>&1; then
        echo "Interface sr0 does not exist. Creating sr0..."
        ip link add sr0 type dummy || true
    else
        echo "Interface sr0 already exists."
    fi
    ip link set sr0 up || true
fi

chown root:root /usr/sbin/bgp-isolate
chmod 0755 /usr/sbin/bgp-isolate

chown root:root /usr/sbin/bgp-unisolate
chmod 0755 /usr/sbin/bgp-unisolate

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

exec /usr/local/bin/supervisord

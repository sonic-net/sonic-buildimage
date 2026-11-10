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

FRR_SUPERVISOR_CONTEXT='{"frr_has_namespace":false}'
if [[ -n "${NAMESPACE_ID:-}" ]]; then
    FRR_SUPERVISOR_CONTEXT='{"frr_has_namespace":true}'
fi
FRR_VARS=$(sonic-cfggen $CFGGEN_PARAMS -a "$FRR_SUPERVISOR_CONTEXT")
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

render_integrated_frr_config()
(
    # Freeze one ConfigDB read before rendering any managed template. Both the
    # full candidate and its independent fallback use exactly these JSON bytes.
    # This subshell owns secret temporary files and cleans them before FRR starts.
    umask 077
    startup_dir=$(mktemp -d /tmp/frr-startup.XXXXXX) || return 1
    trap 'rm -rf -- "$startup_dir"' EXIT
    trap 'exit 1' HUP INT TERM
    snapshot="$startup_dir/snapshot.json"
    candidate="$startup_dir/candidate.conf"
    outcome="$startup_dir/outcome.json"
    fallback="$startup_dir/fallback.json"
    if ! sonic-cfggen -d -y /etc/sonic/constants.yml --print-data > "$snapshot"; then
        echo "Failed to capture the FRR startup snapshot" >&2
        return 1
    fi
    rendered=false
    if sonic-cfggen -j "$snapshot" -T /usr/local/sonic/frrcfgd \
            -t /usr/share/sonic/templates/gen_frr.conf.j2,"$candidate"; then
        rendered=true
    else
        # Never let the parser see a partially rendered candidate.
        rm -f -- "$candidate"
    fi

    # The binary location is shared with the existing supervisord commands.
    frr_version=$(PATH="/usr/lib/frr:$PATH" zebra --version 2>/dev/null |
        awk 'NR == 1 {for (i = 1; i <= NF; i++) if ($i ~ /^[0-9]+\.[0-9]+(\.[0-9]+)?([_-].*)?$/) {sub(/[_-].*/, "", $i); print $i; exit}}')
    if python3 -m frrcfgd.isis_startup --snapshot "$snapshot" \
            --rendered-config "$candidate" --frr-version "$frr_version" \
            --namespace "${NAMESPACE_ID:-}" > "$outcome" && [ "$rendered" = true ]; then
        cp -- "$candidate" /etc/frr/frr.conf
        return $?
    fi

    # A failed registration must never emit an unregistered managed slice.
    # Named/mixed profiles and other namespaces keep their legacy IS-IS tables.
    profile=$(jq -r '.profile // "canonical"' "$outcome" 2>/dev/null)
    if ! python3 - "$snapshot" "$fallback" "$profile" "${NAMESPACE_ID:-}" <<'PY'
import json
import sys

with open(sys.argv[1]) as source:
    config = json.load(source)
tables = ('SRV6_MY_SOURCE', 'SRV6_MY_LOCATORS', 'SRV6_MY_SIDS')
globals_ = config.get('ISIS_GLOBALS', {})
legacy = (sys.argv[3] == 'legacy' or bool(sys.argv[4]) or
          isinstance(globals_, dict) and any(key != 'default' for key in globals_))
if not legacy:
    tables += ('ISIS_GLOBALS', 'ISIS_GLOBALS_TIMERS', 'ISIS_INTERFACE')
else:
    for row in globals_.values():
        row.pop('srv6_locator', None)
        row.pop('srv6_interface', None)
for name in tables:
    config.pop(name, None)
with open(sys.argv[2], 'w') as output:
    json.dump(config, output, allow_nan=False)
PY
    then
        echo "Failed to prepare the independent FRR startup fallback" >&2
        return 1
    fi
    echo "IS-IS/SRv6 canonical startup unavailable; rendering the independent fallback" >&2
    # Read the captured snapshot, never ConfigDB a second time for fallback.
    sonic-cfggen -j "$fallback" -T /usr/local/sonic/frrcfgd \
        -t /usr/share/sonic/templates/gen_frr.conf.j2,/etc/frr/frr.conf
)

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
        -t /usr/share/sonic/templates/sharpd/sharpd.conf.j2,/etc/frr/sharpd.conf \
    "
    MGMT_FRAMEWORK_CONFIG=$(echo $FRR_VARS | jq -r '.frr_mgmt_framework_config')
    if [ -n "$MGMT_FRAMEWORK_CONFIG" ] && [ "$MGMT_FRAMEWORK_CONFIG" != "false" ]; then
        render_integrated_frr_config || exit 1
        echo "service integrated-vtysh-config" > /etc/frr/vtysh.conf
        rm -f /etc/frr/bgpd.conf /etc/frr/zebra.conf /etc/frr/staticd.conf \
              /etc/frr/bfdd.conf /etc/frr/ospfd.conf /etc/frr/pimd.conf \
              /etc/frr/sharpd.conf
    else
        rm -f /etc/frr/bfdd.conf /etc/frr/ospfd.conf
        sonic-cfggen $CFGGEN_PARAMS
        echo "no service integrated-vtysh-config" > /etc/frr/vtysh.conf
        rm -f /etc/frr/frr.conf
    fi
elif [ "$CONFIG_TYPE" == "split" ]; then
    echo "no service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/frr.conf
    write_default_zebra_config /etc/frr/zebra.conf
elif [ "$CONFIG_TYPE" == "split-unified" ]; then
    echo "service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/bgpd.conf /etc/frr/zebra.conf /etc/frr/staticd.conf \
          /etc/frr/sharpd.conf
    write_default_zebra_config /etc/frr/frr.conf
elif [ "$CONFIG_TYPE" == "unified" ]; then
    render_integrated_frr_config || exit 1
    echo "service integrated-vtysh-config" > /etc/frr/vtysh.conf
    rm -f /etc/frr/bgpd.conf /etc/frr/zebra.conf /etc/frr/staticd.conf \
          /etc/frr/bfdd.conf /etc/frr/ospfd.conf /etc/frr/pimd.conf \
          /etc/frr/sharpd.conf
fi

chown -R frr:frr /etc/frr/

# Create sr0 interface for SRv6 support
if ! ip link show sr0 > /dev/null 2>&1; then
    echo "Interface sr0 does not exist. Creating sr0..."
    ip link add sr0 type dummy || true
else
    echo "Interface sr0 already exists."
fi
ip link set sr0 up || true

chown root:root /usr/sbin/bgp-isolate
chmod 0755 /usr/sbin/bgp-isolate

chown root:root /usr/sbin/bgp-unisolate
chmod 0755 /usr/sbin/bgp-unisolate

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

exec /usr/local/bin/supervisord

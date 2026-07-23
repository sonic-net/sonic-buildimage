#!/usr/bin/env bash

set -e

function fast_reboot {
  case "$(cat /proc/cmdline)" in
    *fast-reboot*)
      if [[ -f /fdb.json ]];
      then
        swssconfig /fdb.json
        mv -f /fdb.json /fdb.json.1
      fi

      if [[ -f /arp.json ]];
      then
        swssconfig /arp.json
        mv -f /arp.json /arp.json.1
      fi

      if [[ -f /default_routes.json ]];
      then
        swssconfig /default_routes.json
        mv -f /default_routes.json /default_routes.json.1
      fi

      if [[ -f /media_config.json ]];
      then
        swssconfig /media_config.json
        mv -f /media_config.json /media_config.json.1
      fi

      ;;
    *)
      ;;
  esac
}

# Wait until swss.sh in the host system create file swss:/ready
until [[ -e /ready ]]; do
    sleep 0.1;
done

rm -f /ready

# Restore FDB and ARP table ASAP
fast_reboot

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

HWSKU=${HWSKU:-`sonic-cfggen -d -v "DEVICE_METADATA['localhost']['hwsku']"`}
sonic_asic_type=$(sonic-cfggen -y /etc/sonic/sonic_version.yml -v asic_type)

# Apply only TUNNEL_DECAP_TABLE entries
apply_ipinip_subset() {
  if [[ -f /etc/swss/config.d/ipinip.json ]]; then
    tmpfile=$(mktemp /tmp/ipinip.XXXX.json)

    jq '[ .[] 
          | select(
                has("TUNNEL_DECAP_TABLE:IPINIP_TUNNEL")
            or has("TUNNEL_DECAP_TABLE:IPINIP_V6_TUNNEL")
            or has("TUNNEL_DECAP_TABLE:IPINIP_SUBNET")
            or has("TUNNEL_DECAP_TABLE:IPINIP_SUBNET_V6")
          )
        ]' /etc/swss/config.d/ipinip.json > "$tmpfile"

    if [[ -s "$tmpfile" ]]; then
        echo "Applying filtered ipinip.json subset from $tmpfile"
        swssconfig "$tmpfile"
    else
        echo "No matching TUNNEL_DECAP_TABLE entries found in ipinip.json"
    fi

    rm -f "$tmpfile"
    sleep 1
  fi
}

# Don't load json config if system warm start or
# swss docker warm start is enabled, the data already exists in appDB.
SYSTEM_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
SWSS_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|swss" enable`
if [[ "$SYSTEM_WARM_START" == "true" ]] || [[ "$SWSS_WARM_START" == "true" ]]; then
    # On warm boot, only apply TUNNEL_DECAP_TABLE subset to match ipinip.json.j2 config,
    # except for broadcom and marvell-teralynx asic_types
    if [[ "$sonic_asic_type" == "broadcom" ]] || [[ "$sonic_asic_type" == "marvell-teralynx" ]]; then
        echo "Skip applying ipinip.json config for ASIC type: $sonic_asic_type after warm-boot"
    else
        echo "Preparing to apply ipinip.json config for ASIC type: $sonic_asic_type"
        apply_ipinip_subset
    fi
    exit 0
fi

SWSSCONFIG_ARGS="ipinip.json ports.json switch.json vxlan.json"

for file in $SWSSCONFIG_ARGS; do
    swssconfig /etc/swss/config.d/$file
    sleep 1
done

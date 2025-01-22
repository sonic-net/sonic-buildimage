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

# Don't load json config if system warm start or
# swss docker warm start is enabled, the data already exists in appDB.
SYSTEM_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
SWSS_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|swss" enable`
if [[ "$SYSTEM_WARM_START" == "true" ]] || [[ "$SWSS_WARM_START" == "true" ]]; then
  exit 0
fi

SWSSCONFIG_ARGS="ipinip.json ports.json switch.json vxlan.json"

for file in $SWSSCONFIG_ARGS; do
    swssconfig /etc/swss/config.d/$file
    sleep 1
done

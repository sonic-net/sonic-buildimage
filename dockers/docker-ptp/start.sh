#!/usr/bin/env bash

PTP_CFGGEN_PARAMS=" \
    -d \
    -y /etc/sonic/constants.yml \
    -t /usr/share/sonic/templates/ptp/ptp4l.conf.j2,/etc/ptp4l/ptp4l.conf \
"

mkdir -p /etc/ptp4l

PTP_VARS=$(sonic-cfggen $PTP_CFGGEN_PARAMS)
# CONFIG_TYPE=$(echo $PTP_VARS | jq -r '.docker_ptp_config_mode')

# if [ "$CONFIG_TYPE" == "disabled" ]; then
#     exit 0
# fi

exec sleep infinity

# do configuration update monitoring?

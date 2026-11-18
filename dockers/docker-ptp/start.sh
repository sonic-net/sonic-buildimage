#!/usr/bin/env bash

EXIT_PTP_CONFIG_FILE_NOT_FOUND=1
PTP_CFG_FILE=/etc/ptp4l/ptp4l.conf

PTP_CFGGEN_PARAMS=" \
    -d \
    -y /etc/sonic/constants.yml \
    -t /usr/share/sonic/templates/ptp/ptp_vars.j2 \
    -t /usr/share/sonic/templates/ptp/ptp4l.conf.j2,${PTP_CFG_FILE} \
"

mkdir -p /etc/ptp4l

PTP_VARS=$(sonic-cfggen $PTP_CFGGEN_PARAMS)
CONFIG_TYPE=$(echo $PTP_VARS | jq -r '.ptp_mgmt_framework_config')

if [ "$CONFIG_TYPE" == "disabled" ]; then
    rm -f ${PTP_CFG_FILE}
else if [ ! -f "$PTP_CFG_FILE" ]; then
    echo "ERROR: PTP4L config file not generated at $PTP_CFG_FILE"
    exit $EXIT_PTP_CONFIG_FILE_NOT_FOUND
fi
fi

# TODO: run monitor configuration update
exec sleep infinity

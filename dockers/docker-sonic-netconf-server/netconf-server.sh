#!/usr/bin/env bash

# Startup script for SONiC Management NETCONF Server
EXIT_MGMT_VARS_FILE_NOT_FOUND=1
MGMT_VARS_FILE=/usr/share/sonic/templates/mgmt_vars.j2

if [ ! -f "$MGMT_VARS_FILE" ]; then
    echo "Mgmt vars template file not found"
    exit $EXIT_MGMT_VARS_FILE_NOT_FOUND
fi

# Read basic server settings from mgmt vars entries
MGMT_VARS=$(sonic-cfggen -d -t $MGMT_VARS_FILE)
MGMT_VARS=${MGMT_VARS//[\']/\"}

NETCONF_SERVER=$(echo $MGMT_VARS | jq -r '.netconf_server')

if [ -n "$NETCONF_SERVER" ]; then
    SERVER_PORT=$(echo $NETCONF_SERVER | jq -r '.port')
    LOG_LEVEL=$(echo $NETCONF_SERVER | jq -r '.log_level')
else
    SERVER_PORT=830
    LOG_LEVEL=5
fi

NETCONF_SERVER_ARGS="-logtostderr"
[ ! -z $SERVER_PORT ] && NETCONF_SERVER_ARGS+=" -port $SERVER_PORT"
[ ! -z $LOG_LEVEL   ] && NETCONF_SERVER_ARGS+=" -v $LOG_LEVEL"

echo "NETCONF_SERVER_ARGS = $NETCONF_SERVER_ARGS"

export CVL_SCHEMA_PATH=/usr/sbin/schema

exec /usr/sbin/netconf_server ${NETCONF_SERVER_ARGS}

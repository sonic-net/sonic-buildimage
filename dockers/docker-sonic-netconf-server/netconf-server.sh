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

NETCONF_SERVER=$(echo "$MGMT_VARS" | jq -r '.netconf_server // empty')

if [ -n "$NETCONF_SERVER" ]; then
    SERVER_PORT=$(echo "$NETCONF_SERVER" | jq -r '.port // empty')
    LOG_LEVEL=$(echo "$NETCONF_SERVER" | jq -r '.log_level // empty')
    HOST_KEY_ALGORITHM=$(echo "$NETCONF_SERVER" | jq -r '.host_key_algorithm // empty')
    HOST_KEY_RSA_BITS=$(echo "$NETCONF_SERVER" | jq -r '.host_key_rsa_bits // empty')
else
    SERVER_PORT=830
    LOG_LEVEL=5
fi

[ -z "$HOST_KEY_ALGORITHM" ] && HOST_KEY_ALGORITHM=ed25519
[ -z "$HOST_KEY_RSA_BITS"  ] && HOST_KEY_RSA_BITS=3072

NETCONF_SERVER_ARGS=("-logtostderr")
[ -n "$SERVER_PORT" ] && NETCONF_SERVER_ARGS+=("-port" "$SERVER_PORT")
[ -n "$LOG_LEVEL"   ] && NETCONF_SERVER_ARGS+=("-v" "$LOG_LEVEL")
[ -n "$HOST_KEY_ALGORITHM" ] && NETCONF_SERVER_ARGS+=("-host_key_algorithm" "$HOST_KEY_ALGORITHM")
[ -n "$HOST_KEY_RSA_BITS"  ] && NETCONF_SERVER_ARGS+=("-host_key_rsa_bits" "$HOST_KEY_RSA_BITS")

echo "NETCONF_SERVER_ARGS = ${NETCONF_SERVER_ARGS[*]}"

export CVL_SCHEMA_PATH=/usr/sbin/schema

exec /usr/sbin/netconf_server "${NETCONF_SERVER_ARGS[@]}"

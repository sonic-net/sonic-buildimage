#!/usr/bin/env bash

EXIT_TELEMETRY_VARS_FILE_NOT_FOUND=1
TELEMETRY_VARS_FILE=/usr/share/sonic/templates/telemetry_vars.j2

if [ ! -f "$TELEMETRY_VARS_FILE" ]; then
    echo "Telemetry vars template file not found"
    exit $EXIT_TELEMETRY_VARS_FILE_NOT_FOUND
fi

# Try to read telemetry and certs config from ConfigDB.
# Use default value if no valid config exists
TELEMETRY_VARS=$(sonic-cfggen -d -t $TELEMETRY_VARS_FILE)
TELEMETRY_VARS=${TELEMETRY_VARS//[\']/\"}
X509=$(echo $TELEMETRY_VARS | jq -r '.x509')
GNMI=$(echo $TELEMETRY_VARS | jq -r '.gnmi')
CERTS=$(echo $TELEMETRY_VARS | jq -r '.certs')

TELEMETRY_ARGS=" -logtostderr"
export CVL_SCHEMA_PATH=/usr/sbin/schema

if [ -n "$CERTS" ]; then
    SERVER_CRT=$(echo $CERTS | jq -r '.server_crt')
    SERVER_KEY=$(echo $CERTS | jq -r '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        TELEMETRY_ARGS+=" --insecure"
    else
        TELEMETRY_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(echo $CERTS | jq -r '.ca_crt')
    if [ ! -z $CA_CRT ]; then
        TELEMETRY_ARGS+=" --ca_crt $CA_CRT"
    fi
elif [ -n "$X509" ]; then
    SERVER_CRT=$(echo $X509 | jq -r '.server_crt')
    SERVER_KEY=$(echo $X509 | jq -r '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        TELEMETRY_ARGS+=" --insecure"
    else
        TELEMETRY_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(echo $X509 | jq -r '.ca_crt')
    if [ ! -z $CA_CRT ]; then
        TELEMETRY_ARGS+=" --ca_crt $CA_CRT"
    fi
else
    TELEMETRY_ARGS+=" --noTLS"
fi

# If no configuration entry exists for TELEMETRY, create one default port

if [ -z "$GNMI" ]; then
    PORT=8080
else
    PORT=$(echo $GNMI | jq -r '.port')
fi

# Jump by 8 to get LoM's port
TELEMETRY_ARGS+=" --port $((PORT + 8))"

CLIENT_AUTH=$(echo $GNMI | jq -r '.client_auth')
if [ -z $CLIENT_AUTH ] || [ $CLIENT_AUTH == "false" ]; then
    TELEMETRY_ARGS+=" --allow_no_client_auth"
fi

LOG_LEVEL=$(echo $GNMI | jq -r '.log_level')
if [[ $LOG_LEVEL =~ ^[0-9]+$ ]]; then
    TELEMETRY_ARGS+=" -v=$LOG_LEVEL"
else
    TELEMETRY_ARGS+=" -v=2"
fi

echo "Start /usr/bin/LoMgNMIServer ${TELEMETRY_ARGS}"
exec /usr/bin/LoMgNMIServer ${TELEMETRY_ARGS}

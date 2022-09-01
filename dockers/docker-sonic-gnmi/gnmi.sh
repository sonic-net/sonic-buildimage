#!/usr/bin/env bash

EXIT_GNXI_VARS_FILE_NOT_FOUND=1
GNXI_VARS_FILE=/usr/share/sonic/templates/gnmi_vars.j2

if [ ! -f "$GNXI_VARS_FILE" ]; then
    echo "GNMI vars template file not found"
    exit $EXIT_GNXI_VARS_FILE_NOT_FOUND
fi

# Try to read gnxi and certs config from ConfigDB.
# Use default value if no valid config exists
GNXI_VARS=$(sonic-cfggen -d -t $GNXI_VARS_FILE)
GNXI_VARS=${GNXI_VARS//[\']/\"}
X509=$(echo $GNXI_VARS | jq -r '.x509')
GNMI=$(echo $GNXI_VARS | jq -r '.gnmi')
CERTS=$(echo $GNXI_VARS | jq -r '.certs')

GNXI_ARGS=" -logtostderr"
export CVL_SCHEMA_PATH=/usr/sbin/schema

if [ -n "$CERTS" ]; then
    SERVER_CRT=$(echo $CERTS | jq -r '.server_crt')
    SERVER_KEY=$(echo $CERTS | jq -r '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        GNXI_ARGS+=" --insecure"
    else
        GNXI_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(echo $CERTS | jq -r '.ca_crt')
    if [ ! -z $CA_CRT ]; then
        GNXI_ARGS+=" --ca_crt $CA_CRT"
    fi
elif [ -n "$X509" ]; then
    SERVER_CRT=$(echo $X509 | jq -r '.server_crt')
    SERVER_KEY=$(echo $X509 | jq -r '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        GNXI_ARGS+=" --insecure"
    else
        GNXI_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(echo $X509 | jq -r '.ca_crt')
    if [ ! -z $CA_CRT ]; then
        GNXI_ARGS+=" --ca_crt $CA_CRT"
    fi
else
    GNXI_ARGS+=" --noTLS"
fi

# If no configuration entry exists for GNXI, create one default port
if [ -z "$GNMI" ]; then
    PORT=8080
else
    PORT=$(echo $GNMI | jq -r '.port')
fi
GNXI_ARGS+=" --port $PORT"

CLIENT_AUTH=$(echo $GNMI | jq -r '.client_auth')
if [ -z $CLIENT_AUTH ] || [ $CLIENT_AUTH == "false" ]; then
    GNXI_ARGS+=" --allow_no_client_auth"
fi

LOG_LEVEL=$(echo $GNMI | jq -r '.log_level')
if [ ! -z $LOG_LEVEL ]; then
    GNXI_ARGS+=" -v=$LOG_LEVEL"
else
    GNXI_ARGS+=" -v=2"
fi

exec /usr/sbin/gnxi ${GNXI_ARGS}

#!/usr/bin/env bash

# Startup script for SONiC Management REST Server
EXIT_MGMT_VARS_FILE_NOT_FOUND=1
MGMT_VARS_FILE=/usr/share/sonic/templates/mgmt_vars.j2

if [ ! -f "$MGMT_VARS_FILE" ]; then
    echo "Mgmt vars template file not found"
    exit $EXIT_MGMT_VARS_FILE_NOT_FOUND
fi

# Read basic server settings from mgmt vars entries
MGMT_VARS=$(sonic-cfggen -d -t $MGMT_VARS_FILE)
MGMT_VARS=${MGMT_VARS//[\']/\"}

REST_SERVER=$(echo $MGMT_VARS | jq -r '.rest_server')

if [ -n "$REST_SERVER" ]; then
    SERVER_PORT=$(echo $REST_SERVER | jq -r '.port // empty')
    CLIENT_AUTH=$(echo $REST_SERVER | jq -r '.client_auth // empty')
    LOG_LEVEL=$(echo $REST_SERVER | jq -r '.log_level // empty')

    SERVER_CRT=$(echo $REST_SERVER | jq -r '.server_crt // empty')
    SERVER_KEY=$(echo $REST_SERVER | jq -r '.server_key // empty')
    CA_CRT=$(echo $REST_SERVER | jq -r '.ca_crt // empty')
fi

if [[ -z $SERVER_CRT ]] && [[ -z $SERVER_KEY ]] && [[ -z $CA_CRT ]]; then
    X509=$(echo $MGMT_VARS | jq -r '.x509')
fi

# Read certificate file paths from DEVICE_METADATA|x509 entry.
if [ -n "$X509" ]; then
    SERVER_CRT=$(echo $X509 | jq -r '.server_crt // empty')
    SERVER_KEY=$(echo $X509 | jq -r '.server_key // empty')
    CA_CRT=$(echo $X509 | jq -r '.ca_crt // empty')
fi

# Create temporary server certificate if they not configured in ConfigDB
if [ -z $SERVER_CRT ] && [ -z $SERVER_KEY ]; then
    echo "Generating temporary TLS server certificate ..."
    (cd /tmp && /usr/sbin/generate_cert --host="localhost,127.0.0.1")
    SERVER_CRT=/tmp/cert.pem
    SERVER_KEY=/tmp/key.pem
fi


REST_SERVER_ARGS="-ui /rest_ui -logtostderr"
[ ! -z $SERVER_PORT ] && REST_SERVER_ARGS+=" -port $SERVER_PORT"
[ ! -z $LOG_LEVEL   ] && REST_SERVER_ARGS+=" -v $LOG_LEVEL"
[ ! -z $CLIENT_AUTH ] && REST_SERVER_ARGS+=" -client_auth $CLIENT_AUTH"
[ ! -z $SERVER_CRT  ] && REST_SERVER_ARGS+=" -cert $SERVER_CRT"
[ ! -z $SERVER_KEY  ] && REST_SERVER_ARGS+=" -key $SERVER_KEY"
[ ! -z $CA_CRT      ] && REST_SERVER_ARGS+=" -cacert $CA_CRT"

echo "REST_SERVER_ARGS = $REST_SERVER_ARGS"


export CVL_SCHEMA_PATH=/usr/sbin/schema

exec /usr/sbin/rest_server ${REST_SERVER_ARGS}

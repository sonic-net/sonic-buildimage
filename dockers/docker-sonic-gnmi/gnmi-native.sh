#!/usr/bin/env bash

EXIT_TELEMETRY_VARS_FILE_NOT_FOUND=1
INCORRECT_TELEMETRY_VALUE=2
TELEMETRY_VARS_FILE=/usr/share/sonic/templates/telemetry_vars.j2
ESCAPE_QUOTE="'\''"

extract_field() {
    echo $(echo $1 | jq -r $2)
}

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

# Enable GRPC GO LOG
export GRPC_GO_LOG_VERBOSITY_LEVEL=99
export GRPC_GO_LOG_SEVERITY_LEVEL=info

TELEMETRY_ARGS=" -logtostderr"
export CVL_SCHEMA_PATH=/usr/sbin/schema

if [ -n "$CERTS" ]; then
    SERVER_CRT=$(extract_field "$CERTS" '.server_crt')
    SERVER_KEY=$(extract_field "$CERTS" '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        TELEMETRY_ARGS+=" --insecure"
    else
        TELEMETRY_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(extract_field "$CERTS" '.ca_crt')
    if [ ! -z $CA_CRT ]; then
        TELEMETRY_ARGS+=" --ca_crt $CA_CRT"
    fi

elif [ -n "$X509" ]; then
    SERVER_CRT=$(extract_field "$X509" '.server_crt')
    SERVER_KEY=$(extract_field "$X509" '.server_key')
    if [ -z $SERVER_CRT  ] || [ -z $SERVER_KEY  ]; then
        TELEMETRY_ARGS+=" --insecure"
    else
        TELEMETRY_ARGS+=" --server_crt $SERVER_CRT --server_key $SERVER_KEY "
    fi

    CA_CRT=$(extract_field "$X509" '.ca_crt')
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
    PORT=$(extract_field "$GNMI" '.port')
    if ! [[ $PORT =~ ^[0-9]+$ ]]; then
        echo "Incorrect port value ${PORT}, expecting positive integers" >&2
        exit $INCORRECT_TELEMETRY_VALUE
    fi
fi

TELEMETRY_ARGS+=" --port $PORT"

CLIENT_AUTH=$(extract_field "$GNMI" '.client_auth')
if [ -z $CLIENT_AUTH ] || [ $CLIENT_AUTH == "false" ]; then
    TELEMETRY_ARGS+=" --allow_no_client_auth"
fi

LOG_LEVEL=$(extract_field "$GNMI" '.log_level')
if [[ $LOG_LEVEL =~ ^[0-9]+$ ]]; then
    TELEMETRY_ARGS+=" -v=$LOG_LEVEL"
else
    TELEMETRY_ARGS+=" -v=2"
fi

# Enable ZMQ for SmartSwitch
LOCALHOST_SUBTYPE=`sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "subtype"`
if [[ x"${LOCALHOST_SUBTYPE}" == x"SmartSwitch" ]]; then
    TELEMETRY_ARGS+=" -zmq_port=8100"
fi

# Add VRF parameter when mgmt-vrf enabled
MGMT_VRF_ENABLED=`sonic-db-cli CONFIG_DB hget  "MGMT_VRF_CONFIG|vrf_global" "mgmtVrfEnabled"`
if [[ x"${MGMT_VRF_ENABLED}" == x"true" ]]; then
    TELEMETRY_ARGS+=" --vrf mgmt"
fi

# Server will handle threshold connections consecutively
THRESHOLD_CONNECTIONS=$(extract_field "$GNMI" '.threshold')
if [[ $THRESHOLD_CONNECTIONS =~ ^[0-9]+$ ]]; then
    TELEMETRY_ARGS+=" --threshold $THRESHOLD_CONNECTIONS"
else
    if [ -z "$GNMI" ] || [[ $THRESHOLD_CONNECTIONS == "null" ]]; then
        TELEMETRY_ARGS+=" --threshold 100"
    else
        echo "Incorrect threshold value, expecting positive integers" >&2
        exit $INCORRECT_TELEMETRY_VALUE
    fi
fi

# Close idle connections after certain duration (in seconds)
IDLE_CONN_DURATION=$(extract_field "$GNMI" '.idle_conn_duration')
if [[ $IDLE_CONN_DURATION =~ ^[0-9]+$ ]]; then
    TELEMETRY_ARGS+=" --idle_conn_duration $IDLE_CONN_DURATION"
else
    if [ -z "$GNMI" ] || [[ $IDLE_CONN_DURATION == "null" ]]; then
        TELEMETRY_ARGS+=" --idle_conn_duration 5"
    else
        echo "Incorrect idle_conn_duration value, expecting positive integers" >&2
        exit $INCORRECT_TELEMETRY_VALUE
    fi
fi

USER_AUTH=$(extract_field "$GNMI" '.user_auth')
# If user_auth is not set, default to certs
if [ $USER_AUTH == "null" ]; then
    USER_AUTH="cert"
fi
if [ ! -z "$USER_AUTH" ] && [  $USER_AUTH != "null" ] && [  $USER_AUTH != "none" ]; then
    TELEMETRY_ARGS+=" --client_auth $USER_AUTH"

    if [ $USER_AUTH == "cert" ]; then
        TELEMETRY_ARGS+=" --config_table_name GNMI_CLIENT_CERT"

        ENABLE_CRL=$(echo $GNMI | jq -r '.enable_crl')
        if [ $ENABLE_CRL == "true" ]; then
            TELEMETRY_ARGS+=" --enable_crl"
        fi

        CRL_EXPIRE_DURATION=$(extract_field "$GNMI" '.crl_expire_duration')
        if [ ! -z "$CRL_EXPIRE_DURATION" ] && [ $CRL_EXPIRE_DURATION != "null" ]; then
            TELEMETRY_ARGS+=" --crl_expire_duration $CRL_EXPIRE_DURATION"
        fi
    fi
fi

echo "gnmi args: $TELEMETRY_ARGS"
exec /usr/sbin/telemetry ${TELEMETRY_ARGS}

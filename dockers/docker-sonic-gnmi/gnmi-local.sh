#!/usr/bin/env bash

# gnmi-local: Insecure gNMI server on Unix Domain Socket
# This allows local clients (e.g., device-ops-agent) to connect without TLS

# Socket in mounted directory so it's accessible from host
GNMI_SOCKET="/var/run/gnmi/gnmi.sock"

# Ensure directory exists
mkdir -p /var/run/gnmi

# UDS mode: set port to -1 so --unix_socket takes effect
# No TLS, no client auth for local access
TELEMETRY_ARGS=" -logtostderr"
TELEMETRY_ARGS+=" --port -1"
TELEMETRY_ARGS+=" --noTLS"
TELEMETRY_ARGS+=" --unix_socket $GNMI_SOCKET"
TELEMETRY_ARGS+=" --allow_no_client_auth"
TELEMETRY_ARGS+=" -v=2"

export CVL_SCHEMA_PATH=/usr/sbin/schema

echo "gnmi-local args: $TELEMETRY_ARGS"
exec /usr/sbin/telemetry ${TELEMETRY_ARGS}

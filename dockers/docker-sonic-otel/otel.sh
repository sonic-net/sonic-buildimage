#!/usr/bin/env bash

EXIT_OTEL_VARS_FILE_NOT_FOUND=1
OTEL_VARS_FILE=/usr/share/sonic/templates/otel_vars.j2
CONFIG_FILE=/etc/otel-collector/config.yaml

if [ ! -f "$OTEL_VARS_FILE" ]; then
    echo "OTEL vars template file not found"
    exit $EXIT_OTEL_VARS_FILE_NOT_FOUND
fi

# Read telemetry from ConfigDB
echo "Generating OTEL config from template..."
OTEL_CONFIG=$(sonic-cfggen -d -t $OTEL_VARS_FILE)

if [ $? -ne 0 ] || [ -z "$OTEL_CONFIG" ]; then
    echo "Failed to generate config from template, this should not happen"
    exit 1
fi

echo "$OTEL_CONFIG" > $CONFIG_FILE

OTEL_ARGS="--config=$CONFIG_FILE"

echo "otel collector args: $OTEL_ARGS"
echo "Starting OTEL Collector with config file: $CONFIG_FILE"

exec /usr/bin/otelcol-contrib ${OTEL_ARGS}
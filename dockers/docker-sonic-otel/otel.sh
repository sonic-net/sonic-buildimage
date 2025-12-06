#!/usr/bin/env bash

EXIT_OTEL_CONFIG_FILE_NOT_FOUND=1
OTEL_CONFIG_FILE=/etc/otel-collector/otel_config.yaml
CONFIG_FILE=/etc/otel-collector/config.yaml

echo "Starting otel.sh script"
echo "Checking for config file: $OTEL_CONFIG_FILE"

if [ ! -f "$OTEL_CONFIG_FILE" ]; then
    echo "ERROR: OTEL config file not found at $OTEL_CONFIG_FILE"
    ls -la /etc/otel-collector/
    exit $EXIT_OTEL_CONFIG_FILE_NOT_FOUND
fi

echo "Config file found, copying to runtime location..."
cp "$OTEL_CONFIG_FILE" "$CONFIG_FILE"

echo "Config file contents:"
cat $CONFIG_FILE

# Validate the YAML configuration
echo "Validating YAML configuration..."
python3 -c "import yaml; yaml.safe_load(open('$CONFIG_FILE'))" 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: YAML configuration is invalid"
    exit 1
fi

OTEL_ARGS="--config=$CONFIG_FILE"

echo "Checking otelcol-contrib binary"
ls -la /usr/local/bin/otelcol-contrib

if [ ! -x "/usr/local/bin/otelcol-contrib" ]; then
    echo "ERROR: otelcol-contrib binary not found or not executable"
    exit 1
fi

echo "otel collector args: $OTEL_ARGS"
echo "Starting OTEL Collector with config file: $CONFIG_FILE"

exec /usr/local/bin/otelcol-contrib ${OTEL_ARGS}

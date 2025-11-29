#!/usr/bin/env bash

EXIT_OTEL_VARS_FILE_NOT_FOUND=1
OTEL_VARS_FILE=/usr/share/sonic/templates/otel_vars.j2
CONFIG_FILE=/etc/otel-collector/config.yaml

echo "Starting otel.sh script"
echo "Checking for template file: $OTEL_VARS_FILE"

if [ ! -f "$OTEL_VARS_FILE" ]; then
    echo "ERROR: OTEL vars template file not found at $OTEL_VARS_FILE"
    ls -la /usr/share/sonic/templates/
    exit $EXIT_OTEL_VARS_FILE_NOT_FOUND
fi

echo "Template file found, generating OTEL config from template..."
OTEL_CONFIG=$(sonic-cfggen -d -t $OTEL_VARS_FILE 2>&1)
CFGGEN_EXIT_CODE=$?

echo "sonic-cfggen exit code: $CFGGEN_EXIT_CODE"
echo "Generated config length: ${#OTEL_CONFIG}"

if [ $CFGGEN_EXIT_CODE -ne 0 ]; then
    echo "ERROR: sonic-cfggen failed with exit code $CFGGEN_EXIT_CODE"
    echo "Output: $OTEL_CONFIG"
    exit 1
fi

if [ -z "$OTEL_CONFIG" ]; then
    echo "ERROR: Generated config is empty"
    exit 1
fi

# Ensure config directory exists
echo "Creating config directory"
mkdir -p /etc/otel-collector

echo "Writing config to $CONFIG_FILE"
echo "$OTEL_CONFIG" > $CONFIG_FILE

echo "Config file contents:"
cat $CONFIG_FILE

# Validate the generated YAML
echo "Validating YAML configuration..."
python3 -c "import yaml; yaml.safe_load(open('$CONFIG_FILE'))" 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Generated YAML is invalid"
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

# Test the config before starting
echo "Testing configuration..."
/usr/local/bin/otelcol-contrib --config=$CONFIG_FILE --dry-run 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Configuration test failed"
    exit 1
fi

exec /usr/local/bin/otelcol-contrib ${OTEL_ARGS}

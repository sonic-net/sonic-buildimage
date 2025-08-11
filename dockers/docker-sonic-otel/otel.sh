#!/usr/bin/env bash

EXIT_OTEL_VARS_FILE_NOT_FOUND=1
INCORRECT_OTEL_VALUE=2
OTEL_VARS_FILE=/usr/share/sonic/templates/otel_vars.j2
CONFIG_FILE=/etc/otel-collector/config.yaml

extract_field() {
    echo $(echo $1 | jq -r $2)
}

if [ ! -f "$OTEL_VARS_FILE" ]; then
    echo "OTEL vars template file not found, using default config"
    # Use your exact local configuration as default
    cat > $CONFIG_FILE << 'EOF'
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: "0.0.0.0:4317"
      http:
        endpoint: "0.0.0.0:4318"

processors:
  batch:
    timeout: 1s
    send_batch_size: 10

  filter/ports:
    metrics:
      include:
        match_type: regexp
        metric_names:
          - port\..*

  filter/buffers:
    metrics:
      include:
        match_type: regexp
        metric_names:
          - buffer_pool\..*

exporters:
  influxdb/ports:
    endpoint: http://influxdb-a:8086
    org: docs
    bucket: ports
    token: mytoken123456789
    timeout: 10s

  influxdb/buffers:
    endpoint: http://influxdb-b:8086
    org: docs
    bucket: buffers
    token: mytoken123456789
    timeout: 10s

  debug:
    verbosity: detailed

service:
  telemetry:
    logs:
      level: "debug"
  pipelines:
    metrics/ports:
      receivers: [otlp]
      processors: [batch, filter/ports]
      exporters: [debug, influxdb/ports]
    metrics/buffers:
      receivers: [otlp]
      processors: [batch, filter/buffers]
      exporters: [debug, influxdb/buffers]
    logs:
      receivers: [otlp]
      processors: [batch]
      exporters: [debug]
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [debug]
EOF
else
    # Generate config from ConfigDB template
    OTEL_VARS=$(sonic-cfggen -d -t $OTEL_VARS_FILE)
    echo "$OTEL_VARS" > $CONFIG_FILE
fi

OTEL_ARGS="--config=$CONFIG_FILE"

echo "otel collector args: $OTEL_ARGS"
echo "Starting OTEL Collector with config file: $CONFIG_FILE"

exec /usr/bin/otelcol-contrib ${OTEL_ARGS}
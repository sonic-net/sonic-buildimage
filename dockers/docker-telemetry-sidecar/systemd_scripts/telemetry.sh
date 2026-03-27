#!/bin/bash
# Thin wrapper for telemetry pod control - uses sidecar-specific k8s_pod_control.sh
exec /usr/share/sonic/scripts/docker-telemetry-sidecar/k8s_pod_control.sh telemetry "$@"

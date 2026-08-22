#!/bin/bash
# Thin wrapper for otel pod control - uses sidecar-specific k8s_pod_control.sh
exec /usr/share/sonic/scripts/docker-sonic-otel-sidecar/k8s_pod_control.sh otel "$@"

#!/bin/bash
# Thin wrapper for GNMI pod control - uses sidecar-specific k8s_pod_control.sh
exec /usr/share/sonic/scripts/docker-gnmi-sidecar/k8s_pod_control.sh gnmi "$@"

#!/bin/bash
# Thin wrapper for telemetry pod control - uses shared k8s_pod_control.sh
exec /usr/share/sonic/scripts/k8s_pod_control.sh telemetry "$@"

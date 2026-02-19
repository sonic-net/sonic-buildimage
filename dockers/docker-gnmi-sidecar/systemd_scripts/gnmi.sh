#!/bin/bash
# Thin wrapper for GNMI pod control - uses shared k8s_pod_control.sh
exec /usr/share/sonic/scripts/k8s_pod_control.sh gnmi "$@"

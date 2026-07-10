#!/bin/bash
# Thin wrapper for restapi pod control - uses sidecar-specific k8s_pod_control.sh
exec /usr/share/sonic/scripts/docker-restapi-sidecar/k8s_pod_control.sh restapi "$@"

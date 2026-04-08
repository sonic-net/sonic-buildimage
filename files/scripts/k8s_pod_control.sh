#!/bin/bash
# Shared Kubernetes pod control script for SONiC sidecar services
# 1. Discovers K8s-managed containers via 'docker ps' name filtering
# 2. Uses 'docker restart' to restart the target container
# 3. K8s container names follow the pattern k8s_<container>_<pod>_<ns>_<uid>
#
# Usage: SERVICE_NAME=telemetry k8s_pod_control.sh start
#        Or source this script after setting SERVICE_NAME

set -euo pipefail

# SERVICE_NAME can be provided as first argument or environment variable
if [[ -n "${1:-}" ]] && [[ "${1}" != "start" ]] && [[ "${1}" != "stop" ]] && [[ "${1}" != "restart" ]] && [[ "${1}" != "wait" ]] && [[ "${1}" != "status" ]]; then
  SERVICE_NAME="$1"
  shift
fi

# SERVICE_NAME must be set by caller (e.g., "telemetry", "restapi")
if [[ -z "${SERVICE_NAME:-}" ]]; then
  echo "ERROR: SERVICE_NAME must be provided as first argument or environment variable" >&2
  exit 1
fi

NS="sonic"

NODE_NAME="$(hostname | tr '[:upper:]' '[:lower:]')"
log() { /usr/bin/logger -t "k8s-podctl#system" "$*"; }

# Docker filter for K8s-managed containers of this service.
# K8s container names follow: k8s_<container>_<pod>_<ns>_<uid>
DOCKER_FILTER="name=k8s_${SERVICE_NAME}_"

container_ids_on_node() {
  docker ps -q --filter "${DOCKER_FILTER}" 2>/dev/null || true
}

pods_on_node() {
  docker ps -a --filter "${DOCKER_FILTER}" \
    --format '{{index .Labels "io.kubernetes.pod.name"}} {{.State}}' \
    2>/dev/null || true
}

pod_names_on_node() {
  docker ps -a --filter "${DOCKER_FILTER}" \
    --format '{{index .Labels "io.kubernetes.pod.name"}}' \
    2>/dev/null || true
}

restart_containers() {
  mapfile -t cids < <(container_ids_on_node)
  if (( ${#cids[@]} == 0 )); then
    log "No containers found for '${SERVICE_NAME}' on ${NODE_NAME} (ns=${NS})."
    return 0
  fi

  log "Restarting containers for '${SERVICE_NAME}' on ${NODE_NAME}: ${cids[*]}"

  local rc_any=0
  for cid in "${cids[@]}"; do
    [[ -z "$cid" ]] && continue
    if docker restart "$cid" >/dev/null 2>&1; then
      log "Restarted container ${cid} (${SERVICE_NAME})"
    else
      log "ERROR: failed to restart container ${cid} (${SERVICE_NAME})"
      rc_any=1
    fi
  done

  if (( rc_any != 0 )); then
    log "ERROR one or more container restarts failed for '${SERVICE_NAME}' on ${NODE_NAME}"
  else
    log "All containers restarted for '${SERVICE_NAME}' on ${NODE_NAME}"
  fi
  return "$rc_any"
}

cmd_start() {
  # Re-invoke ourselves with the "restart" action under a hard 20s cap.
  # On a healthy node this finishes in 1-2s; the timeout guards against
  # a hung 'docker restart' so we stay within TimeoutStartSec (30s).
  timeout 20 "${BASH_SOURCE[0]}" "${SERVICE_NAME}" restart 2>&1 \
    | logger -t "${SERVICE_NAME}-start" || true
}

cmd_stop()    { restart_containers; }
cmd_restart() { restart_containers; }

cmd_status() {
  local out=""; out="$(pods_on_node)"
  if [[ -z "$out" ]]; then
    echo "NOT RUNNING (no container on node ${NODE_NAME} for '${SERVICE_NAME}')"
    exit 3
  fi
  while read -r name state; do
    [[ -z "$name" ]] && continue
    echo "pod ${name}: ${state}"
  done <<<"$out"
  if awk '$2=="running"{found=1} END{exit found?0:1}' <<<"$out"; then
    exit 0
  else
    exit 1
  fi
}

cmd_wait() {
  # No-op: just sleep forever so the systemd unit stays "active".
  log "cmd_wait: sleeping indefinitely for '${SERVICE_NAME}' on ${NODE_NAME}"
  while true; do sleep 300; done
}

case "${1:-}" in
  start)   cmd_start ;;
  stop)    cmd_stop ;;
  restart) cmd_restart ;;
  wait)    cmd_wait ;;
  status)  cmd_status ;;
  *)
    echo "Usage: $0 {start|stop|restart|wait|status}" >&2
    exit 2
    ;;
esac

#!/bin/bash
# Shared Kubernetes pod control script for SONiC sidecar services
# 1. Discovers K8s-managed containers via 'docker ps' label filtering
#    (exact match on io.kubernetes.container.name + io.kubernetes.pod.namespace,
#    which are injected by the dockershim/CRI on every K8s-managed container).
# 2. Uses 'docker stop' to stop the target container and leaves the start to
#    kubelet, which replaces a stopped container with a new attempt.
#
# Never 'docker start' / 'docker restart' a kubelet-managed container. kubelet
# polls the runtime once a second; if it observes the container stopped it
# creates the next attempt, and a 'docker restart' then brings the old attempt
# back as well. kubelet only ever reconciles the newest attempt per container
# name and its garbage collector only removes stopped containers, so the old
# attempt keeps running unmanaged (no probes, no restarts) until the next
# reboot. Stopping is safe: a stopped container is exactly what kubelet is
# built to replace.
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

# Docker label filters for K8s-managed containers of this service.
# Using labels (rather than name= substring matching) gives an EXACT match on
# the container name, so e.g. SERVICE_NAME=telemetry will not also match a
# hypothetical 'telemetry_v2' container, and is independent of the
# k8s_<container>_<pod>_<ns>_<uid> dockershim naming convention.
DOCKER_FILTERS=(
  --filter "label=io.kubernetes.container.name=${SERVICE_NAME}"
  --filter "label=io.kubernetes.pod.namespace=${NS}"
)

container_ids_on_node() {
  docker ps -q "${DOCKER_FILTERS[@]}" 2>/dev/null || true
}

pods_on_node() {
  docker ps -a "${DOCKER_FILTERS[@]}" \
    --format '{{index .Labels "io.kubernetes.pod.name"}} {{.State}}' \
    2>/dev/null || true
}

# Seconds 'docker stop' waits for the process to exit on SIGTERM before SIGKILL.
STOP_TIMEOUT="${STOP_TIMEOUT:-10}"
# Seconds cmd_start waits for kubelet to have a running attempt before giving up
# (informational only; kubelet keeps trying regardless).
START_WAIT="${START_WAIT:-20}"

stop_containers() {
  mapfile -t cids < <(container_ids_on_node)
  if (( ${#cids[@]} == 0 )); then
    log "No containers found for '${SERVICE_NAME}' on ${NODE_NAME} (ns=${NS})."
    return 0
  fi

  log "Stopping containers for '${SERVICE_NAME}' on ${NODE_NAME} (kubelet will start the replacement): ${cids[*]}"

  local rc_any=0
  for cid in "${cids[@]}"; do
    [[ -z "$cid" ]] && continue
    if docker stop -t "${STOP_TIMEOUT}" "$cid" >/dev/null 2>&1; then
      log "Stopped container ${cid} (${SERVICE_NAME})"
    else
      log "ERROR: failed to stop container ${cid} (${SERVICE_NAME})"
      rc_any=1
    fi
  done

  if (( rc_any != 0 )); then
    log "ERROR one or more container stops failed for '${SERVICE_NAME}' on ${NODE_NAME}"
  else
    log "All containers stopped for '${SERVICE_NAME}' on ${NODE_NAME}; kubelet owns the restart"
  fi
  return "$rc_any"
}

cmd_start() {
  # kubelet owns starting the container. Only wait (bounded) for a running
  # attempt so 'systemctl start' reflects reality; never docker start/restart.
  local i
  for (( i = 0; i < START_WAIT; i++ )); do
    if [[ -n "$(container_ids_on_node)" ]]; then
      log "'${SERVICE_NAME}' container running on ${NODE_NAME}"
      return 0
    fi
    sleep 1
  done
  log "WARNING: no running '${SERVICE_NAME}' container on ${NODE_NAME} after ${START_WAIT}s; kubelet has not started it yet"
  return 0
}

cmd_stop()    { stop_containers; }
cmd_restart() { stop_containers; }

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

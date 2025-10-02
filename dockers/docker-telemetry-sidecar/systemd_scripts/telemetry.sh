#!/bin/bash
# 1. Uses sudo -n because the systemd service is run as admin user, and kubelet.conf is accessible via root
# 2. Use kubectl to get pods and delete pods with retry
# 3. start/stop/restart are NON-BLOCKING

set -euo pipefail

NS="sonic"
KUBECTL_BIN="/usr/bin/kubectl"
KCF=(--kubeconfig=/etc/kubernetes/kubelet.conf)
REQ_TIMEOUT="5s"
MAX_ATTEMPTS=10
BACKOFF_START=1
BACKOFF_MAX=8

NODE_NAME="$(hostname | tr '[:upper:]' '[:lower:]')"
log() { /usr/bin/logger -t "k8s-podctl#system" "$*"; }

kubectl_retry() {
  local attempt=1 backoff=${BACKOFF_START} out rc
  while true; do
    out="$("${KUBECTL_BIN}" "${KCF[@]}" --request-timeout="${REQ_TIMEOUT}" "$@" 2>&1)"; rc=$?
    if (( rc == 0 )); then
      printf '%s' "$out"
      return 0
    fi
    if (( attempt >= MAX_ATTEMPTS )); then
      echo "$out" >&2
      return "$rc"
    fi
    log "kubectl retry ${attempt}/${MAX_ATTEMPTS} for: $*"
    sleep "${backoff}"
    (( backoff = backoff < BACKOFF_MAX ? backoff*2 : BACKOFF_MAX ))
    (( attempt++ ))
  done
}

pods_on_node() {
  kubectl_retry -n "${NS}" get pods \
    --field-selector "spec.nodeName=${NODE_NAME}" \
    -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.status.phase}{"\n"}{end}' || true
}

pod_names_on_node() {
  kubectl_retry -n "${NS}" get pods \
    --field-selector "spec.nodeName=${NODE_NAME}" \
    -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' || true
}

delete_pod_with_retry() {
  local name="$1"
  kubectl_retry -n "${NS}" delete pod "${name}" --force --grace-period=0 --wait=false
}

kill_pods() {
  mapfile -t names < <(pod_names_on_node)
  if (( ${#names[@]} == 0 )); then
    log "No pods found on ${NODE_NAME} (ns=${NS})."
    return 0
  fi
  log "Deleting pods on ${NODE_NAME}: ${names[*]}"
  local rc=0
  for p in "${names[@]}"; do
    [[ -n "$p" ]] && delete_pod_with_retry "$p" || rc=1
  done
  return "$rc"
}

cmd_start()   { kill_pods; }     # start == kill (DS restarts)
cmd_stop()    { kill_pods; }
cmd_restart() { kill_pods; }

cmd_status() {
  local out=""; out="$(pods_on_node)"
  if [[ -z "$out" ]]; then
    echo "NOT RUNNING (no pod on node ${NODE_NAME})"
    exit 3
  fi
  while read -r name phase; do
    [[ -z "$name" ]] && continue
    echo "pod ${name}: ${phase}"
  done <<<"$out"
  if awk '$2=="Running"{found=1} END{exit found?0:1}' <<<"$out"; then
    exit 0
  else
    exit 1
  fi
}

cmd_wait() {
  log "Waiting on pods (ns=${NS}) on node ${NODE_NAME})…"
  while true; do
    local out=""; out="$(pods_on_node)"
    if [[ -z "$out" ]]; then
      sleep 5; continue
    fi
    if awk '$2=="Running"{found=1} END{exit found?0:1}' <<<"$out"; then
      sleep 60
    else
      sleep 5
    fi
  done
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

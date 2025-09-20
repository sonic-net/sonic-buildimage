#!/bin/bash
set -euo pipefail

SERVICE="telemetry"
NS="${NS:-sonic}"                               # k8s namespace
LABEL="raw_container_name=${SERVICE}"           # selector used by DaemonSet
KUBECTL_BIN="${KUBECTL_BIN:-kubectl}"
NODE_NAME="${NODE_NAME:-$(hostname)}"
DEV="${2:-}"                                    # accepted for compatibility; unused (single-ASIC)

log() { /usr/bin/logger -t "${SERVICE}#system" "$*"; }

require_kubectl() {
  if ! command -v "${KUBECTL_BIN}" >/dev/null 2>&1; then
    echo "ERROR: kubectl not found (KUBECTL_BIN=${KUBECTL_BIN})." >&2
    exit 127
  fi
  # Try a sensible default if KUBECONFIG isn’t set
  if [[ -z "${KUBECONFIG:-}" && -r /etc/kubernetes/kubelet.conf ]]; then
    export KUBECONFIG=/etc/kubernetes/kubelet.conf
  fi
}

pods_on_node() {
  # Prints: "<name> <phase>" per line for this node
  "${KUBECTL_BIN}" -n "${NS}" get pods \
    -l "${LABEL}" \
    --field-selector "spec.nodeName=${NODE_NAME}" \
    -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.status.phase}{"\n"}{end}' 2>/dev/null || true
}

kill_pods() {
  require_kubectl
  local found=0
  while read -r name phase; do
    [[ -z "${name}" ]] && continue
    found=1
    log "Deleting ${SERVICE} pod ${name} (phase=${phase}) on node ${NODE_NAME}"
    # Force/instant delete to emulate “kill”; DaemonSet will recreate
    "${KUBECTL_BIN}" -n "${NS}" delete pod "${name}" --grace-period=0 --force >/dev/null 2>&1 || true
  done < <(pods_on_node)
  if [[ "${found}" -eq 0 ]]; then
    log "No ${SERVICE} pods found on node ${NODE_NAME} (namespace=${NS}, label=${LABEL})."
  fi
}

cmd_start()   { kill_pods; }     # start == kill (DS restarts)
cmd_stop()    { kill_pods; }
cmd_restart() { kill_pods; }

cmd_status() {
  require_kubectl
  local out; out="$(pods_on_node)"
  if [[ -z "${out}" ]]; then
    echo "${SERVICE}: NOT RUNNING (no pod on node ${NODE_NAME})"
    exit 3
  fi
  echo "${out}" | while read -r name phase; do
    [[ -z "${name}" ]] && continue
    echo "${SERVICE} pod ${name}: ${phase}"
  done
  # Exit 0 if at least one Running, 1 otherwise
  if echo "${out}" | awk '$2=="Running"{found=1} END{exit found?0:1}'; then
    exit 0
  else
    exit 1
  fi
}

cmd_wait() {
  require_kubectl
  log "Waiting on ${SERVICE} pods (ns=${NS}, label=${LABEL}) on node ${NODE_NAME}..."
  # Keep the systemd service 'active' as long as at least one pod exists for this node.
  while true; do
    local out; out="$(pods_on_node)"
    if [[ -z "${out}" ]]; then
      # no pod presently; keep waiting (DaemonSet may bring it up)
      sleep 5
      continue
    fi
    # If at least one is Running, sleep longer; otherwise poll faster
    if echo "${out}" | awk '$2=="Running"{found=1} END{exit found?0:1}'; then
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
    echo "Usage: $0 {start|stop|restart|wait|status} [asic-id(optional, ignored)]" >&2
    exit 2
    ;;
esac

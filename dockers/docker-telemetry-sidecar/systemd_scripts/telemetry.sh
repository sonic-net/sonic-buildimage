#!/bin/bash
# 1. Runs as root via systemd service, so direct access to kubelet.conf is available; sudo is not required
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
  local out rc
  out=$(kubectl_retry -n "${NS}" delete pod "${name}" --force --grace-period=0 --wait=false 2>&1)
  rc=$?
  if (( rc != 0 )); then
    log "ERROR delete pod '${name}' failed rc=${rc}: ${out}"
  else
    log "Deleted pod '${name}'"
  fi
  return "$rc"
}

kill_pods() {
  mapfile -t names < <(pod_names_on_node)
  if (( ${#names[@]} == 0 )); then
    log "No pods found on ${NODE_NAME} (ns=${NS})."
    return 0
  fi

  log "Deleting pods on ${NODE_NAME}: ${names[*]}"

  local rc_any=0 rc=0
  for p in "${names[@]}"; do
    [[ -z "$p" ]] && continue
    if ! delete_pod_with_retry "$p"; then
      rc_any=1
    fi
  done

  if (( rc_any != 0 )); then
    log "ERROR one or more pod deletions failed on ${NODE_NAME}"
  else
    log "All targeted pods deleted on ${NODE_NAME}"
  fi
  return "$rc_any"
}

cmd_start() {
  if command -v systemd-cat >/dev/null 2>&1; then
    # background + pipe to journald with distinct priorities
    ( kill_pods ) \
      > >(systemd-cat -t telemetry-start -p info) \
      2> >(systemd-cat -t telemetry-start -p err)
  else
    # background + pipe to syslog via logger
    ( kill_pods ) \
      > >(logger -t "telemetry-start" -p user.info) \
      2> >(logger -t "telemetry-start" -p user.err)
  fi &
  disown
  exit 0
}
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

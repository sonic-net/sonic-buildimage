#!/bin/bash
# Shared Kubernetes pod control script for SONiC sidecar services
# 1. Runs as root via systemd service, so direct access to kubelet.conf is available; sudo is not required
# 2. Pod listing queries the local kubelet API (localhost:10250)
# 3. start/restart complete quickly (kubectl --wait=false); stop is a no-op
# 4. Only target pods matching POD_SELECTOR (default: raw_container_name=<SERVICE_NAME>)
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
KUBECTL_BIN="/usr/bin/kubectl"
KCF=(--kubeconfig=/etc/kubernetes/kubelet.conf)
REQ_TIMEOUT="5s"
MAX_ATTEMPTS=10
BACKOFF_START=1
BACKOFF_MAX=8

# Kubelet local API – queries stay on-node; avoids API server load
# Note: -k (skip server cert verification) is used because the kubelet's
# serving cert is self-signed and not signed by the cluster CA.  This is
# safe because the connection is to localhost only (loopback); there is no
# network path for MITM.  Client authentication is still performed via certs.
KUBELET_URL="https://localhost:10250"
KUBELET_CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
KUBELET_KEY="/var/lib/kubelet/pki/kubelet-client-current.pem"

# Label selector for pods; can be overridden via env
# Example override: POD_SELECTOR="app=telemetry" telemetry.sh start
POD_SELECTOR="${POD_SELECTOR:-raw_container_name=${SERVICE_NAME}}"

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

kubelet_get_pods() {
  local attempt=1 backoff=${BACKOFF_START} out rc http_code body err_msg
  while true; do
    # -w appends the 3-digit HTTP status code after the response body
    out=$(curl -sk --cert "${KUBELET_CERT}" --key "${KUBELET_KEY}" \
      --max-time 5 -w '%{http_code}' "${KUBELET_URL}/pods" 2>&1); rc=$?

    http_code="" body="" err_msg="$out"
    if (( rc == 0 )) && (( ${#out} >= 3 )); then
      http_code="${out: -3}"
      body="${out:0:${#out}-3}"
      if [[ "$http_code" =~ ^2[0-9][0-9]$ ]]; then
        printf '%s' "$body"
        return 0
      fi
      # Non-2xx: build a descriptive message and fall through to retry
      if [[ "$http_code" == "401" || "$http_code" == "403" ]]; then
        err_msg="HTTP ${http_code} from kubelet (authn/authz failure)"
      else
        err_msg="HTTP ${http_code} from kubelet"
      fi
      rc=1
    fi

    if (( attempt >= MAX_ATTEMPTS )); then
      echo "$err_msg" >&2
      return "${rc:-1}"
    fi
    log "kubelet curl retry ${attempt}/${MAX_ATTEMPTS}: ${err_msg}"
    sleep "${backoff}"
    (( backoff = backoff < BACKOFF_MAX ? backoff*2 : BACKOFF_MAX ))
    (( attempt++ ))
  done
}

pods_on_node() {
  local key="${POD_SELECTOR%%=*}"
  local val="${POD_SELECTOR#*=}"
  kubelet_get_pods | jq -r \
    --arg ns "${NS}" --arg key "$key" --arg val "$val" \
    '.items[] | select(.metadata.namespace == $ns) |
     select(.metadata.labels[$key] == $val) |
     "\(.metadata.name) \(.status.phase)"' || true
}

pod_names_on_node() {
  local key="${POD_SELECTOR%%=*}"
  local val="${POD_SELECTOR#*=}"
  kubelet_get_pods | jq -r \
    --arg ns "${NS}" --arg key "$key" --arg val "$val" \
    '.items[] | select(.metadata.namespace == $ns) |
     select(.metadata.labels[$key] == $val) |
     .metadata.name' || true
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
    log "No pods found on ${NODE_NAME} (ns=${NS}, selector=${POD_SELECTOR})."
    return 0
  fi

  log "Deleting pods on ${NODE_NAME} (ns=${NS}, selector=${POD_SELECTOR}): ${names[*]}"

  local rc_any=0
  for p in "${names[@]}"; do
    [[ -z "$p" ]] && continue
    if ! delete_pod_with_retry "$p"; then
      rc_any=1
    fi
  done

  if (( rc_any != 0 )); then
    log "ERROR one or more pod deletions failed on ${NODE_NAME} (selector=${POD_SELECTOR})"
  else
    log "All targeted pods deleted on ${NODE_NAME} (selector=${POD_SELECTOR})"
  fi
  return "$rc_any"
}

cmd_start() {
  # Re-invoke ourselves with the "restart" action under a hard 20s cap.
  # On a healthy node this finishes in 1-2s (kubectl --wait=false).
  # If the API server is unreachable, timeout(1) kills the child so we
  # stay well within the service's TimeoutStartSec (30s).
  timeout 20 "${BASH_SOURCE[0]}" "${SERVICE_NAME}" restart 2>&1 \
    | logger -t "${SERVICE_NAME}-start" || true
}

# stop is a no-op: K8s controls the pod lifecycle; deleting the pod here
# would just cause the controller to recreate it immediately.
cmd_stop()    { kill_pods; }
cmd_restart() { kill_pods; }

cmd_status() {
  local out=""; out="$(pods_on_node)"
  if [[ -z "$out" ]]; then
    echo "NOT RUNNING (no pod on node ${NODE_NAME} with selector '${POD_SELECTOR}')"
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
  local max_rounds="${WAIT_MAX_ROUNDS:-15}"
  local round=0
  log "Waiting on pods (ns=${NS}, selector=${POD_SELECTOR}) on node ${NODE_NAME} (max ${max_rounds} rounds)…"
  while true; do
    local out=""; out="$(pods_on_node)"
    if [[ -z "$out" ]]; then
      if (( ++round > max_rounds )); then
        log "ERROR: timed out after ${max_rounds} rounds waiting for pods (ns=${NS}, selector=${POD_SELECTOR}) on ${NODE_NAME}"
        exit 1
      fi
      sleep 20; continue
    fi
    if awk '$2=="Running"{found=1} END{exit found?0:1}' <<<"$out"; then
      round=0
      sleep 20
    else
      if (( ++round > max_rounds )); then
        log "ERROR: timed out after ${max_rounds} rounds waiting for pods (ns=${NS}, selector=${POD_SELECTOR}) on ${NODE_NAME}"
        exit 1
      fi
      sleep 20
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

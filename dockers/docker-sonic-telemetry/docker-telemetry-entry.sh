#!/usr/bin/env bash

# Control with env var launch_by, only used if container is launched via k8s
if [[ "${launch_by:-}" == "k8s" ]]; then
  echo "Running Part 1 because launch_by=k8s"

  set -euo pipefail

  # --- Create /usr/share/sonic/hwsku symlink (single-ASIC) ---
  ENV_FILE="/etc/sonic/sonic-environment"
  [[ -r "$ENV_FILE" ]] || { echo "ERROR: $ENV_FILE missing or unreadable."; exit 1; }
  # shellcheck source=/dev/null
  . "$ENV_FILE"
  [[ -n "${PLATFORM:-}" && -n "${HWSKU:-}" ]] || { echo "ERROR: PLATFORM/HWSKU not set in $ENV_FILE."; exit 1; }

  SRC_PLATFORM="/usr/share/sonic/device/${PLATFORM}"
  [[ -e "$SRC_PLATFORM" ]] || { echo "ERROR: platform dir not found for PLATFORM=$PLATFORM."; exit 1; }

  SRC_HWSKU="/usr/share/sonic/device/${PLATFORM}/${HWSKU}"
  [[ -e "$SRC_HWSKU" ]] || { echo "ERROR: hwsku dir not found for HWSKU=$HWSKU."; exit 1; }

  mkdir -p /usr/share/sonic

  DST="/usr/share/sonic/platform"
  rm -rf -- "$DST"
  ln -s -- "$SRC_PLATFORM" "$DST"

  DST="/usr/share/sonic/hwsku"
  rm -rf -- "$DST"
  ln -s -- "$SRC_HWSKU" "$DST"

  [[ "$(readlink -f /usr/share/sonic/platform)" == "$(readlink -f "$SRC_PLATFORM")" ]] || { echo "ERROR: platform symlink verification failed."; exit 1; }
  [[ "$(readlink -f /usr/share/sonic/hwsku)"    == "$(readlink -f "$SRC_HWSKU")"    ]] || { echo "ERROR: hwsku symlink verification failed."; exit 1; }
else
  echo "Skipping Part 1 (launch_by=${launch_by:-unset})"
fi

# ---check FEATURE table for switch ---
while true; do
  STATE=$(redis-cli -n 4 HGET "FEATURE|telemetry" state || true)
  if [[ "${STATE}" == "enabled" ]]; then
    echo "Telemetry is enabled. Starting service..."
    break
  else
    echo "Telemetry is disabled. Entering idle..."
    sleep 10
  fi
done

exec /usr/local/bin/supervisord

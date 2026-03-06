#!/bin/bash
# docker-init.sh — GCU venv setup and health-check script.
#
# Usage:
#   gcu-init.sh setup       — create /opt/gcu-venv and install the sonic-gcu wheel
#   gcu-init.sh healthcheck — run lightweight smoke-tests against the installed GCU
#
# This script is invoked by supervisord as two separate one-shot programs so that
# supervisord_dependent_startup can gate execution order:
#   1. gcu-setup        (priority=2): runs 'setup'       — exits 0 on success
#   2. gcu-healthcheck  (priority=3): runs 'healthcheck' — exits 0 on success

set -e

VENV_DIR="/opt/gcu-venv"
WHEEL_DIR="/python-wheels"
WHEEL_GLOB="${WHEEL_DIR}/sonic_gcu-*.whl"
SETUP_SENTINEL="${VENV_DIR}/.setup_complete"

###############################################################################
# setup: create venv and install wheel
###############################################################################
do_setup() {
    echo "[gcu-init] Starting GCU venv setup ..."

    # Create venv with --system-site-packages so C-extension packages
    # (swsscommon, sonic-py-common, libyang Python bindings, etc.) that are
    # installed system-wide are visible inside the venv without reinstalling
    # them. Pure-Python deps that are NOT on the system path must still be
    # declared in the wheel's install_requires and will be downloaded/installed
    # into the venv itself.
    if [ ! -d "${VENV_DIR}" ]; then
        python3 -m venv --system-site-packages "${VENV_DIR}"
        echo "[gcu-init] Created venv at ${VENV_DIR}"
    else
        echo "[gcu-init] Venv already exists at ${VENV_DIR}, skipping creation"
    fi

    # Install the sonic-gcu wheel.
    # shellcheck disable=SC2086
    WHEEL_FILE=$(ls ${WHEEL_GLOB} 2>/dev/null | head -n 1)
    if [ -z "${WHEEL_FILE}" ]; then
        echo "[gcu-init] ERROR: No sonic_gcu wheel found in ${WHEEL_DIR}" >&2
        exit 1
    fi

    echo "[gcu-init] Installing ${WHEEL_FILE} into ${VENV_DIR} ..."
    "${VENV_DIR}/bin/pip" install --no-index --system-site-packages "${WHEEL_FILE}"

    # Write a sentinel file so the healthcheck can verify setup completed OK.
    touch "${SETUP_SENTINEL}"
    echo "[gcu-init] GCU venv setup complete."
}

###############################################################################
# healthcheck: smoke-test venv GCU + host GCU
###############################################################################
do_healthcheck() {
    echo "[gcu-init] Running GCU health checks ..."

    # Gate on setup sentinel — if setup did not complete successfully, skip
    # the healthcheck to avoid misleading 'config apply-patch' errors.
    if [[ ! -f "${SETUP_SENTINEL}" ]]; then
        echo "[gcu-init] ERROR: Setup sentinel not found at ${SETUP_SENTINEL}." >&2
        echo "[gcu-init] 'gcu-setup' may have failed. Skipping healthcheck." >&2
        exit 1
    fi

    # Health-check 1: venv GCU — apply an empty JSON patch via the venv binary.
    # An empty patch list '[]' is a no-op and must succeed without error.
    echo "[gcu-init] Checking venv GCU (${VENV_DIR}/bin/config apply-patch) ..."
    if echo '[]' | "${VENV_DIR}/bin/config" apply-patch /dev/stdin; then
        echo "[gcu-init] Venv GCU health check: PASS"
    else
        echo "[gcu-init] Venv GCU health check: FAIL" >&2
        exit 1
    fi

    # Health-check 2: host (system-installed) GCU — same empty patch via the
    # system-wide 'config' CLI.  SONiC containers run as root so sudo is not
    # needed; running it directly avoids requiring a sudoers configuration
    # inside the container.
    echo "[gcu-init] Checking host GCU (config apply-patch) ..."
    if echo '[]' | config apply-patch /dev/stdin; then
        echo "[gcu-init] Host GCU health check: PASS"
    else
        echo "[gcu-init] Host GCU health check: FAIL" >&2
        exit 1
    fi

    echo "[gcu-init] All GCU health checks passed."
}

###############################################################################
# Dispatch
###############################################################################
COMMAND="${1:-setup}"

case "${COMMAND}" in
    setup)
        do_setup
        ;;
    healthcheck)
        do_healthcheck
        ;;
    *)
        echo "Usage: $0 {setup|healthcheck}" >&2
        exit 1
        ;;
esac

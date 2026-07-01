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

GCU_BASE_DIR="/opt/sonic/gcu"
VENV_DIR="${GCU_BASE_DIR}/venv"
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
    # --no-deps: skip resolving declared deps (e.g. click==7.0) since the venv
    # was created with --system-site-packages and inherits them from the system.
    "${VENV_DIR}/bin/pip" install --no-deps "${WHEEL_FILE}"

    # Create the well-known symlink that sonic-utilities references as
    # GCU_STANDALONE_BIN = "/opt/sonic/gcu/current/bin/gcu-standalone".
    # The console_scripts entry point installed by pip is at ${VENV_DIR}/bin/gcu-standalone.
    # Both the symlink target and the venv live under /opt/sonic/gcu which is a
    # host bind-mount, so the host can execute the binary directly.
    GCU_STANDALONE_DIR="${GCU_BASE_DIR}/current/bin"
    GCU_STANDALONE_BIN="${GCU_STANDALONE_DIR}/gcu-standalone"
    mkdir -p "${GCU_STANDALONE_DIR}"
    ln -sf "${VENV_DIR}/bin/gcu-standalone" "${GCU_STANDALONE_BIN}"
    echo "[gcu-init] Symlinked ${GCU_STANDALONE_BIN} -> ${VENV_DIR}/bin/gcu-standalone"

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

    # Health-check 1: venv GCU — verify the generic_config_updater module is
    # importable and the GCU classes instantiate without error.
    echo "[gcu-init] Checking venv GCU (python3 -m generic_config_updater) ..."
    if "${VENV_DIR}/bin/python3" -c "
from generic_config_updater.generic_updater import GenericUpdater
print('[gcu-init] generic_config_updater import OK')
"; then
        echo "[gcu-init] Venv GCU health check: PASS"
    else
        echo "[gcu-init] Venv GCU health check: FAIL" >&2
        exit 1
    fi

    # Health-check 2: verify the well-known symlink exists and is executable.
    GCU_STANDALONE_BIN="${GCU_BASE_DIR}/current/bin/gcu-standalone"
    echo "[gcu-init] Checking standalone symlink (${GCU_STANDALONE_BIN}) ..."
    if [ -x "${GCU_STANDALONE_BIN}" ]; then
        echo "[gcu-init] Standalone symlink health check: PASS"
    else
        echo "[gcu-init] Standalone symlink health check: FAIL (not found or not executable)" >&2
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

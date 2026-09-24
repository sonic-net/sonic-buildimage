#!/bin/bash
#
# build_sbom.sh — thin shim that delegates to the Python aggregator.
#
# Kept as a shell entry point so slave.mk has a stable command name; all
# SBOM-aggregation logic lives in scripts/build_sbom.py. Honours
# ENABLE_SBOM via the Python script's self-gate.

set -e
SCRIPT_DIR="$(dirname "$0")"

# Refresh auto-extracted OpenVEX statements before the SBOM is written.
# The extractor scans src/*/patches/ for CVE markers in patch headers
# and emits OpenVEX JSON under vex/auto/ — output is deterministic and
# vex/auto/ is gitignored, so this regenerates from source on every
# build instead of trusting potentially-stale committed JSON.
if [ "${ENABLE_SBOM:-n}" = "y" ]; then
    python3 "${SCRIPT_DIR}/sbom_extract_vex_from_patches.py" \
        --output vex/auto 2>&1 || true
fi

exec python3 "${SCRIPT_DIR}/build_sbom.py" "$@"

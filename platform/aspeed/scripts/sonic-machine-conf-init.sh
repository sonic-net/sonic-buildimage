#!/bin/sh
# SONiC Machine Configuration Initialization Script
# This script creates machine.conf on first boot by detecting hardware from DTB.
# Override paths for initramfs: MACHINE_CONF=/tmp/machine.conf LOG_FILE=/dev/null

set -e

MACHINE_CONF="${MACHINE_CONF:-/host/machine.conf}"
LOG_FILE="${LOG_FILE:-/var/log/sonic-machine-conf-init.log}"

mkdir -p "$(dirname "$MACHINE_CONF")"
case "$LOG_FILE" in
/dev/fd/*|/dev/null) ;;
*) mkdir -p "$(dirname "$LOG_FILE")" ;;
esac

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting machine.conf initialization..."

# Check if machine.conf already exists
if [ -f "$MACHINE_CONF" ]; then
    log "machine.conf already exists at $MACHINE_CONF. Skipping initialization."
    exit 0
fi

# Detect hardware from device tree (compatible and model)
if [ ! -f /proc/device-tree/compatible ]; then
    log "ERROR: /proc/device-tree/compatible not found. Cannot detect hardware."
    exit 1
fi

COMPATIBLE=$(cat /proc/device-tree/compatible | tr '\0' ' ')
MODEL=$(cat /proc/device-tree/model 2>/dev/null || echo "")
log "Detected compatible: $COMPATIBLE, model: $MODEL"

PLATFORM=""
MACHINE=""

if echo "$COMPATIBLE" | grep -q "nexthop,nexthop-b27-r0"; then
    PLATFORM="arm64-nexthop_b27-r0"
    MACHINE="aspeed_ast2700"
    log "Detected NextHop B27 BMC platform"
elif echo "$MODEL" | grep -qi "AST2700-A1 Spc6"; then
    PLATFORM="arm64-aspeed_nvidia_ast2700_a1_bmc-r0"
    MACHINE="aspeed_ast2700"
    log "Detected NVIDIA SPC6 BMC platform"
elif echo "$COMPATIBLE" | grep -q "aspeed,ast2700-evb"; then
    PLATFORM="arm64-aspeed_ast2700_evb-r0"
    MACHINE="aspeed_ast2700"
    log "Detected Aspeed AST2700 EVB platform"
else
    log "ERROR: Unknown hardware. Compatible: $COMPATIBLE, model: $MODEL"
    log "Supported platforms: nexthop-b27-r0, nvidia-spc6-bmc, aspeed_ast2700_evb"
    exit 1
fi

# Create machine.conf
log "Creating $MACHINE_CONF for platform: $PLATFORM"

cat > "$MACHINE_CONF" << EOF
onie_platform=$PLATFORM
onie_machine=$MACHINE
onie_arch=arm64
onie_build_platform=$PLATFORM
EOF

log "Created $MACHINE_CONF:"
cat "$MACHINE_CONF" | tee -a "$LOG_FILE"

log "Machine configuration initialization complete"

exit 0


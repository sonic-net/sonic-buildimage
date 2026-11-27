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

# An empty or truncated file (no onie_platform) is regenerated rather than kept.
if grep -q '^onie_platform=.' "$MACHINE_CONF" 2>/dev/null; then
    log "machine.conf already exists at $MACHINE_CONF. Skipping initialization."
    exit 0
fi

# Detect hardware from device tree (compatible and model)
DT_BASE=/proc/device-tree
[ -f "$DT_BASE/compatible" ] || DT_BASE=/sys/firmware/devicetree/base
if [ ! -f "$DT_BASE/compatible" ]; then
    log "ERROR: device tree compatible not found. Cannot detect hardware."
    exit 1
fi

COMPATIBLE=$(tr '\0' ' ' < "$DT_BASE/compatible")
MODEL=$(tr -d '\0' 2>/dev/null < "$DT_BASE/model" || echo "")
log "Detected compatible: $COMPATIBLE, model: $MODEL"

PLATFORM=""
MACHINE=""

if echo "$COMPATIBLE" | grep -q "nexthop,nexthop-b27-r0"; then
    PLATFORM="arm64-nexthop_b27-r0"
    MACHINE="aspeed_ast2700"
    log "Detected NextHop B27 BMC platform"
elif echo "$MODEL" | grep -qi -e "AST2700-A1 Spc6" -e "AST2700-A2 Spc6"; then
    PLATFORM="arm64-aspeed_nvidia_ast2700_bmc-r0"
    MACHINE="aspeed_ast2700"
    log "Detected NVIDIA SPC6 BMC platform"
elif echo "$COMPATIBLE" | grep -q "aspeed,ast2700-evb"; then
    PLATFORM="arm64-aspeed_ast2700_evb-r0"
    MACHINE="aspeed_ast2700"
    log "Detected Aspeed AST2700 EVB platform"
elif echo "$COMPATIBLE" | grep -q "arista_goldfinch-r0"; then
   PLATFORM="arm64-arista_goldfinch-r0"
   MACHINE="aspeed_ast2700"
   log "Detected Arista Goldfinch platform"
elif echo "$MODEL" | grep -q "Nokia-BMC-H6-128"; then
    # The Nokia DTB carries only the generic aspeed,ast2700 compatible.
    PLATFORM="arm64-nokia_bmc_h6_128-r0"
    MACHINE="aspeed_ast2700"
    log "Detected Nokia H6-128 BMC platform"
else
    log "ERROR: Unknown hardware. Compatible: $COMPATIBLE, model: $MODEL"
    log "Supported platforms: nexthop-b27-r0, nvidia-spc6-bmc, aspeed_ast2700_evb, arista_goldfinch-r0, nokia_bmc_h6_128-r0"
    exit 1
fi

# Create machine.conf
log "Creating $MACHINE_CONF for platform: $PLATFORM"

# Write via a temp file so a power loss never leaves a partial machine.conf.
TMP_CONF="$MACHINE_CONF.tmp.$$"
trap 'rm -f "$TMP_CONF"' EXIT
cat > "$TMP_CONF" << EOF
onie_platform=$PLATFORM
onie_machine=$MACHINE
onie_arch=arm64
onie_build_platform=$PLATFORM
EOF
sync
mv -f "$TMP_CONF" "$MACHINE_CONF"
sync

log "Created $MACHINE_CONF:"
cat "$MACHINE_CONF" | tee -a "$LOG_FILE"

log "Machine configuration initialization complete"

exit 0


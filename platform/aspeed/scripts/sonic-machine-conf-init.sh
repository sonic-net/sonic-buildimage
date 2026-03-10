#!/bin/bash
# SONiC Machine Configuration Initialization Script
# This script creates /host/machine.conf on first boot by detecting hardware from DTB

set -e

LOG_FILE="/var/log/sonic-machine-conf-init.log"
MACHINE_CONF="/host/machine.conf"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting machine.conf initialization..."

# Check if machine.conf already exists
if [ -f "$MACHINE_CONF" ]; then
    log "machine.conf already exists at $MACHINE_CONF. Skipping initialization."
    exit 0
fi

# Detect hardware from device tree compatible string
if [ ! -f /proc/device-tree/compatible ]; then
    log "ERROR: /proc/device-tree/compatible not found. Cannot detect hardware."
    exit 1
fi

# Read compatible string (null-separated values)
COMPATIBLE=$(cat /proc/device-tree/compatible | tr '\0' ' ')
log "Detected compatible string: $COMPATIBLE"

# Map compatible string to platform name
# The compatible string contains multiple values, we check for specific vendor strings
PLATFORM=""
MACHINE=""

if echo "$COMPATIBLE" | grep -q "nexthop,nexthop-b27-r0"; then
    PLATFORM="arm64-nexthop_b27-r0"
    MACHINE="aspeed_ast2700"
    log "Detected NextHop B27 BMC platform"
elif echo "$COMPATIBLE" | grep -q "aspeed,ast2700-evb"; then
    PLATFORM="arm64-aspeed_ast2700_evb-r0"
    MACHINE="aspeed_ast2700"
    log "Detected Aspeed AST2700 EVB platform"
else
    log "ERROR: Unknown hardware. Compatible string: $COMPATIBLE"
    log "Supported platforms:"
    log "  - nexthop,nexthop-b27-r0 -> arm64-nexthop_b27-r0"
    log "  - aspeed,ast2700-evb -> arm64-aspeed_ast2700_evb-r0"
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


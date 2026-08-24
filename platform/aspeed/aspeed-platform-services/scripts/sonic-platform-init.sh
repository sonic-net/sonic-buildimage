#!/bin/bash
# Aspeed Platform Initialization Script
# This script sets up the platform symlink and installs platform wheels on the host

set -e

LOG_FILE="/var/log/aspeed-platform-init.log"
PLATFORM_SYMLINK="/usr/share/sonic/platform"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting Aspeed platform initialization..."

# Get platform name from machine.conf
if [ ! -f /host/machine.conf ]; then
    log "ERROR: /host/machine.conf not found. Cannot determine platform."
    exit 1
fi

# Source machine.conf to get platform
. /host/machine.conf

PLATFORM=""
if [ -n "$aboot_platform" ]; then
    PLATFORM="$aboot_platform"
elif [ -n "$onie_platform" ]; then
    PLATFORM="$onie_platform"
else
    log "ERROR: Could not determine platform from /host/machine.conf"
    exit 1
fi

log "Detected platform: $PLATFORM"

PLATFORM_DEVICE_DIR="/usr/share/sonic/device/$PLATFORM"

# Check if platform device directory exists
if [ ! -d "$PLATFORM_DEVICE_DIR" ]; then
    log "ERROR: Platform device directory not found: $PLATFORM_DEVICE_DIR"
    exit 1
fi

# Create platform symlink for host services (watchdog-control, etc.)
log "Creating platform symlink..."

# Remove old symlink if it exists
if [ -L "$PLATFORM_SYMLINK" ]; then
    log "Removing existing symlink: $PLATFORM_SYMLINK"
    rm -f "$PLATFORM_SYMLINK"
elif [ -e "$PLATFORM_SYMLINK" ]; then
    log "WARNING: $PLATFORM_SYMLINK exists but is not a symlink. Removing..."
    rm -rf "$PLATFORM_SYMLINK"
fi

# Create the symlink
ln -sf "$PLATFORM_DEVICE_DIR" "$PLATFORM_SYMLINK"
log "Created symlink: $PLATFORM_SYMLINK -> $PLATFORM_DEVICE_DIR"

# Install platform wheels on host for services like watchdog-control
# These services run on the host (not in containers) and need to import sonic_platform
log "Installing platform wheels on host..."

WHEEL_COUNT=$(find "$PLATFORM_DEVICE_DIR" -maxdepth 1 -name "*.whl" 2>/dev/null | wc -l)

if [ "$WHEEL_COUNT" -eq 0 ]; then
    log "WARNING: No .whl files found in $PLATFORM_DEVICE_DIR"
else
    log "Found $WHEEL_COUNT wheel file(s) to install"
    
    # Install wheels without dependencies (dependencies should already be installed)
    for wheel in "$PLATFORM_DEVICE_DIR"/*.whl; do
        if [ -f "$wheel" ]; then
            log "Installing: $(basename $wheel)"
            if pip3 install --no-deps "$wheel"; then
                log "Successfully installed: $(basename $wheel)"
            else
                log "WARNING: Failed to install: $(basename $wheel)"
            fi
        fi
    done
fi

log "Aspeed platform initialization complete"

exit 0


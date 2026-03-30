#!/bin/bash
# Aspeed USB Network Gadget udev Rules Initialization Script
# This script installs udev rules to rename USB network interface from usb0 to bmc0

set -e

LOG_FILE="/var/log/sonic-usb-network-udev-init.log"
PLATFORM_SYMLINK="/usr/share/sonic/platform"
RULES_SRC="${PLATFORM_SYMLINK}/70-usb-network.rules"
RULES_DEST="/etc/udev/rules.d/70-usb-network.rules"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting USB network udev rules initialization..."

# Check if rules source exists
if [ ! -f "$RULES_SRC" ]; then
    log "ERROR: USB network udev rules not found at $RULES_SRC"
    exit 1
fi

# Install udev rules
log "Installing USB network udev rules..."
cp "$RULES_SRC" "$RULES_DEST"
log "Installed: $RULES_DEST"

# Reload udev to activate the rules
log "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger --subsystem-match=net
log "Udev rules reloaded and triggered"

# Check if interface was renamed (if it already exists)
if ip link show bmc0 &>/dev/null; then
    log "USB network interface 'bmc0' found"
elif ip link show usb0 &>/dev/null; then
    log "WARNING: Interface 'usb0' still exists, rename may apply on next interface creation"
else
    log "No USB network interface found yet (will be created by usb-network-init.sh)"
fi

log "USB network udev rules initialization complete"
exit 0


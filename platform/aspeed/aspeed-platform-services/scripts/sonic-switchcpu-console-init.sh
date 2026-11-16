#!/bin/bash
# Aspeed Switch CPU Console Initialization Script
# This script configures the switch CPU console for consutil access

set -e

LOG_FILE="/var/log/aspeed-switchcpu-console-init.log"
PLATFORM_SYMLINK="/usr/share/sonic/platform"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting switch CPU console initialization..."

# Verify platform symlink exists
if [ ! -L "$PLATFORM_SYMLINK" ]; then
    log "ERROR: Platform symlink $PLATFORM_SYMLINK does not exist"
    exit 1
fi

RULES_SRC="${PLATFORM_SYMLINK}/99-switchCpu.rules"
RULES_DEST="/etc/udev/rules.d/99-switchCpu.rules"
CONSOLE_CONF="${PLATFORM_SYMLINK}/switch_cpu_console.conf"

# Set default console parameters
CONSOLE_BAUD_RATE=115200
CONSOLE_FLOW_CONTROL=0
CONSOLE_REMOTE_DEVICE=SwitchCpu

# Load platform-specific console configuration if available
if [ -f "$CONSOLE_CONF" ]; then
    log "Loading console configuration from $CONSOLE_CONF"
    # shellcheck disable=SC1090
    . "$CONSOLE_CONF"
    log "Console settings: baud=$CONSOLE_BAUD_RATE, flow=$CONSOLE_FLOW_CONTROL, device=$CONSOLE_REMOTE_DEVICE"
else
    log "No console.conf found, using defaults"
fi

# Check if platform provides switch CPU console rules
if [ ! -f "$RULES_SRC" ]; then
    log "No switch CPU console rules found at $RULES_SRC, skipping console setup"
    exit 0
fi

log "Found switch CPU console rules, configuring console..."

# Symlink udev rules
ln -sf "$RULES_SRC" "$RULES_DEST"
log "Created symlink: $RULES_DEST -> $RULES_SRC"

# Reload udev to activate the rules
log "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger
log "Udev rules reloaded and triggered"

# Configure CONSOLE_PORT|0 with platform settings. They are defaults only:
# a field already in CONFIG_DB (e.g. a saved baud_rate) is user config and is kept.
log "Configuring CONSOLE_PORT|0 in CONFIG_DB..."

set_default() {
    local field=$1 value=$2 current

    current=$(sonic-db-cli CONFIG_DB HGET "CONSOLE_PORT|0" "$field" 2>/dev/null || true)
    if [ -n "$current" ]; then
        log "Keeping existing CONSOLE_PORT|0 $field=$current"
        return 0
    fi

    log "Setting CONSOLE_PORT|0 $field=$value"
    if ! sonic-db-cli CONFIG_DB HSET "CONSOLE_PORT|0" "$field" "$value" >/dev/null; then
        log "ERROR: Failed to set CONSOLE_PORT|0 $field"
        exit 1
    fi
}

set_default baud_rate "$CONSOLE_BAUD_RATE"
set_default flow_control "$CONSOLE_FLOW_CONTROL"
set_default remote_device "$CONSOLE_REMOTE_DEVICE"
log "CONSOLE_PORT|0 configured successfully"

log "Switch CPU console initialization complete"
exit 0


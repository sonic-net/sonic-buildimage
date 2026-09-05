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

# Enable console feature
log "Enabling console feature..."
if config console enable; then
    log "Console feature enabled via config command"
else
    log "WARNING: config console enable failed, trying direct DB write..."
fi

# Verify and ensure it's actually enabled in CONFIG_DB
if sonic-db-cli CONFIG_DB HSET "CONSOLE_SWITCH|console_mgmt" enabled yes; then
    log "Console feature enabled in CONFIG_DB"
else
    log "ERROR: Failed to enable console feature in CONFIG_DB"
    exit 1
fi

# Reload udev to activate the rules
log "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger
log "Udev rules reloaded and triggered"

# Configure CONSOLE_PORT|0 with platform settings
log "Configuring CONSOLE_PORT|0 in CONFIG_DB..."

# Check if entry exists
if sonic-db-cli CONFIG_DB HGETALL "CONSOLE_PORT|0" >/dev/null 2>&1; then
    log "CONSOLE_PORT|0 already exists, deleting to ensure correct settings..."
    sonic-db-cli CONFIG_DB DEL "CONSOLE_PORT|0"
fi

# Create with platform-specific settings
log "Creating CONSOLE_PORT|0 with baud=$CONSOLE_BAUD_RATE, flow=$CONSOLE_FLOW_CONTROL, device=$CONSOLE_REMOTE_DEVICE"
if sonic-db-cli CONFIG_DB HMSET "CONSOLE_PORT|0" \
    baud_rate "$CONSOLE_BAUD_RATE" \
    flow_control "$CONSOLE_FLOW_CONTROL" \
    remote_device "$CONSOLE_REMOTE_DEVICE"; then
    log "Created CONSOLE_PORT|0 successfully"
else
    log "ERROR: Failed to create CONSOLE_PORT|0"
    exit 1
fi

log "Switch CPU console initialization complete"
exit 0


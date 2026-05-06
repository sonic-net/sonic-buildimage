#!/bin/bash
# eth0 MAC Address Fixup Script for NextHop B27
# Sets MAC address on eth0 from system EEPROM or U-boot environment

set -e

LOGGER_TAG="eth0-mac-fixup"

log() {
    local prio="$1"
    local msg="$2"
    logger -t "${LOGGER_TAG}" -p "$prio" "$msg"
}

log_info() {
    local msg="$1"
    log "user.info" "$msg"
}

log_error() {
    local msg="$1"
    log "user.error" "$msg"
}

# Strict MAC address validation to be XX:XX:XX:XX:XX:XX hex
# Returns 0 (success) if valid, 1 (failure) if invalid
# Validates format and rejects invalid patterns
validate_mac() {
    local mac="$1"

    if ! echo "$mac" | grep -qE '^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$'; then
        return 1
    fi

    return 0
}

# Step 1: Get MAC from EEPROM + uboot with validation
# Note: EEPROM device is created by kernel at24 driver from device tree
# systemd dependencies ensure I2C drivers are loaded before this script runs
EEPROM_MAC=$(decode-syseeprom -m 2>/dev/null || true)
UBOOT_MAC=$(fw_printenv 2>/dev/null | grep '^ethaddr=' | cut -d'=' -f2 || true)

MAC_TO_USE=""

# Priority: Use EEPROM MAC if valid, otherwise fall back to U-boot MAC
if validate_mac "$EEPROM_MAC"; then
    MAC_TO_USE="$EEPROM_MAC"
    log_info "Using EEPROM MAC: $EEPROM_MAC"

    # Sync U-boot environment to match EEPROM (if different)
    if [ -n "$UBOOT_MAC" ] && [ "$UBOOT_MAC" != "$EEPROM_MAC" ]; then
        log_info "Updating U-boot ethaddr from $UBOOT_MAC to $EEPROM_MAC"
        fw_setenv ethaddr "$EEPROM_MAC"
    elif [ -z "$UBOOT_MAC" ]; then
        log_info "Setting U-boot ethaddr to $EEPROM_MAC (was unset)"
        fw_setenv ethaddr "$EEPROM_MAC"
    fi
elif validate_mac "$UBOOT_MAC"; then
    MAC_TO_USE="$UBOOT_MAC"
    log_info "EEPROM MAC invalid ('$EEPROM_MAC'), falling back to U-boot MAC: $UBOOT_MAC"
else
    # Both sources are invalid - cannot proceed
    log_error "No valid MAC address available - EEPROM: '$EEPROM_MAC', U-boot: '$UBOOT_MAC'"
    exit 1
fi

# Step 2: Verify eth0 interface exists
# Note: systemd dependencies ensure network drivers are loaded before networking starts
INTERFACE='eth0'
if ! ip link show "$INTERFACE" &>/dev/null; then
    log_error "Interface '$INTERFACE' not found"
    exit 1
fi

CURRENT_ETH0_MAC=$(cat /sys/class/net/eth0/address)
if [ "$(echo "$CURRENT_ETH0_MAC" | tr 'A-F' 'a-f')" = "$(echo "$MAC_TO_USE" | tr 'A-F' 'a-f')" ]; then
    log_info "eth0 MAC already matches desired MAC, skipping update"
    exit 0
fi

# Step 3: Update MAC address
log_info "Updating MAC address on ${INTERFACE} to ${MAC_TO_USE}"

# Bring interface down
ip link set "${INTERFACE}" down 2>/dev/null || true

# Flush all IP addresses to clear stale DHCP leases
ip addr flush dev "${INTERFACE}" 2>/dev/null || true

# Set MAC address
if ! ip link set "${INTERFACE}" address "${MAC_TO_USE}"; then
    log_error "Failed to set MAC address on ${INTERFACE}"
    exit 1
fi

log_info "Successfully set MAC address ${MAC_TO_USE} on ${INTERFACE}"

# Bring interface up
ip link set "${INTERFACE}" up || true

# If DHCP client is already running, restart it to get new lease with updated MAC
# (This handles the case where networking.service started before this script ran on first boot)
DHCLIENT_PID_FILE="/var/run/dhclient.${INTERFACE}.pid"
if [ -f "$DHCLIENT_PID_FILE" ]; then
    log_info "Restarting dhclient to obtain new DHCP lease with updated MAC"
    dhclient -r "${INTERFACE}" 2>/dev/null || true
    dhclient "${INTERFACE}" >/dev/null 2>&1
fi

log_info "eth0 MAC address updated to ${MAC_TO_USE}"
exit 0

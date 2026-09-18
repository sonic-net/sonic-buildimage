#!/bin/bash
# Micas USB gadget udev rule initialization.

set -euo pipefail

RULES_DEST="/etc/udev/rules.d/70-usb-network.rules"

log() {
    logger -t micas-usb-udev "$*"
}

cat > "${RULES_DEST}" <<'EOF'
# Micas USB network interface rename rule
SUBSYSTEM=="net", ACTION=="add", DRIVERS=="?*", ATTR{address}=="02:00:00:00:00:01", NAME="bmc0"
EOF

udevadm control --reload-rules
udevadm trigger --subsystem-match=net

if ip link show bmc0 &>/dev/null; then
    log "USB network interface 'bmc0' found"
elif ip link show usb0 &>/dev/null; then
    log "Interface 'usb0' still exists and will be renamed on the next device event"
else
    log "USB network interface is not present yet"
fi

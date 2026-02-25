#!/usr/bin/env bash
#
# sonic-udev-setup.sh
# Enables console mode and symlinks the platform udev rules file.
# Assumes 99-sonic-tty.rules and udevprefix.conf already exist in the platform directory.
# Must be run as root on a Debian-based system.
#

set -euo pipefail

PLATFORM_NAME=$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
PLATFORM_DIR="/usr/share/sonic/device/${PLATFORM_NAME}"
RULES_SRC="${PLATFORM_DIR}/99-sonic-tty.rules"
RULES_DEST="/etc/udev/rules.d/99-sonic-tty.rules"

# --- Ensure we are running as root ---
if [[ "$(id -u)" -ne 0 ]]; then
    echo "Error: this script must be run as root." >&2
    exit 1
fi

# --- Enable console ---
echo "Enabling console ..."
sudo config console enable

# --- Symlink udev rules ---
echo "Creating symlink ${RULES_DEST} -> ${RULES_SRC} ..."
ln -sf "${RULES_SRC}" "${RULES_DEST}"

# --- Reload udev rules ---
echo "Reloading udev rules ..."
udevadm control --reload-rules
udevadm trigger

echo "sonic udev setup complete."
echo "  Rules symlinked: ${RULES_DEST} -> ${RULES_SRC}"

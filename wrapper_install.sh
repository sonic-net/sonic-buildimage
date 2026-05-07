#!/bin/bash
# wrapper_install.sh
# Unified installer for SONIE + SONiC

set -e
set -x

# Redirect to console for visibility in QEMU
if [[ -c /dev/ttyS0 ]]; then
    exec > /dev/ttyS0 2>&1
fi
log() {
    echo "[UNIFIED_INSTALLER]: $1"
}

log "Starting Unified Installation..."

# Resolve script directory and change into it
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
log "Changed directory to: $SCRIPT_DIR"


# 1. Detect Installers in the current directory (which is extracted temp dir)
# We expect them to be named sonie-*.bin and sonic-*.bin
# The imageset construction in test_installer_qemu.sh copies them here.

SONIE_BIN=$(find . -maxdepth 1 -name "sonie-*.bin" | head -n1)
SONIC_BIN=$(find . -maxdepth 1 -name "sonic-*.bin" | head -n1)

if [[ -z "$SONIE_BIN" ]]; then
    log "Error: SONIE installer binary not found!"
    exit 1
fi

if [[ -z "$SONIC_BIN" ]]; then
    log "Error: SONiC installer binary not found!"
    exit 1
fi

# 2. Install SONIE
log "Found SONIE Installer: $SONIE_BIN"
log "Installing SONIE with sonic-installer..."
chmod +x "$SONIE_BIN"
# Use --skip-bootloader for SONIE because we are in ONIE and the SONIE environment/EFI might not exist yet.
# This treats it as a "forced" install using the DummyBootloader.
# Run SONIE installer directly to avoid sonic-installer's squashfs dependency
./"$SONIE_BIN"
log "SONIE Installation Complete."

# 3. Install SONiC
log "Found SONiC Installer: $SONIC_BIN"
log "Installing SONiC with sonic-installer..."
chmod +x "$SONIC_BIN"
# Use standard sonic-installer (no skip-bootloader).
# Since SONIE is now installed, sonic-installer should detect the 'sonie' bootloader (via SonieGrubBootloader)
# and correctly handle boot env vars (bootcount, rollback, etc.).
# We must mount the XBOOTLDR partition to /host so that sonic-installer can read grub.cfg
# User assumes grub.cfg is already mounted to /host/grub (e.g. via bind-mount or previous step)
if [ ! -f /host/grub/grub.cfg ]; then
    log "WARNING: /host/grub/grub.cfg not found! sonic-installer might fail."
else
    log "Verified /host/grub/grub.cfg exists."
fi

yes | sonic-installer install -y --skip-platform-check ./"$SONIC_BIN"
log "SONiC Installation Complete."
trap - EXIT

log "Unified Installation Successful. Exiting (ONIE discovery should reboot)."
exit 0

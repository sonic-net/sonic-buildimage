#!/bin/bash
#
# Install SONiC to eMMC from OpenBMC
#
# This script is designed to be run from a running OpenBMC instance (in SPI flash)
# to install SONiC onto the eMMC storage.
#
# Usage: ./install-sonic-to-emmc.sh <path-to-sonic-aspeed-arm64-emmc.img.gz>
#

set -e

EMMC_DEVICE="/dev/mmcblk0"
EMMC_PARTITION="${EMMC_DEVICE}p1"
MOUNT_POINT="/mnt/sonic_install"
#BOOTCONF="ast2700-evb"
BOOTCONF="nexthop-b27-r0"

# Platform-specific console settings
# AST2700 EVB uses ttyS12, AST2720 uses ttyS12
CONSOLE_DEV="12"
CONSOLE_SPEED="115200"
EARLYCON="earlycon=uart8250,mmio32,0x14c33b00"
VAR_LOG_SIZE="512"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    log_error "This script must be run as root"
    exit 1
fi

# Check arguments
if [ $# -ne 1 ]; then
    log_error "Usage: $0 <path-to-sonic-aspeed-arm64-emmc.img.gz>"
    exit 1
fi

SONIC_IMAGE="$1"

# Verify image file exists
if [ ! -f "$SONIC_IMAGE" ]; then
    log_error "Image file not found: $SONIC_IMAGE"
    exit 1
fi

log_info "SONiC eMMC Installation Script"
log_info "================================"
log_info "Image: $SONIC_IMAGE"
log_info "Target: $EMMC_DEVICE"
log_info ""

# Verify eMMC device exists
if [ ! -b "$EMMC_DEVICE" ]; then
    log_error "eMMC device not found: $EMMC_DEVICE"
    exit 1
fi

log_info "eMMC device found: $EMMC_DEVICE"

# Get eMMC size
EMMC_SIZE=$(blockdev --getsize64 $EMMC_DEVICE 2>/dev/null || echo "0")
EMMC_SIZE_GB=$((EMMC_SIZE / 1024 / 1024 / 1024))
log_info "eMMC size: ${EMMC_SIZE_GB}GB"

# Confirm installation
log_warn "WARNING: This will ERASE all data on $EMMC_DEVICE!"
read -p "Are you sure you want to continue? (yes/no): " CONFIRM
if [ "$CONFIRM" != "yes" ]; then
    log_info "Installation cancelled"
    exit 0
fi

# Unmount any existing partitions
log_info "Unmounting any existing eMMC partitions..."
umount ${EMMC_DEVICE}p* 2>/dev/null || true

# Write image to eMMC
log_info "Writing SONiC image to eMMC (this may take several minutes)..."
log_info "Command: gunzip -c $SONIC_IMAGE | dd of=$EMMC_DEVICE bs=4M"

if ! gunzip -c "$SONIC_IMAGE" | dd of="$EMMC_DEVICE" bs=4M; then
    log_error "Failed to write image to eMMC"
    exit 1
fi

log_info "Syncing writes to disk..."
sync
sleep 2

# Verify partition exists
if [ ! -b "$EMMC_PARTITION" ]; then
    log_error "Partition not found after writing: $EMMC_PARTITION"
    exit 1
fi

# Mount the SONiC partition
log_info "Mounting SONiC partition..."
mkdir -p "$MOUNT_POINT"
if ! mount "$EMMC_PARTITION" "$MOUNT_POINT"; then
    log_error "Failed to mount $EMMC_PARTITION"
    exit 1
fi

# Get SONiC version from image directory
log_info "Detecting SONiC version..."
SONIC_IMAGE_DIR=$(ls -1 "$MOUNT_POINT" | grep "^image-" | head -n 1)

if [ -z "$SONIC_IMAGE_DIR" ]; then
    log_error "No SONiC image directory found in $MOUNT_POINT"
    umount "$MOUNT_POINT"
    exit 1
fi

# Extract version (remove 'image-' prefix)
IMAGE_DIR="$SONIC_IMAGE_DIR"
SONIC_VERSION="${SONIC_IMAGE_DIR#image-}"

log_info "Detected image directory: $IMAGE_DIR"
log_info "Detected SONiC version: $SONIC_VERSION"

# Get partition UUID
log_info "Getting partition UUID..."
UUID=$(blkid "$EMMC_PARTITION" | sed -n 's/.*UUID="\([^"]*\)".*/\1/p')

if [ -z "$UUID" ]; then
    log_error "Failed to get partition UUID"
    umount "$MOUNT_POINT"
    exit 1
fi

log_info "Partition UUID: $UUID"

# Unmount
log_info "Unmounting SONiC partition..."
umount "$MOUNT_POINT"
rmdir "$MOUNT_POINT"

# Configure U-Boot environment
log_info "Configuring U-Boot environment for dual boot..."

# Construct console port
CONSOLE_PORT="ttyS${CONSOLE_DEV}"

# Construct kernel command line arguments (linuxargs)
LINUXARGS="console=${CONSOLE_PORT},${CONSOLE_SPEED}n8 ${EARLYCON} loopfstype=squashfs loop=${IMAGE_DIR}/fs.squashfs varlog_size=${VAR_LOG_SIZE}"

# FIT image path
FIT_NAME="${IMAGE_DIR}/boot/sonic_arm64.fit"

# Partition number (typically 1 for single partition setup)
DEMO_PART="1"

# Disk interface (mmc for eMMC)
DISK_INTERFACE="mmc"

log_info "Setting U-Boot variables..."

# Image configuration (slot 1 - current image)
fw_setenv image_dir "$IMAGE_DIR" || { log_error "Failed to set image_dir"; exit 1; }
fw_setenv fit_name "$FIT_NAME" || { log_error "Failed to set fit_name"; exit 1; }
fw_setenv sonic_version_1 "$SONIC_VERSION" || { log_error "Failed to set sonic_version_1"; exit 1; }

# Old/backup image (slot 2 - empty for first installation)
fw_setenv image_dir_old "" || { log_error "Failed to set image_dir_old"; exit 1; }
fw_setenv fit_name_old "" || { log_error "Failed to set fit_name_old"; exit 1; }
fw_setenv sonic_version_2 "None" || { log_error "Failed to set sonic_version_2"; exit 1; }
fw_setenv linuxargs_old "" || { log_error "Failed to set linuxargs_old"; exit 1; }

# Kernel command line arguments
fw_setenv linuxargs "$LINUXARGS" || { log_error "Failed to set linuxargs"; exit 1; }

# Boot commands
fw_setenv sonic_boot_load "ext4load ${DISK_INTERFACE} 0:${DEMO_PART} \${loadaddr} \${fit_name}" || { log_error "Failed to set sonic_boot_load"; exit 1; }
fw_setenv sonic_boot_load_old "ext4load ${DISK_INTERFACE} 0:${DEMO_PART} \${loadaddr} \${fit_name_old}" || { log_error "Failed to set sonic_boot_load_old"; exit 1; }
fw_setenv sonic_bootargs "setenv bootargs root=UUID=${UUID} rw rootwait panic=1 \${linuxargs}" || { log_error "Failed to set sonic_bootargs"; exit 1; }
fw_setenv sonic_bootargs_old "setenv bootargs root=UUID=${UUID} rw rootwait panic=1 \${linuxargs_old}" || { log_error "Failed to set sonic_bootargs_old"; exit 1; }
fw_setenv sonic_image_1 "run sonic_bootargs; run sonic_boot_load; bootm \${loadaddr}#conf-\${bootconf}" || { log_error "Failed to set sonic_image_1"; exit 1; }
fw_setenv sonic_image_2 "run sonic_bootargs_old; run sonic_boot_load_old; bootm \${loadaddr}#conf-\${bootconf}" || { log_error "Failed to set sonic_image_2"; exit 1; }

# Boot menu with instructions
fw_setenv print_menu "echo ===================================================; echo SONiC Boot Menu; echo ===================================================; echo To boot \$sonic_version_1; echo   type: run sonic_image_1; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo To boot \$sonic_version_2; echo   type: run sonic_image_2; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo ===================================================" || { log_error "Failed to set print_menu"; exit 1; }

# Boot configuration
fw_setenv boot_next "run sonic_image_1" || { log_error "Failed to set boot_next"; exit 1; }
fw_setenv bootcmd "run print_menu; test -n \"\$boot_once\" && setenv do_boot_once \"\$boot_once\" && setenv boot_once \"\" && saveenv && run do_boot_once; run boot_next" || { log_error "Failed to set bootcmd"; exit 1; }

# Set bootconf (platform identifier)
fw_setenv bootconf "$BOOTCONF" || { log_error "Failed to set bootconf"; exit 1; }

# Memory addresses (from platform_arm64.conf)
fw_setenv loadaddr "0x432000000" || { log_error "Failed to set loadaddr"; exit 1; }
fw_setenv kernel_addr "0x403000000" || { log_error "Failed to set kernel_addr"; exit 1; }
fw_setenv fdt_addr "0x44C000000" || { log_error "Failed to set fdt_addr"; exit 1; }
fw_setenv initrd_addr "0x440000000" || { log_error "Failed to set initrd_addr"; exit 1; }

log_info "✓ All U-Boot variables set successfully"

# Verify U-Boot environment
log_info ""
log_info "Verifying U-Boot environment..."
log_info "================================"

VERIFY_FAILED=0

# Verify critical variables
for var in image_dir fit_name sonic_version_1 linuxargs bootconf sonic_image_1 boot_next bootcmd; do
    VALUE=$(fw_printenv -n $var 2>/dev/null || echo "")
    if [ -z "$VALUE" ]; then
        log_error "Variable '$var' is not set!"
        VERIFY_FAILED=1
    else
        log_info "✓ $var: ${VALUE:0:60}$([ ${#VALUE} -gt 60 ] && echo '...')"
    fi
done

log_info ""

if [ $VERIFY_FAILED -eq 1 ]; then
    log_error "U-Boot environment verification failed!"
    exit 1
fi

# Installation complete
log_info "================================"
log_info "SONiC installation completed successfully!"
log_info ""
log_info "Summary:"
log_info "  - SONiC version: $SONIC_VERSION"
log_info "  - Image directory: $IMAGE_DIR"
log_info "  - Partition: ${EMMC_PARTITION} (UUID: $UUID)"
log_info "  - Boot configuration: $BOOTCONF"
log_info "  - Console: $CONSOLE_PORT @ ${CONSOLE_SPEED}n8"
log_info "  - Default boot: sonic_image_1"
log_info ""
log_info "Boot configuration:"
log_info "  - Image 1 (sonic_version_1): $SONIC_VERSION"
log_info "  - Image 2 (sonic_version_2): None"
log_info ""
log_info "Next steps:"
log_info "  1. Reboot the system: 'reboot'"
log_info "  2. U-Boot will boot SONiC from eMMC"
log_info "  3. After boot, verify with: 'show version'"
log_info ""
log_warn "Note: Ensure U-Boot is configured to boot from eMMC!"
log_info ""

exit 0

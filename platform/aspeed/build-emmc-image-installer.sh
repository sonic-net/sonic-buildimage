#!/bin/bash
# Build Complete SONiC eMMC Image for AST2700 Using SONiC Installer
# This creates a ready-to-flash disk image by leveraging the built-in SONiC installer
#
# Usage: ./build-emmc-image-installer.sh <sonic-aspeed-arm64.bin> [output-image-name]
#
# This script uses the SONiC installer's "build" mode to create a raw ext4 image,
# then embeds it into a full GPT-partitioned eMMC image with a single SONiC-OS partition.
#
# Partition Layout (Standard SONiC Approach):
#   /dev/mmcblk0p1 - SONiC-OS (entire disk 10 GB - assumes pSLC on a 32G eMMC)
#     ├── image-SONiC-OS-<version1>/
#     ├── image-SONiC-OS-<version2>/
#     └── machine.conf
#
# Requirements:
#   - Root privileges (for loop device mounting)
#   - sgdisk (gdisk package)
#   - mkfs.ext4 (e2fsprogs package)
#   - bash, unzip, tar, gzip

set -e  # Exit on error

# Configuration
# We use 10GB as the 32G eMMC card in chipmunk will be 10G post pSLC partitioning
EMMC_SIZE_MB=10240
PARTITION_LABEL="SONiC-OS"  # Single partition for all SONiC images
ROOT_PART_SIZE_MB=$((EMMC_SIZE_MB - 100))  # SONiC-OS partition size (leave 100MB for GPT overhead)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_requirements() {
    log_info "Checking requirements..."
    local missing=0
    for cmd in sgdisk mkfs.ext4 losetup bash unzip tar gzip; do
        if ! command -v $cmd &> /dev/null; then
            log_error "Required command not found: $cmd"
            missing=1
        fi
    done
    [ $missing -eq 1 ] && exit 1
    [ "$(id -u)" -ne 0 ] && { log_error "Must run as root"; exit 1; }
    log_info "All requirements satisfied"
}

cleanup() {
    log_info "Cleaning up..."
    [ -n "$LOOP_DEV" ] && losetup -d "$LOOP_DEV" 2>/dev/null || true
    [ -n "$RAW_IMAGE" ] && [ -f "$RAW_IMAGE" ] && rm -f "$RAW_IMAGE" || true
}

# Parse arguments
[ $# -lt 1 ] && { echo "Usage: $0 <sonic-aspeed-arm64.bin> [output-image-name]"; exit 1; }

SONIC_BIN="$(realpath "$1")"
OUTPUT_IMAGE="${2:-sonic-aspeed-arm64-emmc.img}"

[ ! -f "$SONIC_BIN" ] && { log_error "SONiC binary not found: $SONIC_BIN"; exit 1; }

check_requirements
trap cleanup EXIT

ORIG_DIR=$(pwd)
LOOP_DEV=""
RAW_IMAGE="target/sonic-aspeed.raw"

# Create target directory if it doesn't exist
mkdir -p target

log_info "Building eMMC image from: $SONIC_BIN"
log_info "Output image: $OUTPUT_IMAGE"

# Step 1: Use SONiC installer in "build" mode to create raw ext4 image
log_info "Running SONiC installer in 'build' mode..."
log_info "This will create a raw ext4 image with SONiC installed..."

# The installer expects to be run as a self-extracting archive
# In "build" mode, it creates OUTPUT_RAW_IMAGE
# We need to:
# 1. Pre-create the raw image file with the right size (installer expects it to exist)
# 2. Patch the installer to use our filename

# Pre-create the raw image file (same as build_image.sh does)
log_info "Creating raw image file: $RAW_IMAGE (${ROOT_PART_SIZE_MB} MB)..."
fallocate -l "${ROOT_PART_SIZE_MB}M" "$RAW_IMAGE" || {
    log_error "Failed to create raw image file"
    exit 1
}

# Run the installer directly
# It will detect install_env="build" and use the pre-created raw image
# The installer already has the correct path: target/sonic-aspeed.raw
log_info "Running installer (this may take a few minutes)..."
bash "$SONIC_BIN" || {
    log_error "Installer failed"
    exit 1
}

# Verify raw image was created
if [ ! -f "$RAW_IMAGE" ]; then
    log_error "Raw image not created by installer"
    exit 1
fi

log_info "Raw SONiC image created: $RAW_IMAGE ($(du -h $RAW_IMAGE | cut -f1))"

# Step 2: Mark first boot in the raw image
# Note: machine.conf will be created by first-boot service based on DTB detection
log_info "Marking first boot in raw image..."
TEMP_MOUNT=$(mktemp -d)
mount -o loop "$RAW_IMAGE" "$TEMP_MOUNT"

touch "$TEMP_MOUNT/.first_boot_from_prebuilt_image"

umount "$TEMP_MOUNT"
rmdir "$TEMP_MOUNT"

# Step 3: Create full eMMC disk image with GPT partitions
DISK_IMAGE="${OUTPUT_IMAGE%.gz}"
DISK_IMAGE="${DISK_IMAGE%.img}.img"

log_info "Creating eMMC disk image (${EMMC_SIZE_MB} MB)..."
dd if=/dev/zero of="$DISK_IMAGE" bs=1M count=$EMMC_SIZE_MB

log_info "Creating GPT partition table..."
sgdisk -o "$DISK_IMAGE"

log_info "Creating single SONiC-OS partition using entire disk..."
sgdisk -n 1:0:0 -t 1:8300 -c 1:"$PARTITION_LABEL" "$DISK_IMAGE"

log_info "Setting up loop device..."
LOOP_DEV=$(losetup -f --show -P "$DISK_IMAGE")
log_info "Loop device: $LOOP_DEV"
sleep 1
partprobe "$LOOP_DEV" || true
sleep 1

log_info "Copying SONiC installation to SONiC-OS partition..."
dd if="$RAW_IMAGE" of="${LOOP_DEV}p1" bs=128M conv=sparse oflag=direct status=progress

log_info "Cleaning up..."
losetup -d "$LOOP_DEV"
LOOP_DEV=""
rm -f "$RAW_IMAGE"
RAW_IMAGE=""

log_info "Compressing image..."
gzip -f "$DISK_IMAGE"

log_info "eMMC image created successfully: ${DISK_IMAGE}.gz"
log_info ""
log_info "Image size: $(du -h ${DISK_IMAGE}.gz | cut -f1)"


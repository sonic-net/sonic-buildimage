#!/bin/bash
# SONiC U-Boot Environment Initialization Script
# This script runs once on first boot to set U-Boot environment variables
# Required for sonic-installer to work properly

set -e

LOG_FILE="/var/log/sonic-uboot-env-init.log"
MARKER_FILE="/etc/sonic/.uboot-env-initialized"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

# Check if already initialized
if [ -f "$MARKER_FILE" ]; then
    log "U-Boot environment already initialized. Exiting."
    exit 0
fi

log "Starting U-Boot environment initialization..."

# Check if fw_setenv is available
if ! command -v fw_setenv &> /dev/null; then
    log "ERROR: fw_setenv not found. Cannot initialize U-Boot environment."
    exit 1
fi

# Wait for /proc/mtd to be available (up to 10 seconds)
# MTD devices may not be ready immediately during early boot
WAIT_COUNT=0
MAX_WAIT=10
while [ ! -f /proc/mtd ] && [ $WAIT_COUNT -lt $MAX_WAIT ]; do
    log "Waiting for /proc/mtd to be available... ($WAIT_COUNT/$MAX_WAIT)"
    sleep 1
    WAIT_COUNT=$((WAIT_COUNT + 1))
done

if [ ! -f /proc/mtd ]; then
    log "WARNING: /proc/mtd not available after ${MAX_WAIT}s, will use default configuration"
fi

# Create or update /etc/fw_env.config
# Always try to detect the best configuration, even if file exists
log "Configuring U-Boot environment access..."

# Check for MTD device first by parsing /proc/mtd
# Example line: mtd1: 00020000 00010000 "u-boot-env"
MTD_ENV_LINE=$(grep -E '"u-boot-env"|"uboot-env"' /proc/mtd 2>/dev/null || true)

if [ -n "$MTD_ENV_LINE" ]; then
    # Parse MTD device name, size, and erasesize from /proc/mtd
    MTD_DEV=$(echo "$MTD_ENV_LINE" | awk -F: '{print $1}')
    MTD_SIZE=$(echo "$MTD_ENV_LINE" | awk '{print "0x" $2}')
    MTD_ERASESIZE=$(echo "$MTD_ENV_LINE" | awk '{print "0x" $3}')

    if [ -c "/dev/$MTD_DEV" ]; then
        FW_ENV_CONFIG="/dev/$MTD_DEV 0x0 $MTD_SIZE $MTD_ERASESIZE"
        log "Detected U-Boot env from /proc/mtd: $FW_ENV_CONFIG"
    fi
fi

# If not found in MTD, try device tree
if [ -z "$FW_ENV_CONFIG" ]; then
    DTB_HAS_ENV_BLK=$(grep -E "uboot-env|u-boot-env" /proc/mtd 2>/dev/null | sed -e 's/:.*$//' || true)
    if [ -n "$DTB_HAS_ENV_BLK" ] && [ -c "/dev/$DTB_HAS_ENV_BLK" ]; then
        PROC_ENV_FILE=$(find /proc/device-tree/ -name env_size 2>/dev/null || true)
        if [ -n "$PROC_ENV_FILE" ]; then
            UBOOT_ENV_SIZ="0x$(hd $PROC_ENV_FILE | awk 'FNR==1 {print $2 $3 $4 $5}')"
            UBOOT_ENV_ERASE_SIZ="0x$(grep -E "uboot-env|u-boot-env" /proc/mtd | awk '{print $3}')"
            if [[ -n "$UBOOT_ENV_SIZ" && -n "$UBOOT_ENV_ERASE_SIZ" ]]; then
                FW_ENV_CONFIG="/dev/$DTB_HAS_ENV_BLK 0x00000000 $UBOOT_ENV_SIZ $UBOOT_ENV_ERASE_SIZ"
                log "Detected U-Boot env from device tree: $FW_ENV_CONFIG"
            fi
        fi
    fi
fi

# If still not found, use eMMC default location
if [ -z "$FW_ENV_CONFIG" ]; then
    # AST2700 default: U-Boot environment on eMMC at 31.25 MB offset
    FW_ENV_CONFIG="/dev/mmcblk0 0x1F40000 0x20000 0x1000"
    log "Using default eMMC U-Boot env location: $FW_ENV_CONFIG"
fi

# Update /etc/fw_env.config if it's different from current content
if [ -f /etc/fw_env.config ]; then
    CURRENT_CONFIG=$(cat /etc/fw_env.config)
    if [ "$CURRENT_CONFIG" != "$FW_ENV_CONFIG" ]; then
        log "Updating /etc/fw_env.config (was: $CURRENT_CONFIG)"
        echo "$FW_ENV_CONFIG" > /etc/fw_env.config
        log "Updated /etc/fw_env.config: $FW_ENV_CONFIG"
    else
        log "Using existing /etc/fw_env.config: $FW_ENV_CONFIG"
    fi
else
    echo "$FW_ENV_CONFIG" > /etc/fw_env.config
    log "Created /etc/fw_env.config: $FW_ENV_CONFIG"
fi

# Check if sonic_version_1 is already set
if fw_printenv sonic_version_1 2>/dev/null | grep -q "SONiC-OS-"; then
    log "U-Boot environment already configured (sonic_version_1 is set)"
    mkdir -p "$(dirname "$MARKER_FILE")"
    touch "$MARKER_FILE"
    exit 0
fi

# Detect current image directory
IMAGE_DIR=$(ls -d /host/image-* 2>/dev/null | head -1 | xargs basename)
if [ -z "$IMAGE_DIR" ]; then
    log "ERROR: Cannot find image directory in /host/"
    exit 1
fi

log "Detected image directory: $IMAGE_DIR"

# Get partition UUID
PART_UUID=$(blkid -s PARTUUID -o value /dev/mmcblk0p1 2>/dev/null || echo "")
if [ -z "$PART_UUID" ]; then
    log "WARNING: Cannot detect partition UUID, using device path instead"
    ROOT_DEV="/dev/mmcblk0p1"
else
    ROOT_DEV="UUID=$PART_UUID"
fi

# Extract version from image directory
SONIC_VERSION=$(echo "$IMAGE_DIR" | sed 's/^image-/SONiC-OS-/')

log "Setting U-Boot environment variables..."

# Set U-Boot environment variables
# Note: Using -f flag to force write even if environment is not initialized
FW_ARG="-f"

# Image configuration
fw_setenv $FW_ARG image_dir "$IMAGE_DIR"
fw_setenv $FW_ARG fit_name "$IMAGE_DIR/boot/sonic_arm64.fit"
fw_setenv $FW_ARG sonic_version_1 "$SONIC_VERSION"

# Old/backup image (none for first installation)
fw_setenv $FW_ARG image_dir_old ""
fw_setenv $FW_ARG fit_name_old ""
fw_setenv $FW_ARG sonic_version_2 "None"
fw_setenv $FW_ARG linuxargs_old ""

# Kernel command line arguments
fw_setenv $FW_ARG linuxargs "console=ttyS12,115200n8 earlycon=uart8250,mmio32,0x14c33b00 loopfstype=squashfs loop=$IMAGE_DIR/fs.squashfs varlog_size=4096"

# Boot commands
fw_setenv $FW_ARG sonic_boot_load "ext4load mmc 0:1 \${loadaddr} \$fit_name"
fw_setenv $FW_ARG sonic_boot_load_old "ext4load mmc 0:1 \${loadaddr} \$fit_name_old"
fw_setenv $FW_ARG sonic_bootargs "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs}"
fw_setenv $FW_ARG sonic_bootargs_old "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs_old}"
fw_setenv $FW_ARG sonic_image_1 "run sonic_bootargs; run sonic_boot_load; bootm \${loadaddr}#conf-ast2700-evb.dtb"
fw_setenv $FW_ARG sonic_image_2 ""

# Boot menu with instructions
fw_setenv $FW_ARG print_menu "echo ===================================================; echo SONiC Boot Menu; echo ===================================================; echo To boot \$sonic_version_1; echo   type: run sonic_image_1; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo To boot \$sonic_version_2; echo   type: run sonic_image_2; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo ==================================================="

# Boot configuration
fw_setenv $FW_ARG boot_next "run sonic_image_1"
fw_setenv $FW_ARG bootcmd "run print_menu; test -n \"\$boot_once\" && setenv do_boot_once \"\$boot_once\" && setenv boot_once \"\" && saveenv && run do_boot_once; run boot_next"

# Memory addresses
fw_setenv $FW_ARG loadaddr "0x432000000"
fw_setenv $FW_ARG kernel_addr "0x403000000"
fw_setenv $FW_ARG fdt_addr "0x44C000000"
fw_setenv $FW_ARG initrd_addr "0x440000000"

log "U-Boot environment variables set successfully"

# Create marker file to prevent re-initialization
mkdir -p "$(dirname "$MARKER_FILE")"
touch "$MARKER_FILE"

log "U-Boot environment initialization complete"

exit 0


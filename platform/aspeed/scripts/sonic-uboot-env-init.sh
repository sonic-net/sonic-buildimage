#!/bin/bash
# SONiC U-Boot Environment Initialization Script
# This script has two parts:
# 1. Always create /etc/fw_env.config (runs every boot for every image)
# 2. Set U-Boot environment variables (runs once globally, controlled by marker file)

set -e

LOG_FILE="/var/log/sonic-uboot-env-init.log"
MARKER_FILE="/host/.uboot-env-initialized"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log "Starting U-Boot environment configuration..."

# Check if fw_setenv is available
if ! command -v fw_setenv &> /dev/null; then
    log "ERROR: fw_setenv not found. Cannot configure U-Boot environment."
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

#############################################################################
# PART 1: Create /etc/fw_env.config (ALWAYS runs, every boot, every image)
#############################################################################
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

#############################################################################
# PART 2: Set U-Boot environment variables (runs ONCE globally)
#############################################################################

# Check if already initialized
if [ -f "$MARKER_FILE" ]; then
    log "U-Boot environment already initialized (marker file exists). Skipping U-Boot env setup."
    log "fw_env.config configuration complete."
    exit 0
fi

log "U-Boot environment not yet initialized. Proceeding with first-time setup..."

# Detect current image directory from /proc/cmdline
# The kernel command line contains "loop=image-xxx/fs.squashfs"
IMAGE_DIR=$(grep -o 'loop=[^ ]*' /proc/cmdline | sed 's|loop=\([^/]*\)/.*|\1|')
if [ -z "$IMAGE_DIR" ]; then
    log "WARNING: Cannot detect image from /proc/cmdline, trying fallback method"
    # Fallback: use first image directory (may not be correct for multi-image systems)
    IMAGE_DIR=$(ls -d /host/image-* 2>/dev/null | head -1 | xargs basename)
    if [ -z "$IMAGE_DIR" ]; then
        log "ERROR: Cannot find image directory in /host/"
        exit 1
    fi
    log "Using fallback image directory: $IMAGE_DIR"
else
    log "Detected image directory from /proc/cmdline: $IMAGE_DIR"
fi

# Get filesystem UUID (not partition UUID)
FS_UUID=$(blkid -s UUID -o value /dev/mmcblk0p1 2>/dev/null || echo "")
if [ -z "$FS_UUID" ]; then
    log "WARNING: Cannot detect filesystem UUID, using device path instead"
    ROOT_DEV="/dev/mmcblk0p1"
else
    ROOT_DEV="UUID=$FS_UUID"
fi

# Extract version from image directory
SONIC_VERSION=$(echo "$IMAGE_DIR" | sed 's/^image-/SONiC-OS-/')

log "Setting U-Boot environment variables..."

# Get DTB configuration name from U-Boot bootconf variable
# bootconf is set by U-Boot based on hardware detection
BOOTCONF=$(fw_printenv -n bootconf 2>/dev/null || echo "")
if [ -z "$BOOTCONF" ]; then
    log "WARNING: U-Boot bootconf variable not set, using default: ast2700-evb"
    BOOTCONF="ast2700-evb"
fi
log "Using U-Boot bootconf: $BOOTCONF"

# Set U-Boot environment variables

# Image configuration
fw_setenv image_dir "$IMAGE_DIR" || log "ERROR: Failed to set image_dir"
fw_setenv fit_name "$IMAGE_DIR/boot/sonic_arm64.fit" || log "ERROR: Failed to set fit_name"
fw_setenv sonic_version_1 "$SONIC_VERSION" || log "ERROR: Failed to set sonic_version_1"

# Old/backup image (none for first installation)
fw_setenv image_dir_old "" || log "ERROR: Failed to set image_dir_old"
fw_setenv fit_name_old "" || log "ERROR: Failed to set fit_name_old"
fw_setenv sonic_version_2 "None" || log "ERROR: Failed to set sonic_version_2"
fw_setenv linuxargs_old "" || log "ERROR: Failed to set linuxargs_old"

# Kernel command line arguments
fw_setenv linuxargs "console=ttyS12,115200n8 earlycon=uart8250,mmio32,0x14c33b00 loopfstype=squashfs loop=$IMAGE_DIR/fs.squashfs varlog_size=4096" || log "ERROR: Failed to set linuxargs"

# Boot commands
fw_setenv sonic_boot_load "ext4load mmc 0:1 \${loadaddr} \${fit_name}" || log "ERROR: Failed to set sonic_boot_load"
fw_setenv sonic_boot_load_old "ext4load mmc 0:1 \${loadaddr} \${fit_name_old}" || log "ERROR: Failed to set sonic_boot_load_old"
fw_setenv sonic_bootargs "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs}" || log "ERROR: Failed to set sonic_bootargs"
fw_setenv sonic_bootargs_old "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs_old}" || log "ERROR: Failed to set sonic_bootargs_old"
fw_setenv sonic_image_1 "run sonic_bootargs; run sonic_boot_load; bootm \${loadaddr}#conf-\${bootconf}" || log "ERROR: Failed to set sonic_image_1"
fw_setenv sonic_image_2 "run sonic_bootargs_old; run sonic_boot_load_old; bootm \${loadaddr}#conf-\${bootconf}" || log "ERROR: Failed to set sonic_image_2"

# Boot menu with instructions
fw_setenv print_menu "echo ===================================================; echo SONiC Boot Menu; echo ===================================================; echo To boot \$sonic_version_1; echo   type: run sonic_image_1; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo To boot \$sonic_version_2; echo   type: run sonic_image_2; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo ===================================================" || log "ERROR: Failed to set print_menu"

# Boot configuration
fw_setenv boot_next "run sonic_image_1" || log "ERROR: Failed to set boot_next"
fw_setenv bootcmd "run print_menu; test -n \"\$boot_once\" && setenv do_boot_once \"\$boot_once\" && setenv boot_once \"\" && saveenv && run do_boot_once; run boot_next" || log "ERROR: Failed to set bootcmd"

# Memory addresses
fw_setenv loadaddr "0x432000000" || log "ERROR: Failed to set loadaddr"
fw_setenv kernel_addr "0x403000000" || log "ERROR: Failed to set kernel_addr"
fw_setenv fdt_addr "0x44C000000" || log "ERROR: Failed to set fdt_addr"
fw_setenv initrd_addr "0x440000000" || log "ERROR: Failed to set initrd_addr"

log "U-Boot environment variables set successfully"

# Create marker file to prevent re-initialization
mkdir -p "$(dirname "$MARKER_FILE")"
touch "$MARKER_FILE"

log "U-Boot environment initialization complete"

exit 0


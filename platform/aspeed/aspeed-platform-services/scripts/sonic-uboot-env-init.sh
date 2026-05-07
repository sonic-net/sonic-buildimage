#!/bin/sh
# SONiC U-Boot Environment Initialization Script
#
# Host (default): systemd; /etc/fw_env.config every boot; program U-Boot once (marker on /host).
# Installer: TFTP/raw eMMC initramfs after install; same flow with /sbin helpers. Enable with
#   SONIC_UBOOT_ENV_INSTALLER=1 (TFTP init sets this) or pass --installer.

set -e

installer_mode() {
    case "${SONIC_UBOOT_ENV_INSTALLER:-}" in
    1|yes|true) return 0 ;;
    esac
    for a in "$@"; do
        [ "$a" = "--installer" ] && return 0
    done
    return 1
}

if installer_mode "$@"; then
    INSTALLER_MODE=1
    LOG_FILE="${SONIC_UBOOT_ENV_LOG_FILE:-/tmp/sonic-uboot-env-init.log}"
    export FW_ENV_MSG_PREFIX="${FW_ENV_MSG_PREFIX:-sonic-uboot-env-init-installer}"
    export SONIC_MACHINE_CONF="${SONIC_MACHINE_CONF:-/tmp/machine.conf}"
else
    INSTALLER_MODE=0
    LOG_FILE="${SONIC_UBOOT_ENV_LOG_FILE:-/var/log/sonic-uboot-env-init.log}"
    export FW_ENV_MSG_PREFIX="${FW_ENV_MSG_PREFIX:-sonic-uboot-env-init}"
fi

MARKER_FILE="/host/.uboot-env-initialized"
EMMC="${EMMC:-/dev/mmcblk0}"
MNT="${MNT:-/mnt}"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE" 2>/dev/null || echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >&2
}

mkdir -p "$(dirname "$LOG_FILE")" 2>/dev/null || true

log "Starting U-Boot environment configuration..."

resolve_fw_env_config_sh() {
    if [ -x /usr/bin/sonic-fw-env-config.sh ]; then
        SONIC_FW_ENV_CONFIG_SH=/usr/bin/sonic-fw-env-config.sh
    elif [ -x /sbin/sonic-fw-env-config.sh ]; then
        SONIC_FW_ENV_CONFIG_SH=/sbin/sonic-fw-env-config.sh
    else
        return 1
    fi
    return 0
}

resolve_program_uboot_sh() {
    if [ -x /usr/bin/sonic-program-uboot-env.sh ]; then
        SONIC_PROGRAM_UBOOT_ENV_SH=/usr/bin/sonic-program-uboot-env.sh
    elif [ -x /sbin/sonic-program-uboot-env.sh ]; then
        SONIC_PROGRAM_UBOOT_ENV_SH=/sbin/sonic-program-uboot-env.sh
    else
        return 1
    fi
    return 0
}

run_fw_env_config() {
    export SONIC_FW_ENV_LOG_FILE="$LOG_FILE"
    "$SONIC_FW_ENV_CONFIG_SH"
}

run_installer_uboot_programming() {
    if ! resolve_program_uboot_sh; then
        umount "$MNT" 2>/dev/null || true
        log "ERROR: sonic-program-uboot-env.sh not found in /usr/bin or /sbin."
        exit 1
    fi

    export UBOOT_ENV_BOOT_DEVICE="$BOOT_PART"
    export UBOOT_ENV_DEMO_DEV="$EMMC"
    export UBOOT_ENV_DEMO_PART=1
    export UBOOT_ENV_DISK_INTERFACE=mmc
    export UBOOT_ENV_IMAGE_DIR="$IMAGE_DIR"
    export UBOOT_ENV_INSTALLER_CONF="$MNT/$IMAGE_DIR/installer.conf"
    export SONIC_UBOOT_ENV_LOG_FILE="$LOG_FILE"
    if "$SONIC_PROGRAM_UBOOT_ENV_SH"; then
        log "U-Boot environment updated."
    else
        log "Warning: sonic-program-uboot-env.sh failed (see messages above)."
    fi
}

if [ "$INSTALLER_MODE" = 1 ]; then
    if ! resolve_fw_env_config_sh; then
        log "ERROR: sonic-fw-env-config.sh not found in /usr/bin or /sbin."
        exit 1
    fi
    run_fw_env_config

    if ! command -v fw_setenv >/dev/null 2>&1 || ! command -v fw_printenv >/dev/null 2>&1; then
        log "Warning: fw_setenv/fw_printenv not available; skipping U-Boot env update."
        exit 0
    fi

    sync
    blockdev --rereadpt "$EMMC" 2>/dev/null || true
    command -v partprobe >/dev/null 2>&1 && partprobe "$EMMC" 2>/dev/null || true
    sleep 1

    BOOT_PART=$(blkid -L SONiC-OS 2>/dev/null || true)
    if [ -z "$BOOT_PART" ] && [ -b "${EMMC}p1" ]; then
        BOOT_PART="${EMMC}p1"
    fi
    if [ -z "$BOOT_PART" ] || [ ! -b "$BOOT_PART" ]; then
        log "Warning: SONiC-OS partition not found; skipping U-Boot variable programming."
        exit 0
    fi

    mkdir -p "$MNT"
    if ! mount -t ext4 -o ro "$BOOT_PART" "$MNT" 2>/dev/null; then
        log "Warning: could not mount $BOOT_PART; skipping U-Boot variable programming."
        exit 0
    fi

    IMAGE_DIR=""
    for d in "$MNT"/image-*; do
        [ ! -e "$d" ] && continue
        [ -d "$d" ] || continue
        IMAGE_DIR=$(basename "$d")
        break
    done
    if [ -z "$IMAGE_DIR" ]; then
        umount "$MNT" 2>/dev/null || true
        log "Warning: no image-* directory on $BOOT_PART; skipping U-Boot variable programming."
        exit 0
    fi

    run_installer_uboot_programming
    umount "$MNT" 2>/dev/null || true
    exit 0
fi

if ! command -v fw_setenv >/dev/null 2>&1; then
    log "ERROR: fw_setenv not found. Cannot configure U-Boot environment."
    exit 1
fi

if ! resolve_fw_env_config_sh; then
    log "ERROR: sonic-fw-env-config.sh not found in /usr/bin or /sbin."
    exit 1
fi

log "Configuring U-Boot environment access..."
run_fw_env_config

if blkid -L "ONIE" >/dev/null 2>&1 || blkid -L "ONIE-BOOT" >/dev/null 2>&1; then
    echo "ONIE partition found. Skipping U-Boot environment configurations."
    exit 0
fi

if [ -f "$MARKER_FILE" ]; then
    log "U-Boot environment already initialized (marker file exists). Skipping U-Boot env setup."
    log "fw_env.config configuration complete."
    exit 0
fi

log "U-Boot environment not yet initialized. Proceeding with first-time setup..."

boot_device=$(findmnt -n -o SOURCE /host)
demo_dev=$(echo "$boot_device" | sed 's/p\?[0-9]*$//')
demo_part=$(echo "$boot_device" | grep -o '[0-9]*$')

if [ -z "$boot_device" ]; then
    boot_device="/dev/mmcblk0p1"
    demo_dev="/dev/mmcblk0"
    demo_part=1
    log "WARNING: Could not detect boot device, defaulting to $boot_device"
fi

if echo "$demo_dev" | grep -q "mmc"; then
    disk_interface="mmc"
else
    disk_interface="scsi"
fi

log "Detected boot device: ${boot_device}, partition: ${demo_part}, interface: ${disk_interface}"

IMAGE_DIR=$(grep -o 'loop=[^ ]*' /proc/cmdline | sed 's|loop=\([^/]*\)/.*|\1|')
if [ -z "$IMAGE_DIR" ]; then
    log "WARNING: Cannot detect image from /proc/cmdline, trying fallback method"
    IMAGE_DIR=$(ls -d /host/image-* 2>/dev/null | head -1 | xargs basename)
    if [ -z "$IMAGE_DIR" ]; then
        log "ERROR: Cannot find image directory in /host/"
        exit 1
    fi
    log "Using fallback image directory: $IMAGE_DIR"
else
    log "Detected image directory from /proc/cmdline: $IMAGE_DIR"
fi

if ! resolve_program_uboot_sh; then
    log "ERROR: sonic-program-uboot-env.sh not found in /usr/bin or /sbin."
    exit 1
fi

export UBOOT_ENV_BOOT_DEVICE="$boot_device"
export UBOOT_ENV_DEMO_DEV="$demo_dev"
export UBOOT_ENV_DEMO_PART="$demo_part"
export UBOOT_ENV_DISK_INTERFACE="$disk_interface"
export UBOOT_ENV_IMAGE_DIR="$IMAGE_DIR"
export UBOOT_ENV_INSTALLER_CONF="/host/${IMAGE_DIR}/installer.conf"
export SONIC_UBOOT_ENV_LOG_FILE="$LOG_FILE"
"$SONIC_PROGRAM_UBOOT_ENV_SH"

log "U-Boot environment variables set successfully"

mkdir -p "$(dirname "$MARKER_FILE")"
touch "$MARKER_FILE"

log "U-Boot environment initialization complete"

exit 0

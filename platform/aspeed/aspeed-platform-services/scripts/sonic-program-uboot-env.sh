#!/bin/sh
# Program U-Boot variables for SONiC ext4 + FIT boot. Used by sonic-uboot-env-init (host first boot and
# TFTP initrd with SONIC_UBOOT_ENV_INSTALLER=1).
#
# Required environment:
#   UBOOT_ENV_BOOT_DEVICE     root block device (e.g. /dev/mmcblk0p1) for blkid UUID
#   UBOOT_ENV_DEMO_DEV       whole-disk device (e.g. /dev/mmcblk0)
#   UBOOT_ENV_DEMO_PART      partition index for U-Boot ext4load (e.g. 1)
#   UBOOT_ENV_DISK_INTERFACE  mmc | scsi
#   UBOOT_ENV_IMAGE_DIR       e.g. image-<version>
# Optional:
#   UBOOT_ENV_INSTALLER_CONF  if set and exists, sourced before applying defaults
#   SONIC_UBOOT_ENV_LOG_FILE  append timestamped lines (same style as other SONiC logs)

sonic_uboot_env_log() {
    msg="$*"
    echo "$msg" >&2
    if [ -n "${SONIC_UBOOT_ENV_LOG_FILE:-}" ]; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] $msg" >> "$SONIC_UBOOT_ENV_LOG_FILE"
    fi
}

for v in UBOOT_ENV_BOOT_DEVICE UBOOT_ENV_DEMO_DEV UBOOT_ENV_DEMO_PART UBOOT_ENV_DISK_INTERFACE UBOOT_ENV_IMAGE_DIR; do
    eval "sonic_uboot_chk=\${$v-}"
    if [ -z "$sonic_uboot_chk" ]; then
        sonic_uboot_env_log "ERROR: $v is not set"
        exit 1
    fi
done

if ! command -v fw_setenv >/dev/null 2>&1 || ! command -v fw_printenv >/dev/null 2>&1; then
    sonic_uboot_env_log "ERROR: fw_setenv or fw_printenv not available"
    exit 1
fi

if ! fw_printenv bootcmd >/dev/null 2>&1; then
    sonic_uboot_env_log "ERROR: fw_printenv cannot read U-Boot env (check /etc/fw_env.config)"
    exit 1
fi

if [ -n "${UBOOT_ENV_INSTALLER_CONF:-}" ] && [ -f "$UBOOT_ENV_INSTALLER_CONF" ]; then
    . "$UBOOT_ENV_INSTALLER_CONF" || true
fi

CONSOLE_DEV=${CONSOLE_DEV:-12}
CONSOLE_SPEED=${CONSOLE_SPEED:-115200}
EARLYCON=${EARLYCON:-"earlycon=uart8250,mmio32,0x14c33b00"}
VAR_LOG_SIZE=${VAR_LOG_SIZE:-128} # 128MB for /var/log tmpfs
CONSOLE_PORT="ttyS${CONSOLE_DEV}"

FS_ROOT_DEV=$UBOOT_ENV_BOOT_DEVICE
FS_UUID=$(blkid -s UUID -o value "$FS_ROOT_DEV" 2>/dev/null || true)
if [ -z "$FS_UUID" ]; then
    sonic_uboot_env_log "WARNING: Cannot detect filesystem UUID for $FS_ROOT_DEV; using device path for root="
    ROOT_DEV="$FS_ROOT_DEV"
else
    ROOT_DEV="UUID=$FS_UUID"
fi

IMAGE_DIR=$UBOOT_ENV_IMAGE_DIR
SONIC_VERSION=$(echo "$IMAGE_DIR" | sed 's/^image-/SONiC-OS-/')
disk_interface=$UBOOT_ENV_DISK_INTERFACE
demo_part=$UBOOT_ENV_DEMO_PART

BOOTCONF_RAW=$(fw_printenv -n bootconf 2>/dev/null || true)
BOOTCONF=$BOOTCONF_RAW
if [ -z "$BOOTCONF" ]; then
    BOOTCONF="ast2700-evb"
elif [ "$BOOTCONF" = "ast2700-nvidia-spc6-bmc" ]; then
    BOOTCONF="sonic-ast2700-nvidia-spc6-a1-bmc"
fi
if [ "$BOOTCONF_RAW" != "$BOOTCONF" ]; then
    sonic_uboot_env_log "Adjusting bootconf for FIT: ${BOOTCONF_RAW:-empty} -> $BOOTCONF"
    fw_setenv bootconf "$BOOTCONF" || sonic_uboot_env_log "ERROR: Failed to set bootconf"
fi

sonic_uboot_env_log "Programming U-Boot env (bootconf=$BOOTCONF, image_dir=$IMAGE_DIR, root=$ROOT_DEV)..."

fw_setenv image_dir "$IMAGE_DIR" || sonic_uboot_env_log "ERROR: Failed to set image_dir"
fw_setenv fit_name "$IMAGE_DIR/boot/sonic_arm64.fit" || sonic_uboot_env_log "ERROR: Failed to set fit_name"
fw_setenv sonic_version_1 "$SONIC_VERSION" || sonic_uboot_env_log "ERROR: Failed to set sonic_version_1"
fw_setenv image_dir_old "" || sonic_uboot_env_log "ERROR: Failed to set image_dir_old"
fw_setenv fit_name_old "" || sonic_uboot_env_log "ERROR: Failed to set fit_name_old"
fw_setenv sonic_version_2 "None" || sonic_uboot_env_log "ERROR: Failed to set sonic_version_2"
fw_setenv linuxargs_old "" || sonic_uboot_env_log "ERROR: Failed to set linuxargs_old"

LINUXARGS_VAL="console=${CONSOLE_PORT},${CONSOLE_SPEED}n8 ${EARLYCON} loopfstype=squashfs loop=$IMAGE_DIR/fs.squashfs varlog_size=${VAR_LOG_SIZE} logs_inram=on"
fw_setenv linuxargs "$LINUXARGS_VAL" || sonic_uboot_env_log "ERROR: Failed to set linuxargs"
BOOTARGS_VAL="root=$ROOT_DEV rw rootwait panic=1 $LINUXARGS_VAL"
fw_setenv bootargs "$BOOTARGS_VAL" || sonic_uboot_env_log "ERROR: Failed to set bootargs"

fw_setenv sonic_boot_load "ext4load ${disk_interface} 0:${demo_part} \${loadaddr} \${fit_name}" || sonic_uboot_env_log "ERROR: Failed to set sonic_boot_load"
fw_setenv sonic_boot_load_old "ext4load ${disk_interface} 0:${demo_part} \${loadaddr} \${fit_name_old}" || sonic_uboot_env_log "ERROR: Failed to set sonic_boot_load_old"
fw_setenv sonic_bootargs "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs}" || sonic_uboot_env_log "ERROR: Failed to set sonic_bootargs"
fw_setenv sonic_bootargs_old "setenv bootargs root=$ROOT_DEV rw rootwait panic=1 \${linuxargs_old}" || sonic_uboot_env_log "ERROR: Failed to set sonic_bootargs_old"
fw_setenv sonic_image_1 "run sonic_bootargs; run sonic_boot_load; bootm \${loadaddr}#conf-\${bootconf}" || sonic_uboot_env_log "ERROR: Failed to set sonic_image_1"
fw_setenv sonic_image_2 "run sonic_bootargs_old; run sonic_boot_load_old; bootm \${loadaddr}#conf-\${bootconf}" || sonic_uboot_env_log "ERROR: Failed to set sonic_image_2"

fw_setenv print_menu "echo ===================================================; echo SONiC Boot Menu; echo ===================================================; echo To boot \$sonic_version_1; echo   type: run sonic_image_1; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo To boot \$sonic_version_2; echo   type: run sonic_image_2; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\"Hit any key to stop autoboot:\\\" during boot; echo; echo ===================================================" || sonic_uboot_env_log "ERROR: Failed to set print_menu"

fw_setenv boot_next "run sonic_image_1" || sonic_uboot_env_log "ERROR: Failed to set boot_next"
fw_setenv bootcmd "run print_menu; test -n \"\$boot_once\" && setenv do_boot_once \"\$boot_once\" && setenv boot_once \"\" && saveenv && run do_boot_once; run boot_next" || sonic_uboot_env_log "ERROR: Failed to set bootcmd"

fw_setenv loadaddr "0x432000000" || sonic_uboot_env_log "ERROR: Failed to set loadaddr"
fw_setenv kernel_addr "0x403000000" || sonic_uboot_env_log "ERROR: Failed to set kernel_addr"
fw_setenv fdt_addr "0x44C000000" || sonic_uboot_env_log "ERROR: Failed to set fdt_addr"
fw_setenv initrd_addr "0x440000000" || sonic_uboot_env_log "ERROR: Failed to set initrd_addr"

sonic_uboot_env_log "U-Boot environment variables programmed successfully."
exit 0

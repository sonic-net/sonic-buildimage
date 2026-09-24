#!/bin/sh
# Generate /etc/fw_env.config for fw_printenv/fw_setenv.
# Order: device data (per onie_platform), then /proc/mtd, then device-tree hints, then eMMC default.
# SONIC_MACHINE_CONF: path to machine.conf (default /host/machine.conf). Override for initramfs
# (e.g. /tmp/machine.conf). If onie_platform is unset, that file is sourced when resolving device fw_env.
# Optional: SONIC_FW_ENV_LOG_FILE — if set, each message is appended there (same timestamped format as
# sonic-uboot-env-init log); messages always go to stderr as well.

FW_ENV_MSG_PREFIX="${FW_ENV_MSG_PREFIX:-sonic-fw-env-config}"
SONIC_MACHINE_CONF="${SONIC_MACHINE_CONF:-/host/machine.conf}"

fw_env_msg() {
    line="[$(date '+%Y-%m-%d %H:%M:%S')] [$FW_ENV_MSG_PREFIX] $*"
    echo "$line" >&2
    if [ -n "${SONIC_FW_ENV_LOG_FILE:-}" ]; then
        echo "$line" >> "$SONIC_FW_ENV_LOG_FILE"
    fi
}

sonic_fw_env_resolve_onie_platform() {
    if [ -n "${onie_platform:-}" ]; then
        return 0
    fi
    if [ ! -f "$SONIC_MACHINE_CONF" ]; then
        fw_env_msg "ERROR: machine.conf not found: $SONIC_MACHINE_CONF"
        exit 1
    fi
    . "$SONIC_MACHINE_CONF"
    if [ -z "${onie_platform:-}" ]; then
        fw_env_msg "ERROR: onie_platform not set after sourcing $SONIC_MACHINE_CONF"
        exit 1
    fi
    return 0
}

sonic_fw_env_wait_for_mtd() {
    WAIT_COUNT=0
    MAX_WAIT="${SONIC_FW_ENV_MTD_WAIT:-10}"
    while [ ! -f /proc/mtd ] && [ "$WAIT_COUNT" -lt "$MAX_WAIT" ]; do
        fw_env_msg "Waiting for /proc/mtd ($WAIT_COUNT/$MAX_WAIT)..."
        sleep 1
        WAIT_COUNT=$((WAIT_COUNT + 1))
    done
    if [ ! -f /proc/mtd ]; then
        fw_env_msg "WARNING: /proc/mtd not available after ${MAX_WAIT}s"
    fi
}

sonic_fw_env_install_from_path() {
    src="$1"
    changed=1
    if [ -f /etc/fw_env.config ]; then
        if command -v cmp >/dev/null 2>&1 && cmp -s "$src" /etc/fw_env.config; then
            changed=0
        elif [ "$(cat "$src")" = "$(cat /etc/fw_env.config)" ]; then
            changed=0
        fi
    fi
    if [ "$changed" -eq 1 ]; then
        cp "$src" /etc/fw_env.config
        fw_env_msg "Updated /etc/fw_env.config from $src"
    else
        fw_env_msg "Keeping /etc/fw_env.config (matches $src)"
    fi
}

sonic_fw_env_install_from_content() {
    content="$1"
    mkdir -p /etc
    if [ -f /etc/fw_env.config ]; then
        CURRENT_CONFIG=$(cat /etc/fw_env.config)
        if [ "$CURRENT_CONFIG" != "$content" ]; then
            fw_env_msg "Updating /etc/fw_env.config"
            printf '%s\n' "$content" > /etc/fw_env.config
        else
            fw_env_msg "Keeping /etc/fw_env.config (unchanged)"
        fi
    else
        printf '%s\n' "$content" > /etc/fw_env.config
        fw_env_msg "Created /etc/fw_env.config"
    fi
}

sonic_fw_env_try_device_data() {
    sonic_fw_env_resolve_onie_platform
    if [ -z "${onie_platform:-}" ]; then
        return 1
    fi
    for p in \
        "/usr/share/sonic/device/${onie_platform}/fw_env" \
        "/device/aspeed/${onie_platform}/fw_env"
    do
        if [ -f "$p" ]; then
            mkdir -p /etc
            sonic_fw_env_install_from_path "$p"
            return 0
        fi
    done
    return 1
}

sonic_fw_env_from_mtd_or_fallback() {
    sonic_fw_env_wait_for_mtd

    MTD_ENV_LINE=$(grep -E '"u-boot-env"|"uboot-env"' /proc/mtd 2>/dev/null || true)
    if [ -n "$MTD_ENV_LINE" ]; then
        MTD_DEV=$(echo "$MTD_ENV_LINE" | awk -F: '{print $1}')
        MTD_SIZE=$(echo "$MTD_ENV_LINE" | awk '{print "0x" $2}')
        MTD_ERASESIZE=$(echo "$MTD_ENV_LINE" | awk '{print "0x" $3}')
        if [ -c "/dev/$MTD_DEV" ]; then
            FW_ENV_CONFIG="/dev/$MTD_DEV 0x0 $MTD_SIZE $MTD_ERASESIZE"
            fw_env_msg "Detected U-Boot env from /proc/mtd: $FW_ENV_CONFIG"
            sonic_fw_env_install_from_content "$FW_ENV_CONFIG"
            return 0
        fi
    fi

    DTB_HAS_ENV_BLK=$(grep -E "uboot-env|u-boot-env" /proc/mtd 2>/dev/null | sed -e 's/:.*$//' || true)
    if [ -n "$DTB_HAS_ENV_BLK" ] && [ -c "/dev/$DTB_HAS_ENV_BLK" ]; then
        PROC_ENV_FILE=$(find /proc/device-tree/ -name env_size 2>/dev/null | head -1 || true)
        if [ -n "$PROC_ENV_FILE" ] && command -v hd >/dev/null 2>&1; then
            UBOOT_ENV_SIZ="0x$(hd "$PROC_ENV_FILE" | awk 'FNR==1 {print $2 $3 $4 $5}')"
            UBOOT_ENV_ERASE_SIZ="0x$(grep -E "uboot-env|u-boot-env" /proc/mtd | awk '{print $3}')"
            if [ -n "$UBOOT_ENV_SIZ" ] && [ -n "$UBOOT_ENV_ERASE_SIZ" ]; then
                FW_ENV_CONFIG="/dev/$DTB_HAS_ENV_BLK 0x00000000 $UBOOT_ENV_SIZ $UBOOT_ENV_ERASE_SIZ"
                fw_env_msg "Detected U-Boot env from device tree: $FW_ENV_CONFIG"
                sonic_fw_env_install_from_content "$FW_ENV_CONFIG"
                return 0
            fi
        fi
    fi

    FW_ENV_CONFIG="/dev/mmcblk0 0x1F40000 0x20000 0x1000"
    fw_env_msg "Using default eMMC U-Boot env location: $FW_ENV_CONFIG"
    sonic_fw_env_install_from_content "$FW_ENV_CONFIG"
    return 0
}

mkdir -p /etc

if sonic_fw_env_try_device_data; then
    exit 0
fi

sonic_fw_env_from_mtd_or_fallback

exit 0

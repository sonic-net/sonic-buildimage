#!/bin/sh
# Write eMMC image to /dev/mmcblk0 and ensure data is persisted before reboot.
# Usage: install-to-emmc.sh [path-to-image.img.gz]
# Default: /tmp/sonic-aspeed-arm64-emmc.img.gz
# After running, reboot to boot from eMMC.
# Initramfs must provide: sgdisk, wipefs, blockdev, partprobe, e2fsck,
# gunzip, dd, blkid, mount. (TFTP installer ramfs has no swap and no eMMC mounts.)

IMG="${1:-/tmp/sonic-aspeed-arm64-emmc.img.gz}"
EMMC="/dev/mmcblk0"

if [ ! -b "$EMMC" ]; then
    echo "Error: $EMMC not found"
    exit 1
fi
if [ ! -f "$IMG" ]; then
    echo "Error: image not found: $IMG"
    echo "Usage: $0 [path-to-image.img.gz]"
    exit 1
fi

rosys="/sys/block/$(basename "$EMMC")/force_ro"
if [ -w "$rosys" ]; then
    echo 0 > "$rosys" 2>/dev/null || true
fi
blockdev --setrw "$EMMC" 2>/dev/null || true
sync

echo "Clearing existing partition table on $EMMC..."
sgdisk --zap-all "$EMMC" || echo "Warning: sgdisk --zap-all failed; continuing with full image write."
wipefs -a "$EMMC" || echo "Warning: wipefs -a failed; continuing with full image write."
sync
blockdev --rereadpt "$EMMC" 2>/dev/null || true
partprobe "$EMMC" 2>/dev/null || true

echo "Writing $IMG to $EMMC..."
# Smaller bs + full pipe reads: fewer phys_seg per I/O on some MMC hosts than bs=1M; conv=fsync at end.
# Pipeline exit status is only from dd; corrupt/truncated gzip can still yield dd exit 0, so require gunzip success.
gz_ok=$(mktemp) || exit 1
(gunzip -c "$IMG" && echo ok >"$gz_ok") | dd of="$EMMC" bs=128k iflag=fullblock conv=fsync
DD_RC=$?
if [ "$DD_RC" -ne 0 ] || [ ! -s "$gz_ok" ]; then
    rm -f "$gz_ok"
    echo "Error: write to $EMMC failed, or image is corrupt/truncated (see kernel log for mmcblk I/O errors)."
    exit 1
fi
rm -f "$gz_ok"
echo "Syncing..."
sync
blockdev --flushbufs "$EMMC" 2>/dev/null || true
echo "Relocating GPT backup to end of $EMMC (fixes invalid alternate GPT when image < device)..."
sgdisk -e "$EMMC" || echo "Warning: sgdisk -e failed"
partprobe "$EMMC" 2>/dev/null || true

# SONiC mounts the SONiC-OS ext4 root at /host; machine.conf must be the file at the fs root
# (see platform/aspeed/build-emmc-image-installer.sh). Copy from TFTP init if we generated it in initramfs.
MACHINE_CONF_SRC=/tmp/machine.conf
if [ -f "$MACHINE_CONF_SRC" ] && [ -s "$MACHINE_CONF_SRC" ]; then
    sync
    blockdev --rereadpt "$EMMC" 2>/dev/null || true
    partprobe "$EMMC" 2>/dev/null || true
    sleep 1
    BOOT_PART=$(blkid -L SONiC-OS 2>/dev/null || true)
    if [ -z "$BOOT_PART" ] && [ -b "${EMMC}p1" ]; then
        BOOT_PART="${EMMC}p1"
    fi
    if [ -n "$BOOT_PART" ] && [ -b "$BOOT_PART" ]; then
        echo "Checking ext4 on $BOOT_PART (e2fsck)..."
        e2fsck -fy "$BOOT_PART" </dev/null 2>/dev/null || true
        MNT=$(mktemp -d)
        if mount -t ext4 -o rw "$BOOT_PART" "$MNT" 2>/dev/null; then
            cp "$MACHINE_CONF_SRC" "$MNT/machine.conf"
            chmod 644 "$MNT/machine.conf" 2>/dev/null || true
            sync
            umount "$MNT" 2>/dev/null || true
            echo "Wrote machine.conf to SONiC-OS partition (visible as /host/machine.conf after boot)."
        else
            echo "Warning: could not mount $BOOT_PART rw; left machine.conf only in initramfs ($MACHINE_CONF_SRC)."
        fi
        rmdir "$MNT" 2>/dev/null || true
    else
        echo "Warning: SONiC-OS partition not found; could not write machine.conf to eMMC."
    fi
fi

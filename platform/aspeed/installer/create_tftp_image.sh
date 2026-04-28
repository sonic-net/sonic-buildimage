#!/bin/bash
#
# Build the Aspeed U-Boot / TFTP install bundle (FIT + initramfs + optional raw eMMC image).
#
# Platform-local installer orchestration under platform/<name>/installer/ (same structural role as
# other platforms' installer scripts). Differences: output is a FIT for TFTP boot, and second-stage
# logic lives in ../tftp-installer-init/.
#
# This script is not wired as a second SONIC_INSTALLERS entry: that would create a second RFS
# squashfs target (see slave.mk rfs_define_target). The TFTP bundle reuses the rootfs built for
# sonic-aspeed-arm64.bin (see recipes/installer-tftp.mk).
#
# Usage (from sonic-buildimage root):
#   ./platform/aspeed/installer/create_tftp_image.sh [output_dir]
#   OUTPUT_TFTP_INSTALL_TAR=target/sonic-aspeed-arm64-tftp-install.tar \
#     ./platform/aspeed/installer/create_tftp_image.sh target/aspeed-tftp
#
# Environment (see also platform/aspeed/onie-image-arm64.conf):
#   FILESYSTEM_ROOT, EMMC_RAW_IMAGE, OUTPUT_TFTP_INSTALL_TAR, TFTP_EMMC_IMAGE_NAME

set -e

unpack_initrd_to_dir() {
    local img="$1" dir="$2"
    local decom=zcat mime magic
    mime=$(file --brief --mime-type "$img" 2>/dev/null) || mime=""
    case "$mime" in
        application/zstd)   decom=zstdcat ;;
        application/x-lz4)  decom=lz4cat ;;
        application/x-lzma) decom=xzcat ;;
        application/gzip|application/x-gzip) decom=zcat ;;
        application/octet-stream)
            magic=$(head -c 6 "$img" | xxd -p 2>/dev/null | tr -d '\n')
            case "$magic" in
                28b52ffd*) decom=zstdcat ;;
                1f8b*)     decom=zcat ;;
                fd377a585a00) decom=xzcat ;;
            esac
            ;;
    esac
    mkdir -p "$dir"
    $decom "$img" | (cd "$dir" && cpio -id 2>/dev/null) || true
}

repack_initrd_cpio_gz() {
    local dir="$1" out="$2"
    (cd "$dir" && find . | cpio -o -H newc) | gzip -9 > "$out"
}

# Walk DT_NEEDED from readelf and copy matching SONAMEs from FILESYSTEM_ROOT (transitive). Used when
# chroot ldd / host ldd yield nothing for the target arch during cross-builds.
copy_elf_needed_closure_into() {
    local seed="$1"
    local dest_root="$2"
    local fs="$FILESYSTEM_ROOT"
    local pending scanned work s f rel

    [ -f "$seed" ] || return 0
    pending=$(mktemp)
    scanned=$(mktemp)
    echo "$seed" >> "$pending"
    while [ -s "$pending" ]; do
        work=$(head -n1 "$pending")
        tail -n +2 "$pending" > "$pending.new" && mv "$pending.new" "$pending"
        if grep -qxF "$work" "$scanned" 2>/dev/null; then
            continue
        fi
        echo "$work" >> "$scanned"
        for s in $(readelf -d "$work" 2>/dev/null | sed -n 's/.*Shared library: \[\([^]]*\)\]/\1/p'); do
            [ -z "$s" ] && continue
            f=$(find "$fs/lib" "$fs/usr/lib" -name "$s" 2>/dev/null | head -n1)
            [ -n "$f" ] && [ -f "$f" ] || continue
            rel="${f#$fs}"
            rel="/${rel#/}"
            mkdir -p "$dest_root$(dirname "$rel")"
            cp -L "$f" "$dest_root$rel" 2>/dev/null || true
            echo "$f" >> "$pending"
        done
    done
    rm -f "$pending" "$scanned"
}

copy_fsroot_binary_to_initrd() {
    local abs="$1" dir="$2"
    local root="${dir%/}"
    local real="$abs"
    [ -L "$abs" ] && real=$(readlink -f "$abs")
    local rel="${abs#$FILESYSTEM_ROOT}"
    rel="/${rel#/}"
    local dest="$root$rel"
    mkdir -p "$(dirname "$dest")"
    rm -f "$dest"
    cp -L "$abs" "$dest"
    chmod 755 "$dest"
    local interp idest ldest chpath
    interp=$(readelf -l "$real" 2>/dev/null | sed -n 's/.*Requesting program interpreter: *\[\([^]]*\)\].*/\1/p')
    if [ -n "$interp" ] && [ -f "$FILESYSTEM_ROOT$interp" ]; then
        idest="$root$interp"
        mkdir -p "$(dirname "$idest")"
        rm -f "$idest"
        cp -L "$FILESYSTEM_ROOT$interp" "$idest" 2>/dev/null || true
    fi
    chpath="/${rel#/}"
    (chroot "$FILESYSTEM_ROOT" ldd "$chpath" 2>/dev/null || ldd "$abs" 2>/dev/null) | awk '/=>/ {print $3}' | while read -r path; do
        [ -z "$path" ] || [ "$path" = "not" ] && continue
        [ -f "$FILESYSTEM_ROOT$path" ] || continue
        ldest="$root$path"
        mkdir -p "$(dirname "$ldest")"
        rm -f "$ldest"
        cp -L "$FILESYSTEM_ROOT$path" "$ldest" 2>/dev/null || true
    done
    copy_elf_needed_closure_into "$real" "$root"
}

inject_fsroot_extras_into_initrd() {
    local dir="$1"
    local p
    for p in \
        /usr/sbin/sgdisk /sbin/sgdisk \
        /usr/sbin/gdisk /sbin/gdisk \
        /usr/sbin/cgdisk /sbin/cgdisk \
        /usr/sbin/fixparts /sbin/fixparts \
        /sbin/e2fsck /usr/sbin/e2fsck \
        /sbin/fsck.ext2 /sbin/fsck.ext3 /sbin/fsck.ext4 \
        /sbin/mke2fs /usr/sbin/mke2fs \
        /sbin/mkfs.ext2 /sbin/mkfs.ext3 /sbin/mkfs.ext4 \
        /sbin/resize2fs \
        /sbin/tune2fs \
        /sbin/dumpe2fs \
        /usr/sbin/badblocks \
        /sbin/debugfs \
        /sbin/e4defrag \
        /usr/sbin/filefrag \
        /usr/sbin/logsave \
        /sbin/mkfs.vfat /usr/sbin/mkfs.vfat /sbin/mkfs.fat \
        /sbin/fsck.vfat /usr/sbin/fsck.vfat /sbin/fsck.fat \
        /usr/sbin/fatlabel /sbin/fatlabel \
        /sbin/parted /usr/sbin/parted \
        /usr/sbin/partprobe /sbin/partprobe \
        /sbin/blkid /usr/sbin/blkid \
        /sbin/blockdev /usr/sbin/blockdev \
        /sbin/wipefs /usr/sbin/wipefs \
        /usr/sbin/sfdisk /sbin/sfdisk \
        /usr/bin/fw_printenv /usr/sbin/fw_printenv \
        /usr/bin/fw_setenv /usr/sbin/fw_setenv \
        /usr/bin/fw_getenv /usr/sbin/fw_getenv; do
        [ -e "$FILESYSTEM_ROOT/$p" ] || continue
        copy_fsroot_binary_to_initrd "$FILESYSTEM_ROOT/$p" "$dir"
    done
}

repack_sonic_initrd_for_tftp_installer() {
    local in="$1" out="$2"
    local d ko
    d=$(mktemp -d)
    unpack_initrd_to_dir "$in" "$d"
    rm -f "$d/init-options" "$d/init-options-base"
    cp "$d/init" "$d/init.stock"
    cp "$PLATFORM_ASPEED/tftp-installer-init/init" "$d/init"
    chmod 755 "$d/init"
    mkdir -p "$d/sbin"
    cp "$PLATFORM_ASPEED/tftp-installer-init/install-to-emmc.sh" "$d/sbin/install-to-emmc.sh"
    chmod 755 "$d/sbin/install-to-emmc.sh"
    cp "$PLATFORM_ASPEED/tftp-installer-init/udhcpc.script" "$d/sbin/udhcpc.script"
    chmod 755 "$d/sbin/udhcpc.script"
    cp "$PLATFORM_ASPEED/scripts/sonic-uboot-env-init.sh" "$d/sbin/sonic-uboot-env-init.sh"
    chmod 755 "$d/sbin/sonic-uboot-env-init.sh"
    cp "$PLATFORM_ASPEED/scripts/sonic-machine-conf-init.sh" "$d/sbin/sonic-machine-conf-init.sh"
    chmod 755 "$d/sbin/sonic-machine-conf-init.sh"
    cp "$PLATFORM_ASPEED/scripts/sonic-fw-env-config.sh" "$d/sbin/sonic-fw-env-config.sh"
    chmod 755 "$d/sbin/sonic-fw-env-config.sh"
    cp "$PLATFORM_ASPEED/scripts/sonic-program-uboot-env.sh" "$d/sbin/sonic-program-uboot-env.sh"
    chmod 755 "$d/sbin/sonic-program-uboot-env.sh"
    _aspeed_repo_root="$(cd "$PLATFORM_ASPEED/../.." && pwd)"
    if [ -d "$_aspeed_repo_root/device/aspeed" ]; then
        mkdir -p "$d/device/aspeed"
        cp -a "$_aspeed_repo_root/device/aspeed/." "$d/device/aspeed/"
    fi
    unset _aspeed_repo_root
    inject_fsroot_extras_into_initrd "$d"
    if [ ! -f "$d/lib/modules/ftgmac100.ko" ] && [ -d "$d/lib/modules" ]; then
        ko=$(find "$d/lib/modules" -name ftgmac100.ko 2>/dev/null | head -1)
        if [ -n "$ko" ]; then
            mkdir -p "$d/lib/modules"
            cp "$ko" "$d/lib/modules/ftgmac100.ko"
        fi
    fi
    repack_initrd_cpio_gz "$d" "$out"
    rm -rf "$d"
}

# Populates globals: KERNEL, KERNEL_VERSION_FULL, SONIC_INITRD. May set BOOT_FALLBACK_TMP for unzip cleanup.
resolve_kernel_and_initrd() {
    local root="$1"
    local k ver ir payload zpath
    KERNEL=""
    KERNEL_VERSION_FULL=""
    SONIC_INITRD=""
    payload="${INSTALLER_PAYLOAD:-fs.zip}"
    zpath="$REPO_ROOT/$payload"

    for k in "$root/boot"/vmlinuz-*; do
        [ -f "$k" ] || continue
        KERNEL="$k"
        break
    done
    if [ -z "$KERNEL" ]; then
        KERNEL=$(find "$root/usr/lib" -path '*/linux-image-*/vmlinuz-*' -type f 2>/dev/null | head -1)
    fi
    if [ -z "$KERNEL" ] || [ ! -f "$KERNEL" ]; then
        if [ -f "$zpath" ] && command -v unzip >/dev/null 2>&1; then
            BOOT_FALLBACK_TMP=$(mktemp -d)
            if unzip -qq -j -o "$zpath" "boot/vmlinuz-*" -d "$BOOT_FALLBACK_TMP" 2>/dev/null; then
                KERNEL=$(ls -1 "$BOOT_FALLBACK_TMP"/vmlinuz-* 2>/dev/null | head -1)
            fi
            if [ -z "$KERNEL" ] || [ ! -f "$KERNEL" ]; then
                rm -rf "$BOOT_FALLBACK_TMP"
                BOOT_FALLBACK_TMP=""
            fi
        fi
    fi
    if [ -z "$KERNEL" ] || [ ! -f "$KERNEL" ]; then
        echo "Error: No kernel found (checked $root/boot/, usr/lib/linux-image-*/vmlinuz-*, and $zpath boot/)."
        echo "Build target/sonic-aspeed-arm64.bin first so fs.zip or fsroot contains /boot."
        return 1
    fi
    ver=$(basename "$KERNEL" | sed 's/^vmlinuz-//')
    KERNEL_VERSION_FULL="$ver"

    SONIC_INITRD="$root/boot/initrd.img-$ver"
    if [ ! -f "$SONIC_INITRD" ]; then
        SONIC_INITRD=$(find "$root/usr/lib" -path "*/linux-image-$ver/initrd.img*" -type f 2>/dev/null | head -1)
    fi
    if [ -n "$BOOT_FALLBACK_TMP" ] && [ -d "$BOOT_FALLBACK_TMP" ]; then
        ir=$(ls -1 "$BOOT_FALLBACK_TMP"/initrd.img-* 2>/dev/null | head -1)
        if [ -n "$ir" ] && [ -f "$ir" ]; then
            SONIC_INITRD="$ir"
        fi
    fi
    if [ -z "$SONIC_INITRD" ] || [ ! -f "$SONIC_INITRD" ]; then
        if [ -f "$zpath" ] && command -v unzip >/dev/null 2>&1; then
            [ -z "$BOOT_FALLBACK_TMP" ] && BOOT_FALLBACK_TMP=$(mktemp -d)
            unzip -qq -j -o "$zpath" "boot/initrd.img-*" -d "$BOOT_FALLBACK_TMP" 2>/dev/null || true
            ir=$(ls -1 "$BOOT_FALLBACK_TMP"/initrd.img-* 2>/dev/null | head -1)
            [ -n "$ir" ] && [ -f "$ir" ] && SONIC_INITRD="$ir"
        fi
    fi
    if [ -z "$SONIC_INITRD" ] || [ ! -f "$SONIC_INITRD" ]; then
        echo "Error: No initrd for kernel $ver (expected boot/initrd.img-$ver, usr/lib/linux-image-$ver/, or $zpath boot/)."
        return 1
    fi
    return 0
}

INSTALLER_DIR="$(cd "$(dirname "$0")" && pwd)"
PLATFORM_ASPEED="$(cd "$INSTALLER_DIR/.." && pwd)"
REPO_ROOT="$(cd "$PLATFORM_ASPEED/../.." && pwd)"
cd "$REPO_ROOT"

OUTPUT_DIR="${1:-target/aspeed-tftp}"
FIT_NAME="sonic_tftp_install.fit"

if [ -r "./platform/aspeed/onie-image-arm64.conf" ]; then
    . ./platform/aspeed/onie-image-arm64.conf
fi
FILESYSTEM_ROOT="${FILESYSTEM_ROOT:-./fsroot-aspeed}"

if [ ! -d "$FILESYSTEM_ROOT" ]; then
    echo "Error: $FILESYSTEM_ROOT not found. Build the Aspeed image first:"
    echo "  make target/sonic-aspeed-arm64.bin"
    exit 1
fi
FILESYSTEM_ROOT="$(cd "$FILESYSTEM_ROOT" && pwd)"

INSTALLER_PAYLOAD="${INSTALLER_PAYLOAD:-fs.zip}"
BOOT_FALLBACK_TMP=""
FIT_STAGING=""

cleanup_all() {
    sudo umount "$FILESYSTEM_ROOT/tmp/tftp-fit" 2>/dev/null || true
    rm -rf "${FIT_STAGING:-}" 2>/dev/null || true
    rm -rf "${BOOT_FALLBACK_TMP:-}" 2>/dev/null || true
}
trap cleanup_all EXIT

if ! resolve_kernel_and_initrd "$FILESYSTEM_ROOT"; then
    exit 1
fi

DTB_DIR=$(ls -d "$FILESYSTEM_ROOT/usr/lib/linux-image-"*/aspeed 2>/dev/null | head -1)
if [ -z "$DTB_DIR" ] || [ ! -d "$DTB_DIR" ]; then
    DTB_DIR=$(ls -d "$FILESYSTEM_ROOT/usr/lib/linux-image-${KERNEL_VERSION_FULL}/aspeed" 2>/dev/null | head -1)
fi

EMMC_RAW_IMAGE="${EMMC_RAW_IMAGE:-target/sonic-aspeed-arm64-emmc.img.gz}"
[ -n "$EMMC_RAW_IMAGE" ] || EMMC_RAW_IMAGE="target/sonic-aspeed-arm64-emmc.img.gz"
if [ -n "${TFTP_EMMC_IMAGE_NAME:-}" ]; then
    TFTP_IMAGE_NAME="$TFTP_EMMC_IMAGE_NAME"
else
    TFTP_IMAGE_NAME=$(basename "$EMMC_RAW_IMAGE")
fi

if [ -z "$DTB_DIR" ] || [ ! -d "$DTB_DIR" ]; then
    echo "Error: No DTB dir under $FILESYSTEM_ROOT/usr/lib/linux-image-*/aspeed"
    exit 1
fi
if [ ! -f "$EMMC_RAW_IMAGE" ]; then
    echo "Error: Raw eMMC image not found: $EMMC_RAW_IMAGE"
    echo "Build it first: make aspeed-emmc-image"
    echo "Or manually: sudo ./platform/aspeed/build-emmc-image-installer.sh target/sonic-aspeed-arm64.bin"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

INITRD_DEST="$OUTPUT_DIR/initrd-tftp-net-install.img"
echo "Repacking production initrd $SONIC_INITRD (strip init-options; installer /init)..."
repack_sonic_initrd_for_tftp_installer "$SONIC_INITRD" "$INITRD_DEST"
INITRAMFS="$INITRD_DEST"

echo "Using kernel: $KERNEL"
echo "Using initramfs (source): $SONIC_INITRD"
echo "Using repacked initramfs: $INITRAMFS"

echo "Building FIT image..."
FIT_STAGING=$(mktemp -d)
cp "$KERNEL" "$FIT_STAGING/kernel"
cp "$OUTPUT_DIR/initrd-tftp-net-install.img" "$FIT_STAGING/initrd"
mkdir -p "$FIT_STAGING/dtb"
cp "$DTB_DIR"/*.dtb "$FIT_STAGING/dtb/" 2>/dev/null || true
sed -e "s|__KERNEL_PATH__|/tmp/tftp-fit/kernel|g" \
    -e "s|__INITRD_PATH__|/tmp/tftp-fit/initrd|g" \
    -e "s|__DTB_PATH_ASPEED__|/tmp/tftp-fit/dtb|g" \
    "$PLATFORM_ASPEED/sonic_fit.its" > "$FIT_STAGING/sonic.its"

sudo mkdir -p "$FILESYSTEM_ROOT/tmp/tftp-fit"
sudo mount --bind "$FIT_STAGING" "$FILESYSTEM_ROOT/tmp/tftp-fit"
sudo chroot "$FILESYSTEM_ROOT" mkimage -f /tmp/tftp-fit/sonic.its /tmp/tftp-fit/out.fit
sudo cp "$FILESYSTEM_ROOT/tmp/tftp-fit/out.fit" "$OUTPUT_DIR/$FIT_NAME"
sudo chown "$(id -u):$(id -g)" "$OUTPUT_DIR/$FIT_NAME"
sudo umount "$FILESYSTEM_ROOT/tmp/tftp-fit"
rm -rf "$FIT_STAGING"
FIT_STAGING=""

cp -v "$EMMC_RAW_IMAGE" "$OUTPUT_DIR/$TFTP_IMAGE_NAME"

fit_addr=0x432000000

cat > "$OUTPUT_DIR/uboot-tftp-commands.txt" << EOF
# Aspeed TFTP initramfs: /init brings up network, can TFTP the eMMC image and run install-to-emmc.sh.
#
# 1. On TFTP server: sonic_tftp_install.fit and $TFTP_IMAGE_NAME (same directory as FIT is typical).
#
# 2. In U-Boot — auto install to eMMC (DHCP + TFTP from serverip):
dhcp
setenv serverip <tftp-server-ip>
setenv loadaddr $fit_addr
setenv bootargs "console=ttyS12,115200n8 earlycon=uart8250,mmio32,0x14c33b00 root=/dev/ram0 rw sonic_install.tftp_server=\${serverip} sonic_install.tftp_image=$TFTP_IMAGE_NAME"
tftp \$loadaddr sonic_tftp_install.fit
# bootconf must match a "configurations" entry in platform/aspeed/sonic_fit.its (name without conf- prefix).
setenv bootconf <fit-configuration>
bootm \$loadaddr#conf-\$bootconf
#
#    Optional bootargs: sonic_install.reboot=1  (reboot after flash; network is always eth0)
#    Static IP instead of DHCP: add e.g. ip=192.168.1.50::192.168.1.1:255.255.255.0::eth0:off
#
# 3. Manual install (no sonic_install.tftp_server in bootargs): shell then
#      /sbin/install-to-emmc.sh /tmp/$TFTP_IMAGE_NAME
#    If you use gunzip|dd manually, run sync (and blockdev --flushbufs /dev/mmcblk0) before reboot
#    or the eMMC root fs may be corrupt (EXT4 journal / I/O errors on first boot).
EOF

if [ -n "${OUTPUT_TFTP_INSTALL_TAR:-}" ]; then
    mkdir -p "$(dirname "$OUTPUT_TFTP_INSTALL_TAR")"
    rm -f "$OUTPUT_TFTP_INSTALL_TAR"
    tar -chf "$OUTPUT_TFTP_INSTALL_TAR" -C "$OUTPUT_DIR" \
        "$FIT_NAME" uboot-tftp-commands.txt "$TFTP_IMAGE_NAME"
    echo "TFTP install archive: $OUTPUT_TFTP_INSTALL_TAR"
fi

echo ""
echo "TFTP initramfs image ready in: $OUTPUT_DIR"
echo "  - $FIT_NAME (boot from TFTP → initramfs shell)"
echo "  - $TFTP_IMAGE_NAME (optional on TFTP if you pull it manually from the shell)"
echo "  - uboot-tftp-commands.txt (U-Boot commands)"
echo ""
echo "Put $FIT_NAME and $TFTP_IMAGE_NAME on your TFTP server, then in U-Boot:"
echo "  setenv serverip <tftp-server-ip>"
echo "  setenv bootargs \"console=... root=/dev/ram0 rw sonic_install.tftp_server=\${serverip} sonic_install.tftp_image=$TFTP_IMAGE_NAME\""
echo "  tftp $fit_addr sonic_tftp_install.fit"
echo "  setenv bootconf <fit-configuration>   # see configurations in platform/aspeed/sonic_fit.its (no conf- prefix)"
echo "  bootm $fit_addr#conf-\$bootconf"
echo ""
echo "Without sonic_install.tftp_server=: shell, then /sbin/install-to-emmc.sh /tmp/$TFTP_IMAGE_NAME"

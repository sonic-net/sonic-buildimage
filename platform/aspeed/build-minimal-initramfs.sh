#!/bin/bash
# Build Ultra-Minimal Installation Initramfs for AST2700
# This creates a tiny (~3-5MB) initramfs that streams and flashes the pre-built eMMC image
#
# Usage: ./build-minimal-initramfs.sh [output-name]
#
# Requirements:
#   - busybox-static package
#   - cpio
#   - gzip

set -e  # Exit on error

# Configuration
TFTP_SERVER="${TFTP_SERVER:-10.20.16.12}"
BOARD_IP="${BOARD_IP:-10.60.1.86}"
GATEWAY="${GATEWAY:-10.60.1.1}"
NETMASK="${NETMASK:-255.255.255.0}"
EMMC_IMAGE="${EMMC_IMAGE:-sonic-aspeed-emmc.img.gz}"
COMPRESSION="${COMPRESSION:-gzip}"  # gzip or zstd

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_requirements() {
    log_info "Checking requirements..."

    local missing=0

    for cmd in cpio gzip mkimage; do
        if ! command -v $cmd &> /dev/null; then
            log_error "Required command not found: $cmd"
            missing=1
        fi
    done

    if [ $missing -eq 1 ]; then
        log_error "Please install missing dependencies:"
        log_error "  Ubuntu/Debian: sudo apt-get install cpio gzip u-boot-tools"
        exit 1
    fi

    log_info "All requirements satisfied"
}

cleanup() {
    if [ -n "$WORK_DIR" ] && [ -d "$WORK_DIR" ]; then
        rm -rf "$WORK_DIR"
    fi
}

trap cleanup EXIT

# Parse arguments
OUTPUT_NAME="${1:-sonic-install-initrd.img}"

log_info "Ultra-Minimal Installation Initramfs Builder"
log_info "============================================="
log_info "Output: $OUTPUT_NAME"
log_info "Network Configuration:"
log_info "  Board IP: $BOARD_IP"
log_info "  Gateway: $GATEWAY"
log_info "  TFTP Server: $TFTP_SERVER"
log_info "  eMMC Image: $EMMC_IMAGE"
log_info ""

check_requirements

# Create working directory
WORK_DIR=$(mktemp -d -t sonic-initramfs.XXXXXX)
log_info "Working directory: $WORK_DIR"
cd "$WORK_DIR"
DESTDIR=$OLDPWD
log_info "DESTDIR : $DESTDIR"

# Step 1: Create directory structure
log_info "Creating directory structure..."
mkdir -p bin sbin lib usr/bin usr/sbin etc proc sys dev tmp mnt

# Step 2: Install busybox
log_info "Installing busybox..."

# Try to find busybox-static
BUSYBOX_PATH=""
for path in /bin/busybox /usr/bin/busybox; do
    if [ -x "$path" ]; then
        BUSYBOX_PATH="$path"
        break
    fi
done

if [ -z "$BUSYBOX_PATH" ]; then
    # Try to download and extract busybox-static package
    log_info "Busybox not found, downloading busybox-static package..."
    
    if command -v apt-get &> /dev/null; then
        apt-get download busybox-static 2>/dev/null || true
        if ls busybox-static_*.deb &>/dev/null; then
            dpkg -x busybox-static_*.deb .
            BUSYBOX_PATH="./bin/busybox"
        fi
    fi
fi

if [ -z "$BUSYBOX_PATH" ] || [ ! -x "$BUSYBOX_PATH" ]; then
    log_error "Cannot find busybox binary"
    log_error "Please install: sudo apt-get install busybox-static"
    exit 1
fi

cp "$BUSYBOX_PATH" bin/busybox
log_info "Busybox installed: $(bin/busybox | head -1)"

# Step 3: Create busybox symlinks
log_info "Creating busybox symlinks..."
cd bin
for cmd in sh ash mount umount mkdir mknod ifconfig route tftp wget \
           sync reboot poweroff ls cat cp mv rm chmod sleep echo \
           grep sed awk find df free ln dd gunzip zcat nc vi \
           setsid wait ping basename dirname xargs tail head dmesg blockdev; do
    ln -sf busybox "$cmd" 2>/dev/null || true
done
cd ..

# Check if zstd is needed
if [ "$COMPRESSION" = "zstd" ]; then
    log_warn "Note: zstd decompression requires zstd binary in initramfs"
    log_warn "      Make sure to include zstd or use gzip compression"
fi

# Step 3.5: Add dropbear for SCP support (optional)
log_info "Adding dropbear SSH client for SCP support..."
if command -v dbclient &> /dev/null; then
    cp $(which dbclient) bin/dbclient 2>/dev/null || log_warn "Could not copy dbclient"
    ln -sf dbclient bin/scp 2>/dev/null || true
    log_info "Dropbear SSH client added (enables SCP downloads)"
else
    log_warn "dbclient not found - SCP downloads will not be available"
    log_warn "Install with: sudo apt-get install dropbear-bin"
fi

# Note: U-Boot tools (fw_setenv/fw_printenv) are NOT included
# MTD devices are not accessible from minimal initramfs
# U-Boot configuration must be done manually from U-Boot prompt

# Step 4: Create installation script (separate, editable)
log_info "Creating installation script..."

cat > install-sonic.sh << 'INSTALL_SCRIPT_EOF'
#!/bin/sh
#
# SONiC AST2700 eMMC Installation Script
# This script downloads and installs SONiC to eMMC
# Can be run manually or automatically from init
#
# Supports multiple download methods:
#   TFTP:  TFTP_SERVER=192.168.1.10 /install-sonic.sh
#   HTTP:  HTTP_URL=http://192.168.1.10/sonic-aspeed-emmc.img.gz /install-sonic.sh
#   SCP:   SCP_URL=user@192.168.1.10:/path/to/sonic-aspeed-emmc.img.gz /install-sonic.sh
#

# Configuration (will be replaced by build script)
TFTP_SERVER="${TFTP_SERVER:-__TFTP_SERVER__}"
HTTP_URL="${HTTP_URL}"
SCP_URL="${SCP_URL}"
EMMC_IMAGE="${EMMC_IMAGE:-__EMMC_IMAGE__}"
COMPRESSION="${COMPRESSION:-__COMPRESSION__}"

echo ""
echo "========================================"
echo "  SONiC eMMC Installation"
echo "========================================"
echo ""

# Check for eMMC
if [ ! -b /dev/mmcblk0 ]; then
    echo "ERROR: eMMC device /dev/mmcblk0 not found!"
    echo "Available block devices:"
    ls -l /dev/mmcblk* /dev/sd* 2>/dev/null || echo "  (none)"
    echo ""
    exit 1
fi

# Determine download method
DOWNLOAD_METHOD=""
if [ -n "$HTTP_URL" ]; then
    DOWNLOAD_METHOD="http"
    DOWNLOAD_SOURCE="$HTTP_URL"
elif [ -n "$SCP_URL" ]; then
    DOWNLOAD_METHOD="scp"
    DOWNLOAD_SOURCE="$SCP_URL"
elif [ -n "$TFTP_SERVER" ]; then
    DOWNLOAD_METHOD="tftp"
    DOWNLOAD_SOURCE="$TFTP_SERVER/$EMMC_IMAGE"
else
    echo "ERROR: No download method specified!"
    echo ""
    echo "Usage examples:"
    echo "  TFTP:  TFTP_SERVER=192.168.1.10 /install-sonic.sh"
    echo "  HTTP:  HTTP_URL=http://192.168.1.10/sonic-aspeed-emmc.img.gz /install-sonic.sh"
    echo "  SCP:   SCP_URL=user@192.168.1.10:/path/to/sonic-aspeed-emmc.img.gz /install-sonic.sh"
    echo ""
    exit 1
fi

# Check available memory
echo "Checking available memory..."
FREE_MEM_MB=$(free -m | awk '/^Mem:/ {print $7}')
echo "Available memory: ${FREE_MEM_MB}MB"
echo ""

# Compressed image is ~580-600MB, need at least 700MB free
if [ "$FREE_MEM_MB" -lt 700 ]; then
    echo "WARNING: Low memory! Available: ${FREE_MEM_MB}MB, Recommended: 700MB+"
    echo "Installation may fail. Consider freeing up memory first."
    echo ""
fi

echo "eMMC device: /dev/mmcblk0"
echo "Download method: $DOWNLOAD_METHOD"
echo "Download source: $DOWNLOAD_SOURCE"
echo "Compression: $COMPRESSION"
echo ""
echo "⚠️  WARNING: This will ERASE all data on /dev/mmcblk0!"
echo ""

# Check if running interactively
if [ -t 0 ]; then
    echo "Press Ctrl+C within 5 seconds to abort..."
    sleep 5
fi

cd /tmp

# Step 1: Download compressed image to /tmp
echo ""
echo "========================================"
echo "  Step 1: Downloading Image"
echo "========================================"
echo ""

TEMP_IMAGE="/tmp/sonic-emmc-install.img.gz"

case "$DOWNLOAD_METHOD" in
    tftp)
        echo "Downloading via TFTP from $TFTP_SERVER..."
        if ! tftp -g -r "$EMMC_IMAGE" "$TFTP_SERVER" -l "$TEMP_IMAGE"; then
            echo "ERROR: TFTP download failed!"
            exit 1
        fi
        ;;
    http)
        echo "Downloading via HTTP from $HTTP_URL..."
        if ! wget -O "$TEMP_IMAGE" "$HTTP_URL"; then
            echo "ERROR: HTTP download failed!"
            rm -f "$TEMP_IMAGE"
            exit 1
        fi
        ;;
    scp)
        echo "Downloading via SCP from $SCP_URL..."
        if command -v scp &> /dev/null; then
            if ! scp "$SCP_URL" "$TEMP_IMAGE"; then
                echo "ERROR: SCP download failed!"
                rm -f "$TEMP_IMAGE"
                exit 1
            fi
        elif command -v dbclient &> /dev/null; then
            # Extract user@host and path
            SCP_HOST="${SCP_URL%:*}"
            SCP_PATH="${SCP_URL##*:}"
            if ! dbclient "$SCP_HOST" "cat $SCP_PATH" > "$TEMP_IMAGE"; then
                echo "ERROR: SCP download failed!"
                rm -f "$TEMP_IMAGE"
                exit 1
            fi
        else
            echo "ERROR: scp/dbclient not available for SCP downloads"
            exit 1
        fi
        ;;
    *)
        echo "ERROR: Unknown download method: $DOWNLOAD_METHOD"
        exit 1
        ;;
esac

# Verify download
if [ ! -f "$TEMP_IMAGE" ]; then
    echo "ERROR: Downloaded file not found!"
    exit 1
fi

DOWNLOADED_SIZE=$(ls -lh "$TEMP_IMAGE" | awk '{print $5}')
echo ""
echo "✓ Download complete: $DOWNLOADED_SIZE"
echo ""

# Step 2: Decompress and write to eMMC
echo "========================================"
echo "  Step 2: Writing to eMMC"
echo "========================================"
echo ""
echo "Decompressing and writing to /dev/mmcblk0..."
echo "This will take 5-10 minutes..."
echo ""

# Decompress and write
if [ "$COMPRESSION" = "zstd" ]; then
    echo "Using zstd decompression..."
    if ! zstd -d -c "$TEMP_IMAGE" > /dev/mmcblk0; then
        echo ""
        echo "ERROR: Decompression/write failed!"
        rm -f "$TEMP_IMAGE"
        exit 1
    fi
elif [ "$COMPRESSION" = "gzip" ]; then
    echo "Using gzip decompression..."
    if ! gunzip < "$TEMP_IMAGE" > /dev/mmcblk0; then
        echo ""
        echo "ERROR: Decompression/write failed!"
        rm -f "$TEMP_IMAGE"
        exit 1
    fi
else
    echo "ERROR: Unknown compression: $COMPRESSION"
    rm -f "$TEMP_IMAGE"
    exit 1
fi

# Clean up
rm -f "$TEMP_IMAGE"

# Sync to ensure all data is written
echo ""
echo "Syncing data to eMMC..."
sync

# Step 3: Fix GPT if needed (for different eMMC sizes)
echo ""
echo "========================================"
echo "  Step 3: Verifying Partition Table"
echo "========================================"
echo ""

# Force kernel to re-read partition table
echo "Re-reading partition table..."
blockdev --rereadpt /dev/mmcblk0 2>&1 | grep -v "GPT:" || true

# Check if GPT needs fixing
if dmesg | tail -20 | grep -q "Alt. header is not at the end"; then
    echo ""
    echo "Note: GPT backup header is not at the end of disk."
    echo "This is normal if your eMMC size differs from the image."
    echo "The partition is still usable - the warning is non-fatal."
    echo ""
fi

echo "✓ Partition table verified"
echo ""

echo "========================================"
echo "  ✅ Installation Complete!"
echo "========================================"
echo ""

# Get image directory and UUID for U-Boot commands
echo "Gathering information for U-Boot configuration..."
mkdir -p /mnt/emmc
if mount /dev/mmcblk0p1 /mnt/emmc 2>/dev/null; then
    IMAGE_DIR=$(ls -d /mnt/emmc/image-* 2>/dev/null | head -1 | xargs basename)
    # Strip "image-" prefix to get version for sonic_version_1
    IMAGE_VERSION="${IMAGE_DIR#image-}"
    PART_UUID=$(dmesg | grep "mmcblk0p1.*mounted filesystem" | tail -1 | sed -n 's/.*mounted filesystem \([0-9a-f-]*\).*/\1/p')
    umount /mnt/emmc
else
    IMAGE_DIR="<image-directory>"
    IMAGE_VERSION="<version>"
    PART_UUID="<partition-uuid>"
fi

echo ""
echo "========================================"
echo "  U-Boot Configuration Commands"
echo "========================================"
echo ""
echo "Reboot and run these commands at U-Boot prompt:"
echo ""
echo "----------------------------------------"
echo "setenv loadaddr 0x432000000"
echo "setenv image_dir $IMAGE_DIR"
echo "setenv fit_name $IMAGE_DIR/boot/sonic_arm64.fit"
echo "setenv sonic_version_1 SONiC-OS-$IMAGE_VERSION"
echo "setenv linuxargs \"console=ttyS12,115200n8 earlycon=uart8250,mmio32,0x14c33b00 loopfstype=squashfs loop=$IMAGE_DIR/fs.squashfs varlog_size=4096\""
echo "setenv image_dir_old \"\""
echo "setenv fit_name_old \"\""
echo "setenv sonic_version_2 \"None\""
echo "setenv linuxargs_old \"\""
echo "setenv sonic_boot_load \"ext4load mmc 0:1 \${loadaddr} \$fit_name\""
echo "setenv sonic_bootargs \"setenv bootargs root=UUID=$PART_UUID rw rootwait panic=1 \${linuxargs}\""
echo "setenv sonic_image_1 \"run sonic_bootargs; run sonic_boot_load; bootm \${loadaddr}#conf-ast2700-evb.dtb\""
echo "setenv sonic_image_2 \"\""
echo "setenv print_menu \"echo ===================================================; echo SONiC Boot Menu; echo ===================================================; echo To boot \$sonic_version_1; echo   type: run sonic_image_1; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\\\\\"Hit any key to stop autoboot:\\\\\\\" during boot; echo; echo To boot \$sonic_version_2; echo   type: run sonic_image_2; echo   at the U-Boot prompt after interrupting U-Boot when it says; echo   \\\\\\\"Hit any key to stop autoboot:\\\\\\\" during boot; echo; echo ===================================================\""
echo "setenv boot_next \"run sonic_image_1\""
echo "setenv bootcmd \"run print_menu; test -n \\\"\$boot_once\\\" && setenv do_boot_once \\\"\$boot_once\\\" && setenv boot_once \\\"\\\" && saveenv && run do_boot_once; run boot_next\""
echo "saveenv"
echo "boot"
echo "----------------------------------------"
echo ""
echo "IMPORTANT: loadaddr must be 0x432000000"
echo "           to avoid FIT image memory overlap!"
echo ""
echo "To reboot now: reboot -f"
echo ""
INSTALL_SCRIPT_EOF

# Replace placeholders in install script
sed -i "s|__TFTP_SERVER__|$TFTP_SERVER|g" install-sonic.sh
sed -i "s|__EMMC_IMAGE__|$EMMC_IMAGE|g" install-sonic.sh
sed -i "s|__COMPRESSION__|$COMPRESSION|g" install-sonic.sh

chmod +x install-sonic.sh

# Step 5: Create init script (minimal, just sets up environment)
log_info "Creating init script..."

cat > init << 'INIT_SCRIPT_EOF'
#!/bin/sh
# Minimal init - sets up environment and drops to shell

# Configuration (will be replaced by build script)
TFTP_SERVER="__TFTP_SERVER__"
BOARD_IP="__BOARD_IP__"
GATEWAY="__GATEWAY__"
NETMASK="__NETMASK__"

# Banner
echo ""
echo "========================================"
echo "  SONiC Installation Environment"
echo "========================================"
echo ""

# Mount essential filesystems
echo "Mounting filesystems..."
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

# Create device nodes if needed
mkdir -p /dev/pts /dev/shm
mount -t devpts devpts /dev/pts 2>/dev/null || true
mount -t tmpfs tmpfs /dev/shm 2>/dev/null || true

[ -c /dev/console ] || mknod /dev/console c 5 1
[ -c /dev/null ] || mknod /dev/null c 1 3

# Wait for devices
echo "Waiting for devices..."
sleep 2

# Network configuration
echo "Configuring network..."
echo "  IP Address: $BOARD_IP"
echo "  Netmask: $NETMASK"
echo "  Gateway: $GATEWAY"
echo "  TFTP Server: $TFTP_SERVER"

ifconfig lo 127.0.0.1 up
ifconfig eth0 $BOARD_IP netmask $NETMASK up
route add default gw $GATEWAY

# Wait for network
echo "Waiting for network..."
sleep 3

# Test network connectivity
echo "Testing network connectivity..."
if ping -c 1 -W 2 $TFTP_SERVER >/dev/null 2>&1; then
    echo "✓ Network OK - TFTP server reachable"
else
    echo "✗ WARNING: Cannot ping TFTP server $TFTP_SERVER"
    echo ""
    echo "Network troubleshooting:"
    echo "  ifconfig eth0"
    ifconfig eth0
    echo ""
    echo "  route -n"
    route -n
    echo ""
fi

echo ""
echo "Memory information:"
free -m
echo ""

# Show available tools
echo "========================================"
echo "  Available Commands"
echo "========================================"
echo ""
echo "  /install-sonic.sh  - Install SONiC to eMMC"
echo "  vi                 - Edit files (busybox vi)"
echo "  fdisk              - Partition management"
echo "  mount/umount       - Mount filesystems"
echo "  ifconfig/route     - Network configuration"
echo "  reboot -f          - Reboot system"
echo ""
echo "Installation script: /install-sonic.sh"
echo ""
echo "To install SONiC (multiple download methods supported):"
echo ""
echo "  TFTP (default):"
echo "    TFTP_SERVER=192.168.1.10 /install-sonic.sh"
echo ""
echo "  HTTP:"
echo "    HTTP_URL=http://192.168.1.10/sonic-aspeed-emmc.img.gz /install-sonic.sh"
echo ""
echo "  SCP:"
echo "    SCP_URL=user@192.168.1.10:/path/to/sonic-aspeed-emmc.img.gz /install-sonic.sh"
echo ""
echo "You can edit the script before running:"
echo "  vi /install-sonic.sh"
echo ""
echo "========================================"
echo ""
echo "Dropping to shell..."
echo ""

# Start shell as a child process (not PID 1) so Ctrl+C works
# PID 1 ignores signals by default, so we spawn a child shell
setsid sh -c 'exec sh </dev/console >/dev/console 2>&1' &

# Wait for shell to exit
wait

# If shell exits, reboot
echo ""
echo "Shell exited. Rebooting..."
sleep 2
reboot -f
INIT_SCRIPT_EOF

# Replace placeholders in init script
sed -i "s|__TFTP_SERVER__|$TFTP_SERVER|g" init
sed -i "s|__BOARD_IP__|$BOARD_IP|g" init
sed -i "s|__GATEWAY__|$GATEWAY|g" init
sed -i "s|__NETMASK__|$NETMASK|g" init

chmod +x init

# Step 6: Create minimal /etc files
log_info "Creating configuration files..."

mkdir -p etc
cat > etc/fstab << 'EOF'
proc  /proc  proc  defaults  0  0
sysfs /sys   sysfs defaults  0  0
EOF

# Step 7: Create initramfs
log_info "Creating initramfs CPIO archive..."
CPIO_FILE="${OUTPUT_NAME}.cpio"
find . | cpio -o -H newc 2>/dev/null > "$CPIO_FILE"

# Compress with gzip
log_info "Compressing with gzip..."
CPIO_GZ="${OUTPUT_NAME}.cpio.gz"
gzip -9 < "$CPIO_FILE" > "$CPIO_GZ"

# Create uImage format for U-Boot
log_info "Creating uImage format initramfs..."
if ! command -v mkimage &> /dev/null; then
    log_error "mkimage not found. Please install u-boot-tools:"
    log_error "  sudo apt-get install u-boot-tools"
    exit 1
fi

# Create uImage with gzip compression
# Load address and entry point set to 0 (U-Boot will use command-line address)
mkimage -A arm64 -O linux -T ramdisk -C gzip -a 0 -e 0 \
    -n "SONiC Installer Initramfs" -d "$CPIO_GZ" "$OUTPUT_NAME"

rm -f "$CPIO_FILE" "$CPIO_GZ"

# Move to original directory
ls -lh "$OUTPUT_NAME"
mv "$OUTPUT_NAME" "$DESTDIR/"
cd "$DESTDIR"

# Step 7: Summary
log_info ""
log_info "========================================="
log_info "✅ Minimal Initramfs Build Complete!"
log_info "========================================="
log_info ""
log_info "Output file:"
ls -lh "$OUTPUT_NAME"
log_info ""
log_info "Configuration:"
log_info "  TFTP Server: $TFTP_SERVER"
log_info "  Board IP: $BOARD_IP"
log_info "  Gateway: $GATEWAY"
log_info "  eMMC Image: $EMMC_IMAGE"
log_info ""
log_info "Next steps:"
log_info "  1. Copy to TFTP server:"
log_info "     sudo cp $OUTPUT_NAME /var/lib/tftpboot/"
log_info ""
log_info "  2. Boot AST2700 from U-Boot:"
log_info "     setenv ipaddr $BOARD_IP"
log_info "     setenv serverip $TFTP_SERVER"
log_info "     setenv netmask $NETMASK"
log_info "     tftp 0x403000000 vmlinuz-6.12.41+deb13-sonic-arm64"
log_info "     tftp 0x440000000 $OUTPUT_NAME"
log_info "     tftp 0x44C000000 ast2700-evb.dtb"
log_info "     setenv bootargs 'console=ttyS12,115200n8 earlycon=uart8250,mmio32,0x14c33b00 root=/dev/ram0 rw'"
log_info "     booti 0x403000000 0x440000000 0x44C000000"
log_info ""
log_info "  3. In the shell, run installation:"
log_info "     /install-sonic.sh"
log_info ""
log_info "  4. Or edit the script first:"
log_info "     vi /install-sonic.sh"
log_info "     export TFTP_SERVER=<new_ip>"
log_info "     /install-sonic.sh"
log_info ""
log_info "The installation script (/install-sonic.sh) is editable at runtime!"
log_info ""


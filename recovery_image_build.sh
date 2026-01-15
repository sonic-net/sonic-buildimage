#!/bin/bash
#
# recovery_image_build.sh
#
# Builds the SONiC recovery image (UKI or FIT format).
#
# This script bundles the kernel, initramfs, and cmdline into a single
# executable binary (UKI) or FIT image for booting.
#
# Globals:
#   FILESYSTEM_ROOT
#   OUTPUT_RECOVERY_IMAGE
#   LINUX_KERNEL_VERSION
#   CONFIGURED_ARCH
#   SIGNING_KEY
#   SIGNING_CERT
#   COMPRESSION_ALGO
#   BOOT_IMAGE_TYPE

set -x -e

# Mandatory variables (will fail if not set)
FILESYSTEM_ROOT="${FILESYSTEM_ROOT:?FILESYSTEM_ROOT is required}"
OUTPUT_RECOVERY_IMAGE="${OUTPUT_RECOVERY_IMAGE:?OUTPUT_RECOVERY_IMAGE is required}"
LINUX_KERNEL_VERSION="${LINUX_KERNEL_VERSION:?LINUX_KERNEL_VERSION is required}"
CONFIGURED_ARCH="${CONFIGURED_ARCH:?CONFIGURED_ARCH is required}"

# Defaults (can be overridden by env vars)
SIGNING_KEY="${SIGNING_KEY:-dummy.key}"
SIGNING_CERT="${SIGNING_CERT:-dummy.crt}"
COMPRESSION_ALGO="${COMPRESSION_ALGO:-zstd}"
BOOT_IMAGE_TYPE="${BOOT_IMAGE_TYPE:-uki}"

#######################################
# Calculate the aligned start address for the next PE section.
# Arguments:
#   prev_offset: The start address of the previous section.
#   prev_file: The file containing the previous section's data.
#   alignment: The section alignment (e.g., 4096).
# Outputs:
#   Writes the new aligned offset to stdout.
#######################################
calculate_next_offset() {
  local -r prev_offset="$1"
  local -r prev_file="$2"
  local -r alignment="$3"

  local size
  size=$(stat -Lc%s "${prev_file}")
  local next_start=$((prev_offset + size))
  echo $((next_start + alignment - next_start % alignment))
}

#######################################
# Builds the initramfs from the filesystem root.
# Globals:
#   FILESYSTEM_ROOT
#   COMPRESSION_ALGO
# Arguments:
#   None
# Outputs:
#   Writes to ../initramfs.img
#######################################
build_initramfs() {
  local initramfs_start
  local initramfs_end

  # Ensure the latest rc.local with fixes is included
  sudo cp files/image_config/platform/rc.local "${FILESYSTEM_ROOT}/etc/rc.local"
  sudo chmod 755 "${FILESYSTEM_ROOT}/etc/rc.local"

  # Ensure Docker runs in RAMDISK mode
  sudo mkdir -p "${FILESYSTEM_ROOT}/etc/systemd/system/docker.service.d"
  echo '[Service]
Environment="DOCKER_RAMDISK=true"' | sudo tee "${FILESYSTEM_ROOT}/etc/systemd/system/docker.service.d/10-ramdisk.conf" > /dev/null

  initramfs_start=$(date +%s)

  # Resolve compression command before pipeline
  local compress_cmd
  case "${COMPRESSION_ALGO}" in
      zstd)
          # Use zstd -19 (ultra) for maximum compression to shrink UKI
          compress_cmd="zstd -19 -T0"
          ;;
      xz)
          compress_cmd="xz --check=crc32 --lzma2=dict=1MiB"
          ;;
      *)
          echo "Unknown compression method: ${COMPRESSION_ALGO}"
          exit 1
          ;;
  esac

  pushd "${FILESYSTEM_ROOT}" > /dev/null
    # 1. Start by finding all files recursively, excluding ./boot and other large/unneeded directories
    sudo find . \
        \( -path ./boot \
        -o -path ./proc \
        -o -path ./sys \
        -o -path ./dev \
        -o -path ./tmp \
        -o -path ./run \
        -o -path ./mnt \
        -o -path ./media \
        -o -path ./var/cache/apt \
        -o -path ./var/log \
        -o -path ./usr/share/doc \
        -o -path ./usr/share/man \
        -o -path ./usr/share/info \
        -o -path ./usr/share/locale \
        -o -path ./usr/share/vim \
        -o -path ./usr/share/zoneinfo \
        -o -path ./usr/share/sonic/device \
        -o -path ./usr/local/share/man \
        -o -path ./usr/local/share/doc \
        -o -path ./usr/local/yang-models \
        -o -path ./usr/local/cvlyang-models \
        -o -path ./usr/lib/modules/*/kernel/drivers/gpu \
        -o -path ./usr/lib/modules/*/kernel/drivers/sound \
        -o -path ./usr/lib/modules/*/kernel/drivers/media \
        -o -path ./usr/lib/modules/*/kernel/drivers/infiniband \
        -o -path ./usr/lib/modules/*/kernel/drivers/net/wireless \
        -o -name "__pycache__" \
        -o -name "*.pyc" \
        -o -name "*.a" \
        \) -prune -o -print0 | \
    LC_ALL=C sort -z | \
    sudo cpio -o -H newc -0 --quiet -R +0:+0 | ${compress_cmd} > ../initramfs.img
  popd > /dev/null
  initramfs_end=$(date +%s)
  echo "Initramfs generation took $((initramfs_end - initramfs_start)) seconds"
}

#######################################
# Generates a FIT image (Universal Payload compatible).
# Globals:
#   FILESYSTEM_ROOT
#   LINUX_KERNEL_VERSION
#   CONFIGURED_ARCH
#   OUTPUT_RECOVERY_IMAGE
# Arguments:
#   None
#######################################
generate_fit_image() {
  echo "Generating FIT image (Universal Payload compatible)..."

  # Generate ITS file
  cat << EOF > target.its
/dts-v1/;

/ {
    description = "SONIE FIT Image";
    #address-cells = <1>;

    images {
        kernel {
            description = "Linux Kernel";
            data = /incbin/("${FILESYSTEM_ROOT}/boot/vmlinuz-${LINUX_KERNEL_VERSION}-${CONFIGURED_ARCH}");
            type = "kernel";
            arch = "x86_64";
            os = "linux";
            compression = "none";
            load = <0x1000000>;
            entry = <0x1000000>;
            hash-1 {
                algo = "sha256";
            };
        };
        ramdisk {
            description = "Initramfs";
            data = /incbin/("./initramfs.img");
            type = "ramdisk";
            arch = "x86_64";
            os = "linux";
            compression = "none";
            hash-1 {
                algo = "sha256";
            };
        };
    };

    configurations {
        default = "conf-1";
        conf-1 {
            description = "Boot Linux Kernel with Ramdisk";
            kernel = "kernel";
            ramdisk = "ramdisk";
            /* * Boot arguments can be hardcoded here.
             * UPL firmware usually appends these to its own arguments.
             */
            bootargs = "$(cat cmdline.txt)";
        };
    };
};
EOF
  mkimage -f target.its "${OUTPUT_RECOVERY_IMAGE}.tmp"
  rm target.its
}

#######################################
# Generates a UKI (Unified Kernel Image).
# Globals:
#   FILESYSTEM_ROOT
#   LINUX_KERNEL_VERSION
#   CONFIGURED_ARCH
#   OUTPUT_RECOVERY_IMAGE
# Arguments:
#   None
#######################################
generate_uki() {
  local efi_stub="${FILESYSTEM_ROOT}/usr/lib/systemd/boot/efi/linuxx64.efi.stub"
  local linux_kernel="${FILESYSTEM_ROOT}/boot/vmlinuz-${LINUX_KERNEL_VERSION}-${CONFIGURED_ARCH}"
  local os_release="${FILESYSTEM_ROOT}/usr/lib/os-release"

  # Systemd's UEFI stub acts as the main PE (Portable Executable) file.
  # We need to extract its Section Alignment boundary so new sections
  # (kernel, initramfs, etc.) are loaded at valid memory addresses.
  local align
  align="$(objdump -p "$efi_stub" | awk '{ if ($1 == "SectionAlignment"){print $2} }')"
  align=$((16#$align)) # Convert from hex to base-10 integer

  # Calculate the end position of the existing EFI stub sections.
  # We sum up the sizes and offsets of all existing sections to find the starting point.
  local stub_end
  stub_end="$(objdump -h "$efi_stub" | awk 'NF==7 {size=strtonum("0x"$3); offset=strtonum("0x"$4)} END {print size + offset}')"

  # --- Calculate Virtual Memory Addresses (VMA) for each new section ---
  # UEFI requires each PE section to start at an address aligned to the `SectionAlignment` boundary.
  # The formula `offset + align - offset % align` pads the address to the next alignment boundary.

  # 1. .osrel section (OS Release info)
  local osrel_offs
  osrel_offs=$((stub_end + align - stub_end % align))

  # --- Stitch sections into the final UKI binary ---
  # Create a staging directory to copy root-owned artifacts so we can read them without sudo
  local staging_dir
  staging_dir=$(mktemp -d)

  # Copy artifacts and take ownership
  # os-release might be world-readable but its parent dir might not be executable for us
  sudo cp "${os_release}" "${staging_dir}/os-release"
  sudo cp "${linux_kernel}" "${staging_dir}/linux-kernel"
  sudo chown "$(id -u):$(id -g)" "${staging_dir}/os-release" "${staging_dir}/linux-kernel"

  # Use staged files for calculations and objcopy
  local staged_os_release="${staging_dir}/os-release"
  local staged_kernel="${staging_dir}/linux-kernel"

  # Re-calculate offsets using staged files (size should remain identical, but just to be safe/consistent)
  # 2. .cmdline section (Kernel parameters)
  local cmdline_offs
  cmdline_offs=$(calculate_next_offset "${osrel_offs}" "${staged_os_release}" "${align}")

  # 3. .initrd section (Initramfs)
  local initramfs_offs
  initramfs_offs=$(calculate_next_offset "${cmdline_offs}" "cmdline.txt" "${align}")

  # 4. .linux section (The actual Linux kernel)
  local linux_offs
  linux_offs=$(calculate_next_offset "${initramfs_offs}" "./initramfs.img" "${align}")

  # objcopy injects these files as new PE sections into the systemd-boot stub.
  # No sudo needed now
  objcopy \
    --add-section .osrel="${staged_os_release}" \
    --change-section-vma .osrel=$(printf 0x%x "$osrel_offs") \
    --add-section .cmdline="cmdline.txt" \
    --change-section-vma .cmdline=$(printf 0x%x "$cmdline_offs") \
    --add-section .initrd="./initramfs.img" \
    --change-section-vma .initrd=$(printf 0x%x "$initramfs_offs") \
    --add-section .linux="${staged_kernel}" \
    --change-section-vma .linux=$(printf 0x%x "$linux_offs") \
    "$efi_stub" \
    "${OUTPUT_RECOVERY_IMAGE}.tmp"

  # Cleanup
  rm -rf "${staging_dir}"
}

#######################################
# Signs the generated image with sbsign.
# Globals:
#   SIGNING_KEY
#   SIGNING_CERT
#   OUTPUT_RECOVERY_IMAGE
# Arguments:
#   None
#######################################
sign_image() {
  # If you don't have keys, generate dummy ones just to fix the headers:
  if [[ ! -f "${SIGNING_KEY}" ]]; then
    openssl req -new -x509 -newkey rsa:2048 -keyout dummy.key -out dummy.crt -nodes -days 1 -subj "/CN=Dummy/"
    SIGNING_KEY="dummy.key"
    SIGNING_CERT="dummy.crt"
  fi

  if ! sbsign --key "${SIGNING_KEY}" \
              --cert "${SIGNING_CERT}" \
              --output "${OUTPUT_RECOVERY_IMAGE}" \
              "${OUTPUT_RECOVERY_IMAGE}.tmp"; then
    echo "Signing with real key failed, trying dummy key..."
    # Ensure dummy keys exist if fallback needed
    if [[ ! -f "dummy.key" ]]; then
        openssl req -new -x509 -newkey rsa:2048 -keyout dummy.key -out dummy.crt -nodes -days 1 -subj "/CN=Dummy/"
    fi
    if ! sbsign --key dummy.key \
           --cert dummy.crt \
           --output "${OUTPUT_RECOVERY_IMAGE}" \
           "${OUTPUT_RECOVERY_IMAGE}.tmp"; then
      echo "WARNING: sbsign failed (likely due to file size). Using unsigned image."
      cp "${OUTPUT_RECOVERY_IMAGE}.tmp" "${OUTPUT_RECOVERY_IMAGE}"
    fi
  fi
  rm "${OUTPUT_RECOVERY_IMAGE}.tmp"
}

#######################################
# Main entry point.
# Arguments:
#   None
#######################################
main() {
  echo "Building image type: ${BOOT_IMAGE_TYPE}"

  if [[ "${IMAGE_TYPE}" == "recovery" ]] || [[ -z "${IMAGE_TYPE}" ]]; then
    build_initramfs

    # Ensure cmdline.txt exists (if not generated by caller)
    if [ ! -f cmdline.txt ]; then
        echo -n "root=/dev/ram0 systemd.unit=multi-user.target console=tty0 console=ttyS0,115200 onie_platform=${ONIE_PLATFORM:-vs} bg_mac=00:00:00:00:00:00 bonding.max_bonds=0 sonic_asic_platform=vs systemd.show_status=true rc.local_debug=y overlay_tmpfs=on tmpfs_size=6g" > cmdline.txt
    fi

    if [ "${BOOT_IMAGE_TYPE}" = "fit" ]; then
      generate_fit_image
    else
      generate_uki
    fi

    sign_image
  fi
}

main "$@"

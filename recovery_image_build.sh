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
SIGNING_KEY="${SIGNING_KEY:-db.key}"
SIGNING_CERT="${SIGNING_CERT:-db.crt}"
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
  local -r padding="${4:-0}" 
  local size
  size=$(stat -Lc%s "${prev_file}")
  # Use ceiling division to align the next start address
  local raw_end=$((prev_offset + size + padding))
  echo $(((raw_end + alignment - 1) / alignment * alignment))
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

  # Install ONIE discovery service only for SONIE recovery images
  if [[ "${IMAGE_TYPE}" == "recovery" ]]; then
      echo "Installing ONIE discovery service for SONIE build..."
      sudo cp files/image_config/onie-discovery/onie-discovery.service "${FILESYSTEM_ROOT}/etc/systemd/system/"
      sudo cp files/image_config/onie-discovery/onie-discovery.sh "${FILESYSTEM_ROOT}/usr/bin/"
      sudo cp files/image_config/onie-discovery/onie-dhcp-event.sh "${FILESYSTEM_ROOT}/usr/bin/"
      sudo chmod 755 "${FILESYSTEM_ROOT}/usr/bin/onie-discovery.sh" "${FILESYSTEM_ROOT}/usr/bin/onie-dhcp-event.sh"
      sudo mkdir -p "${FILESYSTEM_ROOT}/etc/systemd/system/multi-user.target.wants"
      sudo ln -sf /etc/systemd/system/onie-discovery.service "${FILESYSTEM_ROOT}/etc/systemd/system/multi-user.target.wants/onie-discovery.service"
  fi

  initramfs_start=$(date +%s)

  # Resolve compression command before pipeline
  local compress_cmd
  case "${COMPRESSION_ALGO}" in
      zstd)
          # Use zstd -19 (ultra) for maximum compression to shrink UKI, or override with ZSTD_LEVEL
          compress_cmd="zstd -${ZSTD_LEVEL:-19} -T0"
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
        -o -path ./usr/lib/firmware \
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
# Generates a FIT image (UEFI Universal Payload compatible).
# Globals:
#   FILESYSTEM_ROOT
#   LINUX_KERNEL_VERSION
#   CONFIGURED_ARCH
#   OUTPUT_RECOVERY_IMAGE
# Arguments:
#   None
#######################################
generate_fit_image() {
  echo "Generating FIT image (UEFI Universal Payload compatible)..."

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

  echo "DEBUG: objcopy inputs:"
  echo "DEBUG:   osrel: ${staged_os_release} (offset: ${osrel_offs})"
  echo "DEBUG:   cmdline: cmdline.txt (offset: ${cmdline_offs})"
  echo "DEBUG:   initrd: ./initramfs.img (offset: ${initramfs_offs})"
  echo "DEBUG:   linux: ${staged_kernel} (offset: ${linux_offs})"
  echo "DEBUG:   stub: ${efi_stub}"
  ls -l "${staged_os_release}" cmdline.txt "./initramfs.img" "${staged_kernel}" "${efi_stub}" || echo "DEBUG: Some files missing!"

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

ensure_keys() {
  if [[ ! -f "${SIGNING_KEY}" ]]; then
    openssl req -new -x509 -newkey rsa:2048 -keyout db.key -out db.crt -nodes -days 1 -subj "/CN=DB/"
    SIGNING_KEY="db.key"
    SIGNING_CERT="db.crt"
  fi
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
  ensure_keys

  if ! sbsign --key "${SIGNING_KEY}" \
              --cert "${SIGNING_CERT}" \
              --output "${OUTPUT_RECOVERY_IMAGE}" \
              "${OUTPUT_RECOVERY_IMAGE}.tmp"; then
    echo "Signing with real key failed, trying db key..."
    # Ensure db keys exist if fallback needed
    if [[ ! -f "db.key" ]]; then
        openssl req -new -x509 -newkey rsa:2048 -keyout db.key -out db.crt -nodes -days 1 -subj "/CN=DB/"
    fi
    if ! sbsign --key db.key \
           --cert db.crt \
           --output "${OUTPUT_RECOVERY_IMAGE}" \
           "${OUTPUT_RECOVERY_IMAGE}.tmp"; then
      echo "WARNING: sbsign failed (likely due to file size). Using unsigned image."
      cp "${OUTPUT_RECOVERY_IMAGE}.tmp" "${OUTPUT_RECOVERY_IMAGE}"
    fi
  fi
  rm "${OUTPUT_RECOVERY_IMAGE}.tmp"

}

#######################################
# Generates Secure Boot enrollment files (.auth) for OVMF.
# Globals:
#   SIGNING_KEY
#   SIGNING_CERT
# Arguments:
#   None
#######################################
generate_ovmf_enrollment_files() {
  local key="${SIGNING_KEY}"
  local cert="${SIGNING_CERT}"
  local uuid="00000000-0000-0000-0000-000000000000"

  echo "Generating Secure Boot enrollment files from ${key} and ${cert}..."

  # Check if efitools are available
  if ! command -v cert-to-efi-sig-list &> /dev/null || ! command -v sign-efi-sig-list &> /dev/null; then
    echo "Warning: efitools not found. Skipping .auth file generation."
    return
  fi

  # Create ESL (EFI Signature List) files
  cert-to-efi-sig-list -g "$uuid" "$cert" PK.esl
  cert-to-efi-sig-list -g "$uuid" "$cert" KEK.esl
  cert-to-efi-sig-list -g "$uuid" "$cert" db.esl

  # Sign them to create Authenticated Variable updates (.auth)
  # PK is self-signed
  sign-efi-sig-list -k "$key" -c "$cert" PK PK.esl PK.auth
  # KEK is signed by PK
  sign-efi-sig-list -k "$key" -c "$cert" KEK KEK.esl KEK.auth
  # db is signed by KEK
  sign-efi-sig-list -k "$key" -c "$cert" db db.esl db.auth

  rm -f PK.esl KEK.esl db.esl
  echo "Generated PK.auth, KEK.auth, db.auth"
}

#######################################
# Re-signs kernel modules with the current key.
# Globals:
#   FILESYSTEM_ROOT
#   SIGNING_KEY
#   SIGNING_CERT
#   LINUX_KERNEL_VERSION
# Arguments:
#   None
#######################################
re_sign_modules() {
  echo "Checking for module re-signing tools..."

  # Find sign-file
  local sign_file
  sign_file=$(find /usr/lib -type f -name sign-file 2>/dev/null | head -n 1)

  if [[ -z "${sign_file}" ]]; then
      echo "Warning: sign-file not found in /usr/lib. Skipping module re-signing."
      return
  fi

  # Find extract-cert
  local extract_cert
  # Try sibling of sign-file first (../../certs/extract-cert usually)
  extract_cert="$(dirname "$(dirname "${sign_file}")")/certs/extract-cert"

  if [[ ! -f "${extract_cert}" ]]; then
      # Fallback global search
      extract_cert=$(find /usr/lib -type f -name extract-cert 2>/dev/null | head -n 1)
  fi

  if [[ -z "${extract_cert}" || ! -f "${extract_cert}" ]]; then
      echo "Warning: extract-cert not found. Skipping module re-signing."
      return
  fi

  # Optimization: Check if modules are already signed by our key
  # We check a sample module (crypto-sha512 or generic)
  # We compare the Serial Number of our cert with sig_key in modinfo
  local sample_mod
  sample_mod=$(find "${FILESYSTEM_ROOT}/lib/modules/${LINUX_KERNEL_VERSION}" -name "crypto-sha512.ko" -o -name "sha512_generic.ko" | head -n 1)

  if [[ -z "${sample_mod}" ]]; then
      sample_mod=$(find "${FILESYSTEM_ROOT}/lib/modules/${LINUX_KERNEL_VERSION}" -name "*.ko" | head -n 1)
  fi

  if [[ -n "${sample_mod}" ]]; then
      local my_serial
      my_serial=$(openssl x509 -in "${SIGNING_CERT}" -noout -serial | cut -d= -f2 | tr '[:lower:]' '[:upper:]')

      local mod_key
      mod_key=$(modinfo -F sig_key "${sample_mod}" | tr -d ':')

      if [[ "${my_serial}" == "${mod_key}" ]]; then
          echo "Modules already signed by current key (${my_serial}). Skipping re-signing."
          return
      else
          echo "Module signature mismatch (Module: ${mod_key} vs Key: ${my_serial}). Re-signing..."
      fi
  fi

  echo "Re-signing modules in ${FILESYSTEM_ROOT} using ${sign_file} (SHA256)..."

  # We use the existing helper script if available
  if [[ -f "scripts/signing_kernel_modules.sh" ]]; then
      # Make sure absolute paths are used if passing to sudo
      local abs_key="$(readlink -f "${SIGNING_KEY}")"
      local abs_cert="$(readlink -f "${SIGNING_CERT}")"

      # Determine correct module directory
      # files/image_config/platform/rc.local puts modules in /lib/modules usually
      local mod_dir="${FILESYSTEM_ROOT}/lib/modules/${LINUX_KERNEL_VERSION}"
      if [[ ! -d "${mod_dir}" ]]; then
           # Try searching
           mod_dir=$(find "${FILESYSTEM_ROOT}" -type d -name "${LINUX_KERNEL_VERSION}" | grep "/lib/modules/" | head -n 1)
      fi

      if [[ -d "${mod_dir}" ]]; then
        sudo ./scripts/signing_kernel_modules.sh \
            -l "${LINUX_KERNEL_VERSION}" \
            -c "${abs_cert}" \
            -p "${abs_key}" \
            -k "${mod_dir}" \
            -s "${sign_file}" \
            -e "${extract_cert}"
      else
        echo "Warning: Module directory for ${LINUX_KERNEL_VERSION} not found in ${FILESYSTEM_ROOT}."
      fi
  else
      echo "Warning: scripts/signing_kernel_modules.sh not found."
  fi
}

#######################################
# Builds a custom Grub binary using grub-mkimage.
# This allows us to exclude 'shim_lock' for direct UEFI booting.
# Globals:
#   FILESYSTEM_ROOT
# Arguments:
#   None
#######################################
build_custom_grub() {
  echo "Building custom Grub binary (monolithic)..."

  # Ensure target directory exists
  local target_dir="${FILESYSTEM_ROOT}/usr/lib/grub/x86_64-efi/monolithic"
  local target_file="${target_dir}/grubx64.efi"

  sudo mkdir -p "${target_dir}"

  # Copy config to chroot
  sudo cp files/image_config/grub-standalone.cfg "${FILESYSTEM_ROOT}/tmp/grub-standalone.cfg"

  # Modules to include
  local modules=(
  # Partition and FS
  part_gpt fat ext2 iso9660

  # Core Boot & Config
  linux normal chain boot configfile loadenv

  # Secure Boot & TPM
  tpm

  # Search & Utils
  search search_label search_fs_uuid search_fs_file
  echo ls test minicmd

  # Power
  reboot halt

  # Graphics
  serial
)

  # Check if grub-mkstandalone exists in chroot
  if ! sudo chroot "${FILESYSTEM_ROOT}" sh -c "command -v grub-mkstandalone" &>/dev/null; then
      echo "Error: grub-mkstandalone not found in chroot. Is grub-efi-amd64-bin installed?"
      return 1
  fi

  echo "Generating ${target_file}..."
  # We need to run this inside chroot to use the modules installed there
  # We use a relative path for output inside chroot
  local chroot_output="/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"
  local chroot_tmp_config="/tmp/grub-standalone.cfg"
  sudo chroot "${FILESYSTEM_ROOT}" grub-mkstandalone \
    --format x86_64-efi \
    --output "${chroot_output}" \
    --locales="" \
    --fonts="" \
    --modules="${modules[*]}" \
    "boot/grub/grub.cfg=${chroot_tmp_config}"
  sudo rm -f "${FILESYSTEM_ROOT}${chroot_tmp_config}"
  if [[ -f "${target_file}" ]]; then
      echo "Successfully built custom grubx64.efi at ${target_file}"
  else
      echo "Error: Failed to build custom grubx64.efi"
      exit 1
  fi
}

#######################################
# Signs the Grub binary if found in the filesystem.
# Globals:
#   FILESYSTEM_ROOT
#   SIGNING_KEY
#   SIGNING_CERT
# Arguments:
#   None
#######################################
sign_grub() {
  echo "Checking for Grub binary to sign..."
  local grub_path="${FILESYSTEM_ROOT}/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"

  if [[ ! -f "${grub_path}" ]]; then
      # Fallback to search
      grub_path=$(find "${FILESYSTEM_ROOT}/usr/lib/grub" -name grubx64.efi -print -quit 2>/dev/null)
  fi

  if [[ -f "${grub_path}" ]]; then
      echo "Found Grub at ${grub_path}"

      # Use absolute paths for sudo
      local abs_key="$(readlink -f "${SIGNING_KEY}")"
      local abs_cert="$(readlink -f "${SIGNING_CERT}")"

      # Verify if already signed by OUR key
      if sbverify --cert "${abs_cert}" "${grub_path}" &>/dev/null; then
         echo "Grub already signed with current certificate. Skipping."
         return
      fi

      echo "Signing Grub binary..."
      sudo sbsign --key "${abs_key}" --cert "${abs_cert}" --output "${grub_path}" "${grub_path}"

      # Verify signature immediately
      if sbverify --cert "${abs_cert}" "${grub_path}" &>/dev/null; then
          echo "Successfully signed ${grub_path}"
      else
          echo "Error: Signing failed for ${grub_path}"
          exit 1
      fi
  else
      echo "Warning: grubx64.efi not found in ${FILESYSTEM_ROOT}. Skipping Grub signing."
  fi
}

#######################################
main() {
  echo "Building image type: ${BOOT_IMAGE_TYPE}"

  if [[ "${IMAGE_TYPE}" == "recovery" ]] || [[ -z "${IMAGE_TYPE}" ]]; then
    if [[ "${SKIP_SIGNING}" != "1" ]]; then
      ensure_keys
    fi

    # Handle tarball input for Bazel/Artifact builds
    local tmp_fsroot=""
    if [[ -f "${FILESYSTEM_ROOT}" ]]; then
        echo "Extracting rootfs artifact: ${FILESYSTEM_ROOT}"
        tmp_fsroot=$(mktemp -d)
        # Use tar with auto-compression detection if possible, or simple -xf
        tar -xf "${FILESYSTEM_ROOT}" -C "${tmp_fsroot}"
        FILESYSTEM_ROOT="${tmp_fsroot}"

        # Ensure cleanup on exit
        trap 'rm -rf "${tmp_fsroot}"' EXIT
    fi

    # Re-sign modules if we are building a recovery image and have keys
    if [[ "${SKIP_SIGNING}" != "1" ]]; then
        if [[ "${SKIP_MODULE_SIGNING}" != "1" ]]; then
            re_sign_modules
        fi
    else
        echo "Stripping signatures from modules (SKIP_SIGNING=1)..."
        # We need to strip signatures because the kernel might reject them if they are signed by an unknown key
        # even if Secure Boot is disabled, depending on CONFIG_MODULE_SIG_FORCE or similar.
        # find "${FILESYSTEM_ROOT}/lib/modules" -name "*.ko" -exec strip --strip-debug --strip-unneeded {} +
        # Actually, standard 'strip' works.
        find "${FILESYSTEM_ROOT}/lib/modules" -type f -name "*.ko" -print0 | xargs -0 -P "$(nproc)" -I {} strip --strip-debug --strip-unneeded {}
    fi

    # Build custom GRUB binary (no shim_lock)
    build_custom_grub

    # Sign Grub binary
    if [[ "${SKIP_SIGNING}" != "1" ]]; then
      sign_grub
    fi

    build_initramfs

    # Ensure cmdline.txt exists (if not generated by caller)
    if [ ! -f cmdline.txt ]; then
        local -a cmdline_args=(
            "root=/dev/ram0"
            "systemd.unit=multi-user.target"
            "console=tty0"
            "console=ttyS0,115200"
            "onie_platform=${ONIE_PLATFORM:-vs}"
            "bg_mac=00:00:00:00:00:00"
            "bonding.max_bonds=0"
            "sonic_asic_platform=vs"
            "systemd.show_status=true"
            "rc.local_debug=y"
            "overlay_tmpfs=on"
            "tmpfs_size=8g"
        )
        echo -n "${cmdline_args[*]}" > cmdline.txt
    fi

    if [ "${BOOT_IMAGE_TYPE}" = "fit" ]; then
      generate_fit_image
    else
      generate_uki
    fi

    if [[ "${SKIP_SIGNING}" != "1" ]]; then
      sign_image
      generate_ovmf_enrollment_files
    fi
  fi
}

main "$@"

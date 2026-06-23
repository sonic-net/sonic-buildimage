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

set -x -euo pipefail

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
ENABLE_ZSTD="${ENABLE_ZSTD:-}"
SKIP_SIGNING="${SKIP_SIGNING:-1}"
SKIP_MODULE_SIGNING="${SKIP_MODULE_SIGNING:-1}"

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
# Place the unit file in the systemd path, and enable it for some system targets.
# Positional Arguments:
#   orig_path: Path to the unit file to be installed.
# Flag Arguments:
#   -w target: Enable the unit for $target. Can be passed multiple times.
# Globals:
#   systemd_path: Systemd unit path; if unset, use default.
#######################################
install_systemd_service() {
  local systemd_path="${systemd_path:-${FILESYSTEM_ROOT?}/usr/lib/systemd/system}"

  local args; args="$(getopt "-n${FUNCNAME}" -o w: -- "$@")" || return
  eval "set -- ${args}" || return

  local wanted_by=()

  while true; do
    case "$1" in
      -w) wanted_by+=("$2"); shift 2 ;;
      --) shift; break ;;
      *) return 2 ;;
    esac
  done

  [[ "$#" -eq 1 ]] || return 2

  local orig_path="$1"
  local unit_name="${orig_path##*/}"

  sudo install -D -m 0644 "${orig_path}" "${systemd_path}/${unit_name}"

  for target in "${wanted_by[@]}"; do
    local wants_path="${systemd_path}/${target}.target.wants"
    sudo mkdir -p "${wants_path}"
    sudo ln -sf "/usr/lib/systemd/system/${unit_name}" "${wants_path}"
  done
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

  # Install boot-control files
  echo "Installing boot-control files..."
  if [ -d "files/image_config/boot-control" ]; then
      local boot_control_dir="files/image_config/boot-control"

      sudo install -D -m 755 "${boot_control_dir}/set_sonic_boot.sh" "${FILESYSTEM_ROOT}/usr/local/bin/set_sonic_boot.sh"

      install_systemd_service "${boot_control_dir}/sonic-boot-next.service" -w shutdown -w reboot
  else
      echo "Warning: files/image_config/boot-control not found!"
  fi


  # Install SONIE discovery service only for SONIE recovery images
  if [[ "${IMAGE_TYPE}" == "recovery" ]]; then
      echo "Installing SONIE discovery service for SONIE build..."
      sudo install -m0755 files/image_config/sonie-discovery/sonie-discovery.sh "${FILESYSTEM_ROOT}/usr/bin/sonie-discovery.sh"
      install_systemd_service files/image_config/sonie-discovery/sonie-discovery.service -w multi-user
  fi

  initramfs_start=$(date +%s)

  # Resolve compression command before pipeline
  local compress_cmd
  case "${COMPRESSION_ALGO}" in
      zstd)
          # Use zstd -19 (ultra) for maximum compression to shrink UKI, or override with ZSTD_LEVEL.
          # We use --long=23 (8MB window size limit) for better compression without breaking the kernel initramfs decompressor.
          compress_cmd="zstd -1 -T0"
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
        -o -path ./var/lib/docker \
        -o -path ./var/log \
        -o -path ./usr/share/doc \
        -o -path ./usr/share/man \
        -o -path ./usr/share/info \
        -o -path ./usr/share/locale \
        -o -path ./usr/share/vim \
        -o -path ./usr/share/zoneinfo \
        -o -path ./usr/lib/firmware \
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
  local stub_arch="x64"
  if [[ "${CONFIGURED_ARCH}" == "arm64" ]]; then
      stub_arch="aa64"
  elif [[ "${CONFIGURED_ARCH}" == "armhf" ]]; then
      stub_arch="arm"
  fi
  local efi_stub="${FILESYSTEM_ROOT}/usr/lib/systemd/boot/efi/linux${stub_arch}.efi.stub"
  local linux_kernel="${FILESYSTEM_ROOT}/boot/vmlinuz-${LINUX_KERNEL_VERSION}-${CONFIGURED_ARCH}"
  local os_release="${FILESYSTEM_ROOT}/usr/lib/os-release"

  echo "Checking and installing necessary secureboot and uki host tools..."
  if ! command -v ukify &> /dev/null || ! command -v sbsign &> /dev/null || ! command -v cert-to-efi-sig-list &> /dev/null; then
      echo "Installing systemd-ukify, sbsigntool, and efitools on host..."
      sudo apt-get -y update || true
      sudo apt-get -y install systemd-ukify sbsigntool efitools || true
      if ! command -v ukify &> /dev/null; then
          pip3 install --user ukify || true
      fi
  fi
  # Prepare inputs for ukify

  # Note: ukify build arguments:
  # --linux: The kernel image
  # --initrd: The initramfs
  # --cmdline: Kernel command line
  # --os-release: The os-release file
  # --stub: The UEFI stub
  # --output: The output UKI file

  # Clean up potential root-owned previous build artifacts
  sudo rm -f "${OUTPUT_RECOVERY_IMAGE}" "${OUTPUT_RECOVERY_IMAGE}.tmp"

  # Ensure kernel and its directory are accessible by non-root user running ukify
  sudo chmod 755 "${FILESYSTEM_ROOT}/boot"
  sudo chmod 644 "${linux_kernel}"

  local ukify_bin="ukify"
  if ! command -v ukify &> /dev/null; then
      if [[ -f "./src/systemd-ukify/ukify" ]]; then
          ukify_bin="python3 ./src/systemd-ukify/ukify"
      elif [[ -f "../src/systemd-ukify/ukify" ]]; then
          ukify_bin="python3 ../src/systemd-ukify/ukify"
      else
          echo "Error: ukify tool not found!"
          exit 1
      fi
  fi

  ${ukify_bin} build \
    --linux="${linux_kernel}" \
    --initrd="./initramfs.img" \
    --cmdline="@cmdline.txt" \
    --os-release="@${os_release}" \
    --stub="${efi_stub}" \
    --output="${OUTPUT_RECOVERY_IMAGE}.tmp"

  echo "Generating PE Addon for recovery mode..."
  ${ukify_bin} build \
    --cmdline="boot_reason=recovery" \
    --stub="${efi_stub}" \
    --output="${OUTPUT_RECOVERY_IMAGE%.efi}-recovery.addon.efi"
}

#######################################
# Ensures the signing keys exist, creating them if necessary.
# Globals:
#   SIGNING_KEY
#   SIGNING_CERT
# Arguments:
#   None
#######################################
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
  sample_mod=$(find "${FILESYSTEM_ROOT}/lib/modules/"* -name "crypto-sha512.ko" -o -name "sha512_generic.ko" 2>/dev/null | head -n 1)

  if [[ -z "${sample_mod}" ]]; then
      sample_mod=$(find "${FILESYSTEM_ROOT}/lib/modules/"* -name "*.ko" 2>/dev/null | head -n 1)
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
           # Try searching with wildcard to handle arch suffixes like -amd64
           mod_dir=$(find "${FILESYSTEM_ROOT}" -type d -name "*${LINUX_KERNEL_VERSION}*" | grep "/lib/modules/" | head -n 1)
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
  echo ls cat test minicmd lsmmap smbios acpi lsefi lsefimmap lsefisystab lspci

  # Power
  reboot halt

  # Graphics
  serial
)

  # Fix dynamic linker and library paths for non-merged-usr chroot
  if [[ ! -L "${FILESYSTEM_ROOT}/lib/x86_64-linux-gnu" ]]; then
      sudo mkdir -p "${FILESYSTEM_ROOT}/lib"
      sudo ln -s "/usr/lib/x86_64-linux-gnu" "${FILESYSTEM_ROOT}/lib/x86_64-linux-gnu"
  fi

  # Check if grub-mkstandalone exists in chroot
  if ! sudo chroot "${FILESYSTEM_ROOT}" test -x /usr/bin/grub-mkstandalone; then
      echo "Error: grub-mkstandalone not found in chroot at /usr/bin/grub-mkstandalone"
      return 1
  fi

  echo "Generating ${target_file}..."
  # We need to run this inside chroot to use the modules installed there

  # Install grub-efi-amd64-bin temporarily into chroot so grub-mkstandalone can find the modules
  local grub_efi_deb=$(sudo find "${FILESYSTEM_ROOT}" -name "grub-efi-amd64-bin*.deb" -print -quit)
  if [[ -n "${grub_efi_deb}" ]]; then
      local deb_base=$(basename "${grub_efi_deb}")
      sudo cp "${grub_efi_deb}" "${FILESYSTEM_ROOT}/tmp/"
      sudo chroot "${FILESYSTEM_ROOT}" dpkg -i "/tmp/${deb_base}" || true
  fi

  # We use a relative path for output inside chroot
  local chroot_output="/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"
  local chroot_tmp_config="/tmp/grub-standalone.cfg"
  sudo chroot "${FILESYSTEM_ROOT}" /usr/bin/grub-mkstandalone \
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
        find "${FILESYSTEM_ROOT}/lib/modules" -type f -name "*.ko" -print0 | xargs -0 -P "$(nproc)" -I {} sudo strip --strip-debug --strip-unneeded {}
    fi

    if [[ "${TARGET_BOOTLOADER}" = "grub" ]]; then
        # Build custom GRUB binary (no shim_lock)
        build_custom_grub

        # Sign grub.
        [[ "${SKIP_SIGNING}" != "1" ]] && sign_grub
    elif [[ "${TARGET_BOOTLOADER}" = "systemd-boot" ]]; then
        echo "TARGET_BOOTLOADER=${TARGET_BOOTLOADER}... Nothing to do."
    else
        echo "unsupported TARGET_BOOTLOADER (${TARGET_BOOTLOADER:-<unset>}). Expected either grub or systemd-boot."
        return 1;
    fi

    build_initramfs

    # Always regenerate cmdline.txt to avoid poisoning from previous test runs
    # (e.g. test_ukify_build.sh which creates a 'console=ttyS0' dummy).
    local -a cmdline_args=(
        "root=/dev/ram0"
        "systemd.unit=multi-user.target"
        "console=tty0"
        "console=ttyS0,115200"
        "onie_platform=${ONIE_PLATFORM:-vs}"
        "bg_mac=00:00:00:00:00:00"
        "bonding.max_bonds=0"
        "sonic_asic_platform=${SONIC_ASIC_PLATFORM:-${CONFIGURED_PLATFORM:-vs}}"
        "systemd.show_status=true"
        "rc.local_debug=y"
        "overlay_tmpfs=on"
        "tmpfs_size=16g"
    )
    local final_cmdline="${cmdline_args[*]}"

    echo -n "${final_cmdline}" > cmdline.txt

    if [ "${BOOT_IMAGE_TYPE}" = "fit" ]; then
      generate_fit_image
    else
      generate_uki
    fi

    if [[ "${SKIP_SIGNING}" != "1" ]]; then
      sign_image
      generate_ovmf_enrollment_files
    else
      if [[ -f "${OUTPUT_RECOVERY_IMAGE}.tmp" ]]; then
          mv "${OUTPUT_RECOVERY_IMAGE}.tmp" "${OUTPUT_RECOVERY_IMAGE}"
      fi
    fi

  fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi

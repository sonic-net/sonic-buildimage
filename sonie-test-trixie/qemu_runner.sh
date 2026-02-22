#!/bin/bash
#
# QEMU Runner Script for SONiC
#
# This script sets up the network environment (bridge, tap), configures
# services (dnsmasq, python HTTP server) via templates, generates a client
# disk image, and launches QEMU.
#

set -eu
set -o pipefail
set -x

# --- Defaults ---
# Usage of "readonly" where appropriate for constants
readonly DISK_IMG_SIZE="${DISK_IMG_SIZE:-128G}"
readonly HTTP_PORT="${HTTP_PORT:-8000}"
readonly CLIENT_DISK_IMG="${CLIENT_DISK_IMG:-client_disk.img}"
readonly CLIENT_MAC="${CLIENT_MAC:-52:54:00:12:34:56}"
readonly CLIENT_OVMF_VARS="${CLIENT_OVMF_VARS:-OVMF_VARS.fd}"
readonly UKI_FILE="${UKI_FILE:-sonie.efi}"
readonly IMAGESET_FILE="${IMAGESET_FILE:-imageset.json}"
readonly SECURE_BOOT="${SECURE_BOOT:-0}"

# --- IPv6 Configuration ---
readonly USE_IPV6="${USE_IPV6:-0}"
readonly BRIDGE_IP6="${BRIDGE_IP6:-fd00::1}"
readonly CLIENT_IP6_START="${CLIENT_IP6_START:-fd00::2}"
readonly CLIENT_IP6_END="${CLIENT_IP6_END:-fd00::10}"

# --- IPv4 Configuration ---
readonly BRIDGE_IP="${BRIDGE_IP:-192.168.100.1}"
readonly CLIENT_IP_START="${CLIENT_IP_START:-192.168.100.2}"
readonly CLIENT_IP_END="${CLIENT_IP_END:-192.168.100.10}"

# --- Constants ---
readonly SCRIPT_DIR="$(dirname "$0")"
# Templates are copied to /etc/qemu-runner/templates in the Docker image
readonly TEMPLATE_DIR="/etc/qemu-runner/templates"
readonly DNSMASQ_CONF="/etc/dnsmasq.conf"
readonly LOG_DIR="/data/sonie-test-logs"

# Ensure log directory exists
mkdir -p "${LOG_DIR}"

# Global PIDs for cleanup (mutable)
DNSMASQ_PID=""
HTTP_PID=""
SWTPM_PID=""
TPM_DIR=""

#######################################
# Clean up background processes and temporary resources.
# Globals:
#   DNSMASQ_PID
#   HTTP_PID
# Arguments:
#   None
#######################################
cleanup() {
  echo "Cleaning up..."
  if [[ -n "${DNSMASQ_PID}" ]]; then
    echo "Killing dnsmasq (PID ${DNSMASQ_PID})"
    kill "${DNSMASQ_PID}" 2>/dev/null || true
  fi
  if [[ -n "${HTTP_PID}" ]]; then
    echo "Killing python server (PID ${HTTP_PID})"
    kill "${HTTP_PID}" 2>/dev/null || true
  fi
  if [[ -n "${SWTPM_PID}" ]]; then
    echo "Killing swtpm (PID ${SWTPM_PID})"
    kill "${SWTPM_PID}" 2>/dev/null || true
  fi
  if [[ -n "${TPM_DIR}" && -d "${TPM_DIR}" ]]; then
    rm -rf "${TPM_DIR}"
  fi
}
trap cleanup EXIT

#######################################
# Check if required dependencies are installed.
# Arguments:
#   None
# Returns:
#   1 if a dependency is missing.
#######################################
check_dependencies() {
  local -r deps=("qemu-system-x86_64" "envsubst" "dnsmasq" "python3" "ss" "ip" "mkfs.vfat" "mcopy" "sgdisk" "tcpdump")
  for dep in "${deps[@]}"; do
    if ! command -v "${dep}" &> /dev/null; then
      echo "Error: Required dependency '${dep}' not found."
      return 1
    fi
  done
}

#######################################
# Setup network bridge and tap interface.
# Globals:
#   None
# Arguments:
#   None
#######################################
setup_network() {
  echo 'Setting up Bridge br0...'
  [[ ! -d /dev/net ]] && mkdir -p /dev/net
  [[ ! -c /dev/net/tun ]] && mknod /dev/net/tun c 10 200

  ip link add br0 type bridge 2>/dev/null || true # Ignore if exists
  ip link set dev br0 up
  ip link set dev br0 promisc on

  echo 'Setting up TAP tap0...'
  ip tuntap add dev tap0 mode tap 2>/dev/null || true
  ip link set dev tap0 master br0
  ip link set dev tap0 up

  # Disable offloads
  ethtool -K tap0 tx off || echo "Warning: Failed to disable TX checksum offloads"
  ip link set dev br0 type bridge mcast_snooping 0 || echo "Warning: Failed to disable mcast_snooping"

  echo "Starting tcpdump..."
  # tcpdump -n -i br0 -U -w "${LOG_DIR}/tcpdump.pcap" > "${LOG_DIR}/tcpdump.log" 2>&1 &
}

#######################################
# Generate the client disk image.
# Globals:
#   INSTALL_DISK
#   CLIENT_DISK_IMG
#   UKI_FILE
#   DISK_IMG_SIZE
# Arguments:
#   None
#######################################
generate_disk() {
  if [[ "${INSTALL_DISK:-0}" -eq 1 ]]; then
    echo "Generating Client Disk Image (${CLIENT_DISK_IMG}) with ESP..."

    if [[ "${PRESERVE_DISK:-0}" -eq 1 && -f "${CLIENT_DISK_IMG}" ]]; then
        echo "Preserving existing disk ${CLIENT_DISK_IMG}..."
        return
    fi

    local -r esp_part_image="esp_part.img"
    local -r grub_config_file="grub.cfg"
    local -r uki_boot_name="linux.efi"

    if [[ ! -f "${UKI_FILE}" ]]; then
      echo "Error: UKI File ${UKI_FILE} not found inside container."
      exit 1
    fi

    local uki_size_bytes
    uki_size_bytes=$(stat -c%s "${UKI_FILE}")
    local uki_size_mb=$(( (uki_size_bytes + 1024 * 1024 - 1) / (1024 * 1024) ))
    local esp_size_mb=$((uki_size_mb * 2 + 128))
    local esp_blocks=$((esp_size_mb * 1024))

    echo "Creating ESP (${esp_size_mb}MB)..."
    rm -f "${esp_part_image}"
    mkfs.vfat -C "${esp_part_image}" -F 32 -n "ESP" "${esp_blocks}"
    mmd -i "${esp_part_image}" ::/EFI ::/EFI/BOOT ::/boot ::/boot/grub

    # Locate Grub
    local platform
    platform=$([[ -f .platform ]] && cat .platform || echo vs)
    local grub_efi_file=""

    # Force Direct Boot (Bypass GRUB) for testing stability
    grub_efi_file=""


    if [[ -n "${grub_efi_file}" ]]; then
      echo "Using Grub: ${grub_efi_file}"
      mcopy -i "${esp_part_image}" "${grub_efi_file}" ::/EFI/BOOT/BOOTX64.EFI
    else
      echo "Direct Boot: BOOTX64.EFI will be populated via loop mount to avoid mcopy OOM."
    fi

    # Debug mcopy
    mcopy -V

    # Test small copy
    echo "test" > test_small.txt
    mcopy -i "${esp_part_image}" test_small.txt "::/test_small.txt"
    echo "Small mcopy succeeded."

    # Copy UKI (large file)
    echo "Copying UKI (${uki_size_mb} MB)..."

    # Use loop mount to avoid OOM with mcopy on large files
    mkdir -p /mnt/esp_tmp
    mount -o loop "${esp_part_image}" /mnt/esp_tmp

    # Use dd with controlled block size and sync to avoid memory spikes
    # Copy to linux.efi
    # Use dd with bs=4M and no sync flag to avoid OOM
    dd if="${UKI_FILE}" of="/mnt/esp_tmp/EFI/BOOT/${uki_boot_name}" bs=4M status=progress
    sync

    # If direct boot, also copy to BOOTX64.EFI
    if [[ -z "${grub_efi_file}" ]]; then
        echo "Populating BOOTX64.EFI from UKI..."
        cp "/mnt/esp_tmp/EFI/BOOT/${uki_boot_name}" "/mnt/esp_tmp/EFI/BOOT/BOOTX64.EFI"
    fi

    umount /mnt/esp_tmp
    rmdir /mnt/esp_tmp
    echo "UKI copy succeeded."

    if [[ -n "${grub_efi_file}" ]]; then
      # Create Grub Config
      # Use minimal config to avoid serial/terminal issues
      printf "set timeout=2\nset default=0\nmenuentry \"Boot UKI\" {\n    chainloader /EFI/BOOT/%s\n}\n" "${uki_boot_name}" > "${grub_config_file}"
      # Copy config to multiple locations for signed/shim loaded GRUBs
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/EFI/BOOT/grub.cfg
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/boot/grub/grub.cfg

      # Backup locations for Debian/SONiC signed GRUB
      mmd -i "${esp_part_image}" ::/EFI/debian ::/EFI/SONiC
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/EFI/debian/grub.cfg
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/EFI/SONiC/grub.cfg
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/grub.cfg
      # Support custom prefixes (/grub or /EFI/SONIE)
      mmd -i "${esp_part_image}" ::/grub ::/EFI/SONIE-OS || true
      mcopy -i "${esp_part_image}" "${grub_config_file}" ::/grub/grub.cfg
      # Keep grub_config_file for XBOOTLDR

      # Copy GRUB modules to ESP (critical for non-monolithic signed images)
      local modules_src="fsroot-${platform}-recovery/usr/lib/grub/x86_64-efi"
      if [[ -d "${modules_src}" ]]; then
          echo "Copying GRUB modules from ${modules_src}..."

          # Target 1: /boot/grub/x86_64-efi
          mmd -i "${esp_part_image}" ::/boot/grub/x86_64-efi || true
          mcopy -s -i "${esp_part_image}" "${modules_src}" ::/boot/grub/

          # Target 2: /EFI/BOOT/x86_64-efi
          mmd -i "${esp_part_image}" ::/EFI/BOOT/x86_64-efi || true
          mcopy -s -i "${esp_part_image}" "${modules_src}" ::/EFI/BOOT/

          # Target 3: /EFI/debian/x86_64-efi
          mmd -i "${esp_part_image}" ::/EFI/debian/x86_64-efi || true
          mcopy -s -i "${esp_part_image}" "${modules_src}" ::/EFI/debian/

          # Target 4: /EFI/SONiC/x86_64-efi
          mmd -i "${esp_part_image}" ::/EFI/SONiC/x86_64-efi || true
          mcopy -s -i "${esp_part_image}" "${modules_src}" ::/EFI/SONiC/
      else
          echo "Warning: GRUB modules not found at ${modules_src}"
      fi
    fi

    # Create final disk image
    rm -f "${CLIENT_DISK_IMG}"
    truncate -s "${DISK_IMG_SIZE}" "${CLIENT_DISK_IMG}"
    dd if="${esp_part_image}" of="${CLIENT_DISK_IMG}" bs=1M seek=1 conv=notrunc status=none

    sgdisk --version
    echo "Filesystem:"
    ls -l "${CLIENT_DISK_IMG}"

    # Try just printing first
    sgdisk -p "${CLIENT_DISK_IMG}" 2>&1 || true

    # Create new GPT table (zap existing if any)
    echo "Creating new GPT..."
    sgdisk -Z "${CLIENT_DISK_IMG}" 2>&1 || true
    sgdisk -o "${CLIENT_DISK_IMG}" 2>&1 || true

    # Create partition 1 (ESP)
    # Align to 2048 sectors (1MB), matching our dd seek=1
    echo "Creating Partition 1 (ESP)..."
    sgdisk -n "1:2048:+${esp_size_mb}M" -t 1:ef00 "${CLIENT_DISK_IMG}" 2>&1 || true

    sgdisk -p "${CLIENT_DISK_IMG}" 2>&1 || true

    # Create partition 2 (XBOOTLDR)
    echo "Creating Partition 2 (XBOOTLDR)..."
    local xbootldr_part_image="xbootldr_part.img"
    local xbootldr_size_mb=256
    local xbootldr_blocks=$((xbootldr_size_mb * 1024))

    rm -f "${xbootldr_part_image}"
    # Label filesystem as XBOOTLDR for detection
    mkfs.vfat -C "${xbootldr_part_image}" -F 32 -n "XBOOTLDR" "${xbootldr_blocks}"

    # Populate XBOOTLDR with grub config if available
    if [[ -n "${grub_efi_file}" ]]; then
        # Create standard grub locations
        # rc.local looks in /boot/grub, /boot/boot/grub, or root /grub.cfg
        # We'll put it in /grub/grub.cfg and /boot/grub/grub.cfg to be safe
        mmd -i "${xbootldr_part_image}" ::/grub ::/boot ::/boot/grub || true
        mcopy -i "${xbootldr_part_image}" "${grub_config_file}" ::/grub/grub.cfg
        mcopy -i "${xbootldr_part_image}" "${grub_config_file}" ::/boot/grub/grub.cfg
        mcopy -i "${xbootldr_part_image}" "${grub_config_file}" ::/grub.cfg
    fi

    # DD XBOOTLDR partition into place
    # partition 1 ends at 1 + esp_size_mb
    local p2_offset_mb=$((1 + esp_size_mb))
    echo "Writing XBOOTLDR to disk at offset ${p2_offset_mb}MB..."
    dd if="${xbootldr_part_image}" of="${CLIENT_DISK_IMG}" bs=1M seek="${p2_offset_mb}" conv=notrunc status=none

    rm "${xbootldr_part_image}"

    # Cleanup grub config if it exists
    if [[ -f "${grub_config_file}" ]]; then
        rm "${grub_config_file}"
    fi

    # Partition 2 definition
    # 2048 (1MB) + esp_size_mb * 2048sectors/mb = end_sector_of_p1
    # start_sector_of_p2 = end_sector_of_p1
    # We used bs=1M for dd, so pure MB math works for start.
    # sgdisk takes start sector. 1MB = 2048 sectors.
    local p2_start_sector=$((p2_offset_mb * 2048))

    sgdisk -n "2:${p2_start_sector}:+${xbootldr_size_mb}M" -t 2:ea00 -c 2:"XBOOTLDR" "${CLIENT_DISK_IMG}" 2>&1 || true

    sgdisk -p "${CLIENT_DISK_IMG}" 2>&1 || true

    rm "${esp_part_image}"
  else
    echo "Generating Blank Client Disk Image (${CLIENT_DISK_IMG})..."

    if [[ "${PRESERVE_DISK:-0}" -eq 1 && -f "${CLIENT_DISK_IMG}" ]]; then
        echo "Preserving existing disk ${CLIENT_DISK_IMG}..."
        return
    fi

    rm -f "${CLIENT_DISK_IMG}"
    truncate -s "${DISK_IMG_SIZE}" "${CLIENT_DISK_IMG}"
  fi
}

#######################################
# Configure DNSMasq and HTTP services.
# Globals:
#   USE_IPV6
#   HTTP_PORT
#   TEMPLATE_DIR
#   DNSMASQ_CONF
#   BOOT_IPXE_FILE
# Arguments:
#   None
#######################################
configure_services() {
  # Export variables for envsubst
  # Explicitly export ALL variables needed by templates
  export BRIDGE_IP6 CLIENT_IP6_START CLIENT_IP6_END CLIENT_MAC HTTP_PORT IMAGESET_FILE UKI_FILE
  export BRIDGE_IP CLIENT_IP_START CLIENT_IP_END
  export UKI_FILENAME="$(basename "${UKI_FILE}")"
  # Calculated variables
  export DNSMASQ_CONFIG_BUNDLE_LINE=""
  export HTTP_ADDR=""

  local server_host=""

  if [[ "${USE_IPV6}" -eq 1 ]]; then
    echo 'Configuring IPv6...'
    ip addr add "${BRIDGE_IP6}/64" dev br0 nodad
    ip addr add "${BRIDGE_IP}/24" dev br0

    echo "${BRIDGE_IP6} server.boot" >> /etc/hosts

    if [[ -n "${CONFIG_BUNDLE:-}" ]]; then
      export DNSMASQ_CONFIG_BUNDLE_LINE="dhcp-option=tag:onie_client6,option6:60,http://[${BRIDGE_IP6}]:${HTTP_PORT}/${CONFIG_BUNDLE}"
    fi


    export HTTP_ADDR="[${BRIDGE_IP6}]"
    envsubst < "${TEMPLATE_DIR}/dnsmasq.ipv6.conf.template" > "${DNSMASQ_CONF}"

    server_host="${BRIDGE_IP6}"
  else
    echo 'Configuring IPv4...'
    ip addr add "${BRIDGE_IP}/24" dev br0

    export HTTP_ADDR="${BRIDGE_IP}"
    envsubst < "${TEMPLATE_DIR}/dnsmasq.ipv4.conf.template" > "${DNSMASQ_CONF}"

    server_host="${BRIDGE_IP}"
  fi

  if [[ "${BOOTZ_ENABLED:-0}" -eq 1 ]]; then
      # option 60 (vendor class) for Bootz is "OpenConfig-Bootz"
      echo "dhcp-vendorclass=set:bootz_client,OpenConfig-Bootz" >> "${DNSMASQ_CONF}"
  fi

  echo "Starting dnsmasq..."
  echo "============"
  cat "${DNSMASQ_CONF}"
  echo "============"
  dnsmasq -C "${DNSMASQ_CONF}" --log-dhcp --log-queries --enable-tftp --tftp-root=/data -d > "${LOG_DIR}/dnsmasq.log" 2>&1 &
  DNSMASQ_PID=$!

  echo "Starting python server..."
  # Ensure variables passed to python script are correct
  # Bind to all interfaces ("") to avoid race conditions or binding issues
  local bind_host=""
  if [[ "${USE_IPV6}" -eq 1 ]]; then
      bind_host="::"
  fi
  PYTHONUNBUFFERED=1 python3 "${SCRIPT_DIR}/http_server_progress.py" "${HTTP_PORT}" "${bind_host}" "/data" > "${LOG_DIR}/http_server.log" 2>&1 &
  HTTP_PID=$!
  echo "HTTP Server PID: ${HTTP_PID}"

  # Wait for server
  echo "Waiting for Python server to bind to port ${HTTP_PORT}..."
  local started=0
  set +e
  for i in {1..10}; do
    if ss -tulpn | grep -q ":${HTTP_PORT}"; then
      echo "Server is listening on port ${HTTP_PORT}"
      started=1
      break
    fi
    echo "Wait ${i}/10. ss output:"
    ss -tulpn || true
    ps aux | grep python || true
    sleep 1
  done
  set -e

  if [[ "${started}" -eq 0 ]]; then
    echo "ERROR: Python server failed to start."
    if kill -0 "${HTTP_PID}" 2>/dev/null; then
      echo "Process is running but not listening?"
    else
      echo "Process exited."
    fi
    cat /data/http_server.log
    exit 1
  fi

  if [[ "${BOOTZ_ENABLED:-0}" -eq 1 ]]; then
      echo "Starting Bootz Server..."
      # Bootz server might need flags. Assuming defaults or env vars for now.
      # It typically listens on a port (e.g., 50051 for gRPC).
      # We mounted it at /usr/local/bin/bootz_server
      # Calculate hashes
      local sonie_hash
      sonie_hash=$(sha256sum /data/sonie.efi | awk '{print $1}')
      local sonic_hash
      sonic_hash=$(sha256sum /data/sonic-vs.bin | awk '{print $1}')

      # Create dummy inventory for Bootz
      cat <<EOF > /data/inventory.prototxt
options {
    bootzserver: "[${server_host}]:50051"
}
chassis {
    name: "test_device"
    serial_number: "123456789"
    manufacturer: "SONiC"
    bootloader_password_hash: "none"
    controller_cards {
        serial_number: "CardA"
        part_number: "PartA"
    }
    software_image {
        name: "sonie"
        version: "1.0"
        url: "http://[${server_host}]:${HTTP_PORT}/sonie.efi"
        os_image_hash: "${sonie_hash}"
        hash_algorithm: "SHA256"
    }

    boot_mode: BOOT_MODE_INSECURE
}
EOF

      if [[ -x /usr/local/bin/bootz_server ]]; then
          /usr/local/bin/bootz_server \
            --bootz_addr=:50051 \
            --inv_config=/data/inventory.prototxt \
            > "${LOG_DIR}/bootz_server.log" 2>&1 &
          BOOTZ_PID=$!
          echo "Bootz Server PID: ${BOOTZ_PID}"
      else
          echo "Error: Bootz server binary not found or executable at /usr/local/bin/bootz_server"
      fi
  fi
}

#######################################
# Setup software TPM.
# Globals:
#   TPM_DIR
#   SWTPM_PID
# Arguments:
#   None
#######################################
setup_tpm() {
  echo "Setting up TPM..."
  TPM_DIR=$(mktemp -d)
  mkdir -p "${TPM_DIR}"

  # Initialize TPM state
  # Use --tpm2 for TPM 2.0
  swtpm_setup --tpmstate "${TPM_DIR}" --create-ek-cert --create-platform-cert --tpm2

  # Start swtpm socket
  # Using unix socket for QEMU connection
  swtpm socket --tpmstate dir="${TPM_DIR}" \
    --ctrl type=unixio,path="${TPM_DIR}/swtpm-sock" \
    --tpm2 \
    --log level=10,file="${LOG_DIR}/swtpm.log" &
  SWTPM_PID=$!
  echo "SWTPM PID: ${SWTPM_PID}"

  # Wait for socket
  local i
  for i in {1..10}; do
    if [[ -S "${TPM_DIR}/swtpm-sock" ]]; then
      echo "TPM socket created."
      return
    fi
    sleep 0.5
  done
  echo "Error: TPM socket not found."
  exit 1
}

#######################################
# Launch QEMU with configured parameters.
# Globals:
#   CLIENT_MAC
#   CLIENT_DISK_IMG
#   CLIENT_OVMF_VARS
# Arguments:
#   None
#######################################
launch_qemu() {
  local cpu_opts=()
  if [[ -w /dev/kvm ]]; then
    echo "KVM detected."
    cpu_opts=(-cpu host)
  else
    echo "Warning: /dev/kvm not writeable or present. Using TCG."
    cpu_opts=(-cpu qemu64 -machine q35)
  fi

  echo 'Launching QEMU connected to tap0...'
  local qemu_args=(
    -nographic -serial mon:stdio -m "${QEMU_MEMORY:-8G}" "${cpu_opts[@]}"
    -machine q35,smm=on,accel=kvm -global driver=cfi.pflash01,property=secure,value=$([[ "${SECURE_BOOT}" -eq 1 ]] && echo on || echo off)
    -drive if=pflash,format=raw,readonly=on,file=/ovmf_code.fd
    -drive if=pflash,format=raw,file="${CLIENT_OVMF_VARS}"
    -drive file="${CLIENT_DISK_IMG}",format=raw
    -netdev tap,id=net0,ifname=tap0,script=no,downscript=no
    -device "virtio-net-pci,netdev=net0,mac=${CLIENT_MAC},csum=off"
    -chardev "socket,id=chrtpm,path=${TPM_DIR}/swtpm-sock"
    -tpmdev "emulator,id=tpm0,chardev=chrtpm"
    -device "tpm-tis,tpmdev=tpm0"
    -k en-us
    -debugcon file:/data/ovmf_debug.log -global isa-debugcon.iobase=0x402
  )

  qemu-system-x86_64 "${qemu_args[@]}"
}

#######################################
# Enroll Secure Boot keys into OVMF variables.
# Globals:
#   CLIENT_OVMF_VARS
#   PK_AUTH
#   KEK_AUTH
#   DB_AUTH
# Arguments:
#   None
#######################################
enroll_secure_boot_keys() {
  local vars_file="${CLIENT_OVMF_VARS}"
  local guid="55555555-5555-5555-5555-555555555555"

  if [[ ! -f "${vars_file}" ]]; then
      echo "Error: OVMF vars file ${vars_file} not found for enrollment."
      return
  fi

  if [[ "${SECURE_BOOT}" -ne 1 ]]; then
      echo "Secure Boot disabled. Skipping key enrollment."
      return
  fi

  # Check if we have cert files
  local found_keys=0
  for cert in "${PK_CERT}" "${KEK_CERT}" "${DB_CERT}"; do
      if [[ -n "${cert}" && -f "${cert}" ]]; then
          found_keys=1
      fi
  done

  if [[ "${found_keys}" -eq 0 ]]; then
      echo "No Secure Boot keys (certs) found. Skipping enrollment."
      return
  fi

  echo "Enrolling Secure Boot keys into ${vars_file} using virt-fw-vars..."

  # Build command logic
  # We should preferably do it in one pass or multiple passes.
  # virt-fw-vars supports multiple arguments.

  local cmd=(virt-fw-vars --inplace "${vars_file}")

  if [[ -n "${DB_CERT}" && -f "${DB_CERT}" ]]; then
      echo "Enrolling db from ${DB_CERT}..."
      cmd+=(--add-db "${guid}" "${DB_CERT}")
  fi

  if [[ -n "${KEK_CERT}" && -f "${KEK_CERT}" ]]; then
      echo "Enrolling KEK from ${KEK_CERT}..."
      cmd+=(--add-kek "${guid}" "${KEK_CERT}")
  fi

  if [[ -n "${PK_CERT}" && -f "${PK_CERT}" ]]; then
      echo "Enrolling PK from ${PK_CERT} (Enabling Secure Boot)..."
      cmd+=(--set-pk "${guid}" "${PK_CERT}")
      # --secure-boot flag might be redundant if PK is set, but let's add it to be sure
      cmd+=(--secure-boot)
  fi

  "${cmd[@]}"
}

#######################################
# Configure Boot Order to prioritize IPv6.
# Globals:
#   USE_IPV6
#   CLIENT_OVMF_VARS
#   BRIDGE_IP6
#   HTTP_PORT
#   UKI_FILE
# Arguments:
#   None
#######################################
configure_boot() {
  if [[ "${USE_IPV6}" -eq 1 && "${INSTALL_DISK:-0}" -ne 1 ]]; then
    # Prioritize IPv6 HTTP Boot using BootNext
    # This avoids waiting for PXEv4 timeout
    echo "Configuring BootNext for IPv6 HTTP Boot..."
    local vars_file="${CLIENT_OVMF_VARS}"
    # Note: URI must match the one served by dnsmasq/http server
    # qemu_runner.sh sets up bridge IP, and http server on port 8000
    local boot_uri="http://[${BRIDGE_IP6}]:${HTTP_PORT}/$(basename "${UKI_FILE}")"

    echo "Setting BootNext to ${boot_uri}..."
    virt-fw-vars --inplace "${vars_file}" --set-boot-uri "${boot_uri}" || echo "Warning: Failed to set BootNext"
  fi
}

#######################################
# Main execution function.
# Arguments:
#   None
#######################################
main() {
  check_dependencies
  setup_network
  generate_disk
  configure_services
  setup_tpm
  enroll_secure_boot_keys
  configure_boot
  launch_qemu
}

# Execute main
main "$@"

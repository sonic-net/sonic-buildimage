#!/bin/bash
#
# QEMU Runner Script for SONiC
#
# This script sets up the network environment (bridge, tap), configures
# services (dnsmasq, python HTTP server) via templates, generates a client
# disk image, and launches QEMU.
#

set -e
set -o pipefail

# --- Defaults ---
: "${secure_boot:=0}"
exec > /data/qemu_runner.log 2>&1
echo "Starting qemu_runner.sh..."
# Usage of "readonly" where appropriate for constants
readonly DISK_IMG_SIZE="${DISK_IMG_SIZE:-128G}"
readonly HTTP_PORT="${HTTP_PORT:-8000}"
readonly CLIENT_DISK_IMG="${CLIENT_DISK_IMG:-client_disk.img}"
readonly CLIENT_MAC="${CLIENT_MAC:-52:54:00:12:34:56}"
readonly CLIENT_OVMF_VARS="${CLIENT_OVMF_VARS:-OVMF_VARS.fd}"
readonly UKI_FILE="${UKI_FILE:-sonie.efi}"
readonly IMAGESET_FILE="${IMAGESET_FILE:-imageset.json}"

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

  echo 'Setting up TAP tap0...'
  ip tuntap add dev tap0 mode tap 2>/dev/null || true
  ip link set dev tap0 master br0
  ip link set dev tap0 up

  # Disable offloads
  ethtool -K tap0 tx off || echo "Warning: Failed to disable TX checksum offloads"
  ip link set dev br0 type bridge mcast_snooping 0 || echo "Warning: Failed to disable mcast_snooping"

  echo "Starting tcpdump..."
  tcpdump -n -i br0 -U -w "${LOG_DIR}/tcpdump.pcap" > "${LOG_DIR}/tcpdump.log" 2>&1 &
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
    echo "Generating Client Disk Image (${CLIENT_DISK_IMG}) as Superfloppy ESP..."

    local uki_size_bytes
    uki_size_bytes=$(stat -c%s "${UKI_FILE}")
    local uki_size_mb=$(( (uki_size_bytes + 1024 * 1024 - 1) / (1024 * 1024) ))
    local esp_size_mb=$((uki_size_mb + 64))
    local esp_blocks=$((esp_size_mb * 1024))
    local grub_config_file="grub.cfg"
    local uki_boot_name="linux.efi"

    # Create the disk image as a raw ESP filesystem (Superfloppy)
    rm -f "${CLIENT_DISK_IMG}"
    # Use calculated blocks. -C creates the file.
    mkfs.vfat -C "${CLIENT_DISK_IMG}" -F 32 -n "ESP" "${esp_blocks}"
    
    mmd -i "${CLIENT_DISK_IMG}" ::/EFI ::/EFI/BOOT ::/boot ::/boot/grub

    # Locate Grub
    local platform
    platform=$([[ -f .platform ]] && cat .platform || echo vs)
    local grub_efi_file=""
    local shim_efi_file=""

    # Check for Shim+Signed Grub
    if [[ -f "/usr/lib/shim/shimx64.efi.signed" && -f "/usr/lib/grub/x86_64-efi-signed/grubx64.efi.signed" ]]; then
      shim_efi_file="/usr/lib/shim/shimx64.efi.signed"
      grub_efi_file="/usr/lib/grub/x86_64-efi-signed/grubx64.efi.signed"
      echo "Using Shim: ${shim_efi_file} + Grub: ${grub_efi_file}"
    elif [[ -f "/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi" ]]; then
      grub_efi_file="/usr/lib/grub/x86_64-efi/monolithic/grubx64.efi"
      echo "Using Monolithic Grub: ${grub_efi_file}"
    else
      echo "Warning: Grub EFI not found. Will copy UKI as BOOTX64.EFI for Direct Boot."
    fi

    if [[ -n "${shim_efi_file}" ]]; then
        mcopy -i "${CLIENT_DISK_IMG}" "${shim_efi_file}" ::/EFI/BOOT/BOOTX64.EFI
        mcopy -i "${CLIENT_DISK_IMG}" "${grub_efi_file}" ::/EFI/BOOT/grubx64.efi
    elif [[ -n "${grub_efi_file}" ]]; then
        mcopy -i "${CLIENT_DISK_IMG}" "${grub_efi_file}" ::/EFI/BOOT/BOOTX64.EFI
    else
        mcopy -i "${CLIENT_DISK_IMG}" "${UKI_FILE}" ::/EFI/BOOT/BOOTX64.EFI
    fi

    mcopy -i "${CLIENT_DISK_IMG}" "${UKI_FILE}" "::/EFI/BOOT/${uki_boot_name}"
    
    # startup.nsh fallback
    echo "fs0:\\EFI\\BOOT\\BOOTX64.EFI" > startup.nsh
    mcopy -i "${CLIENT_DISK_IMG}" startup.nsh ::/startup.nsh
    rm startup.nsh

    if [[ -n "${grub_efi_file}" ]]; then
      # Grub Config
      # Use chainloader. Arguments are passed via SystemdOptions EFI variable injected separately.
      # Sidecar files proved unreliable with chainloader in this environment.
      printf "console=ttyS0,115200n8\nset timeout=2\nset default=0\nmenuentry \"Boot UKI\" {\n    chainloader /EFI/BOOT/%s\n}\n" "${uki_boot_name}" > "${grub_config_file}"
      mcopy -i "${CLIENT_DISK_IMG}" "${grub_config_file}" ::/EFI/BOOT/grub.cfg
      mcopy -i "${CLIENT_DISK_IMG}" "${grub_config_file}" ::/boot/grub/grub.cfg
      rm "${grub_config_file}"
    fi

  else
    echo "Generating Blank Client Disk Image (${CLIENT_DISK_IMG})..."
    rm -f "${CLIENT_DISK_IMG}"
    truncate -s "${DISK_IMG_SIZE}" "${CLIENT_DISK_IMG}"
  fi
}

inject_uefi_vars() {
  echo "Injecting SystemdOptions into OVMF Variables..."
  local options="overlay_tmpfs=on console=ttyS0,115200n8"
  # Python to encode UTF-16LE hex
  local hex_data
  hex_data=$(python3 -c "print('${options}'.encode('utf-16le').hex())")

  cat > systemd_options.json <<EOF
{
    "version": 2,
    "variables": [
        {
            "name": "SystemdOptions",
            "guid": "4a67b082-0a4c-41cf-b6c7-440b29bb8c4f",
            "attr": 7,
            "data": "${hex_data}"
        }
    ]
}
EOF
  
  if [[ -f "${CLIENT_OVMF_VARS}" ]]; then
      virt-fw-vars -i "${CLIENT_OVMF_VARS}" --set-json systemd_options.json -o "${CLIENT_OVMF_VARS}"
  else
      echo "Warning: ${CLIENT_OVMF_VARS} not found. Skipping variable injection."
  fi
  rm systemd_options.json
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
  # Calculated variables
  export DNSMASQ_CONFIG_BUNDLE_LINE=""
  export HTTP_ADDR=""

  local server_host=""

  if [[ "${USE_IPV6}" -eq 1 ]]; then
    echo 'Configuring IPv6...'
    ip addr add "${BRIDGE_IP6}/64" dev br0 nodad

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


  echo "Starting dnsmasq..."
  echo "============"
  cat "${DNSMASQ_CONF}"
  echo "============"
  dnsmasq -C "${DNSMASQ_CONF}" --log-dhcp --log-queries --enable-tftp --tftp-root=/data -d > "${LOG_DIR}/dnsmasq.log" 2>&1 &
  DNSMASQ_PID=$!

  echo "Starting python server..."
  # Ensure variables passed to python script are correct
  PYTHONUNBUFFERED=1 python3 "${SCRIPT_DIR}/http_server_progress.py" "${HTTP_PORT}" "${server_host}" > "${LOG_DIR}/http_server.log" 2>&1 &
  HTTP_PID=$!
  echo "HTTP Server PID: ${HTTP_PID}"

  # Wait for server
  echo "Waiting for Python server to bind to port ${HTTP_PORT}..."
  local started=0
  for i in {1..10}; do
    if ss -tulpn | grep -q ":${HTTP_PORT} "; then
      echo "Server is listening on port ${HTTP_PORT}"
      started=1
      break
    fi
    sleep 1
  done

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
    -nographic -m 8G "${cpu_opts[@]}"
    -machine q35,smm=on,accel=kvm -global driver=cfi.pflash01,property=secure,value=on
    -drive if=pflash,format=raw,readonly=on,file=/ovmf_code.fd
    -drive if=pflash,format=raw,file="${CLIENT_OVMF_VARS}"
    -device virtio-net-pci,netdev=n1,mac="${CLIENT_MAC}"
    -netdev tap,id=n1,ifname=tap0,script=no,downscript=no
    -object rng-random,filename=/dev/urandom,id=rng0
    -device virtio-rng-pci,rng=rng0
    -device virtio-blk-pci,drive=d0
    -drive file="${CLIENT_DISK_IMG}",if=none,id=d0,format=raw
    -chardev "socket,id=chrtpm,path=${TPM_DIR}/swtpm-sock"
    -tpmdev "emulator,id=tpm0,chardev=chrtpm"
    -device "tpm-tis,tpmdev=tpm0"
    -debugcon file:/data/ovmf_debug.log -global isa-debugcon.iobase=0x402
    # -boot n
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
  if [[ "${USE_IPV6}" -eq 1 ]]; then
    # Prioritize IPv6 HTTP Boot using BootNext
    # This avoids waiting for PXEv4 timeout
    echo "Configuring BootNext for IPv6 HTTP Boot..."
    local vars_file="${CLIENT_OVMF_VARS}"
    # Note: URI must match the one served by dnsmasq/http server
    # qemu_runner.sh sets up bridge IP, and http server on port 8000
    local boot_uri="http://[${BRIDGE_IP6}]:${HTTP_PORT}/${UKI_FILE}"
    
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
  inject_uefi_vars
  # configure_boot
  launch_qemu
}

# Execute main
main "$@"

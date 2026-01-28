#!/bin/bash
#
# onie-discovery.sh - Emulate ONIE discovery behavior
#
# Implements logic similar to onie-upstream:
# - discover.sh
# - exec_installer
# - functions

readonly LOGFILE="/var/log/onie-discovery.log"
readonly INSTALLER_PATH="/tmp/onie-installer"

#######################################
# Log a message with a timestamp.
# Globals:
#   LOGFILE
# Arguments:
#   $1: Message to log
# Returns:
#   None
#######################################
log() {
  echo "$(date) - $1" | tee -a "$LOGFILE"
}

#######################################
# Import ONIE variables from /proc/cmdline.
# Globals:
#   ONIE_PLATFORM
#   ONIE_MACHINE
#   ONIE_ARCH
#   ONIE_VENDOR_ID
#   ONIE_INSTALL_URL
#   ONIE_BOOT_REASON
# Arguments:
#   None
# Returns:
#   None
#######################################
import_cmdline() {
  local x
  for x in $(cat /proc/cmdline); do
    case "$x" in
      onie_platform=*)
        ONIE_PLATFORM="${x#*=}"
        ;;
      onie_machine=*)
        ONIE_MACHINE="${x#*=}"
        ;;
      onie_arch=*)
        ONIE_ARCH="${x#*=}"
        ;;
      onie_vendor_id=*)
        ONIE_VENDOR_ID="${x#*=}"
        ;;
      onie_install_url=*)
        ONIE_INSTALL_URL="${x#*=}"
        ;;
      boot_reason=*)
        ONIE_BOOT_REASON="${x#*=}"
        ;;
    esac
  done

  # Fallback to defaults or machine.conf if necessary
  if [[ -f /host/machine.conf ]]; then
    # shellcheck source=/dev/null
    . /host/machine.conf
    [[ -z "$ONIE_PLATFORM" ]] && ONIE_PLATFORM="$onie_platform"
    [[ -z "$ONIE_MACHINE" ]] && ONIE_MACHINE="$onie_machine"
    [[ -z "$ONIE_ARCH" ]] && ONIE_ARCH="$onie_arch"
  fi
}

#######################################
# Generate default installer filenames.
# Globals:
#   ONIE_BOOT_REASON
#   ONIE_PLATFORM
#   ONIE_ARCH
#   ONIE_MACHINE
# Arguments:
#   None
# Returns:
#   List of filenames to stdout
#######################################
get_default_filenames() {
  local prefix="onie-installer"
  if [[ "$ONIE_BOOT_REASON" == "update" ]]; then
    prefix="onie-updater"
  fi

  # Default list based on ONIE logic
  echo "${prefix}-${ONIE_PLATFORM}"
  echo "${prefix}-${ONIE_ARCH}-${ONIE_MACHINE}"
  echo "${prefix}-${ONIE_MACHINE}"
  echo "${prefix}-${ONIE_ARCH}"
  echo "${prefix}"

  # Add .bin suffixes
  echo "${prefix}-${ONIE_PLATFORM}.bin"
  echo "${prefix}-${ONIE_ARCH}-${ONIE_MACHINE}.bin"
  echo "${prefix}-${ONIE_MACHINE}.bin"
  echo "${prefix}-${ONIE_ARCH}.bin"
  echo "${prefix}.bin"
}

#######################################
# Run an installer from a previously downloaded path.
# Globals:
#   INSTALLER_PATH
# Arguments:
#   None
# Returns:
#   None (Exits on success)
#######################################
run_installer() {
  log "Running installer..."
  chmod +x "$INSTALLER_PATH"
  "$INSTALLER_PATH"
  local ret=$?
  log "Installer finished with code $ret"
  if [[ $ret -eq 0 ]]; then
    log "Installer reported success. Rebooting..."
    reboot
    exit 0
  fi
}

#######################################
# Download and try to run an installer from a URL.
# Globals:
#   INSTALLER_PATH
# Arguments:
#   $1: URL to download
# Returns:
#   0 if successful (should theoretically exit before returning), 1 otherwise
#######################################
url_run() {
  local url="$1"
  log "Attempting URL: $url"

  # Clean up previous attempts
  rm -f "$INSTALLER_PATH"

  local ret=1
  if [[ "$url" == http* ]] || [[ "$url" == ftp* ]]; then
    if command -v wget >/dev/null; then
      wget -q -O "$INSTALLER_PATH" "$url"
      ret=$?
    elif command -v curl >/dev/null; then
      curl -s -o "$INSTALLER_PATH" "$url"
      ret=$?
    fi
  elif [[ "$url" == tftp* ]]; then
    # Try curl for tftp if available
    if command -v curl >/dev/null; then
      curl -s -o "$INSTALLER_PATH" "$url"
      ret=$?
    else
      # Parse tftp url: tftp://server/path
      local no_proto="${url#tftp://}"
      local server="${no_proto%%/*}"
      local path="${no_proto#*/}"
      # Busybox tftp
      if command -v tftp >/dev/null; then
        tftp -g -r "$path" -l "$INSTALLER_PATH" "$server"
        ret=$?
      fi
    fi
  elif [[ "$url" == file* ]]; then
    local path="${url#file://}"
    cp "$path" "$INSTALLER_PATH"
    ret=$?
  elif [[ -f "$url" ]]; then
    cp "$url" "$INSTALLER_PATH"
    ret=$?
  fi

  if [[ $ret -eq 0 ]] && [[ -f "$INSTALLER_PATH" ]]; then
    log "Download successful: $INSTALLER_PATH"
    run_installer
    return 0
  fi

  return 1
}

#######################################
# Discover installer via static URL from cmdline.
# Globals:
#   ONIE_INSTALL_URL
# Arguments:
#   None
# Returns:
#   0 if found, 1 otherwise
#######################################
discover_static() {
  if [[ -n "$ONIE_INSTALL_URL" ]]; then
    url_run "$ONIE_INSTALL_URL" && return 0
  fi
  return 1
}

#######################################
# Discover installer on local filesystems.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   0 if found, 1 otherwise
#######################################
discover_local() {
  # Scan local mounts (simple version)
  local m f
  for m in $(mount | grep "^/dev" | awk '{print $3}'); do
    for f in $(get_default_filenames); do
      if [[ -f "$m/$f" ]]; then
        url_run "file://$m/$f" && return 0
      fi
    done
  done
  return 1
}

#######################################
# Convert string to hex.
# Arguments:
#   $1: Input string
# Returns:
#   Hex string to stdout
#######################################
str2hex() {
  echo -n "$1" | hexdump -ve '1/1 "%.2x"'
}

#######################################
# Convert string to length-prefixed hex (Option 77 style).
# Arguments:
#   $1: Input string
# Returns:
#   Length+Hex string to stdout
#######################################
str2lenhex() {
  local str="$1"
  printf "%02X%s" "${#str}" "$(str2hex "$str")"
}

#######################################
# Create a VIVSO sub-option (Code + Length + Data).
# Arguments:
#   $1: Option Code
#   $2: Data String
# Returns:
#   Hex string to stdout
#######################################
make_str_opt() {
  local code="$1"
  local str="$2"
  printf "%02X%02X%s" "$code" "${#str}" "$(str2hex "$str")"
}

#######################################
# Discover installer via DHCP.
# Globals:
#   ONIE_PLATFORM
#   ONIE_MACHINE
#   ONIE_ARCH
#   ONIE_MACHINE_REV
#   onie_serial_num
# Arguments:
#   None
# Returns:
#   0 if found, 1 otherwise
#######################################
discover_dhcp() {
  log "Starting DHCP discovery..."

  local lease_file="/tmp/onie-dhcp-lease.sh"
  rm -f "$lease_file"

  local onie_iana_enterprise=42623
  # Default to onie_os if not set
  local onie_dhcp_user_class="${onie_dhcp_user_class:-onie_os}"

  # Client ID: 00 + serial (Option 61)
  local client_id_hex="00$(str2hex "${onie_serial_num:-0}")"

  # User Class: Option 77
  local user_class_hex
  user_class_hex="$(str2lenhex "$onie_dhcp_user_class")"

  # VIVSO: Option 125
  # code 3 - machine, code 4 - CPU arch, code 5 - machine revision
  local vivso_machine
  local vivso_arch
  local vivso_rev
  vivso_machine="$(make_str_opt 3 "${ONIE_MACHINE}")"
  vivso_arch="$(make_str_opt 4 "${ONIE_ARCH}")"
  vivso_rev="$(make_str_opt 5 "${onie_machine_rev:-0}")"

  local vivso_payload="${vivso_machine}${vivso_arch}${vivso_rev}"
  local vivso_len=$(( ${#vivso_payload} / 2 ))
  local vivso_hex
  vivso_hex="$(printf %08X%02X%s "$onie_iana_enterprise" "$vivso_len" "$vivso_payload")"

  local iface
  # Strategy 1: udhcpc (Busybox)
  if command -v udhcpc >/dev/null; then
    log "Using udhcpc..."
    # Request options: 1/subnet, 3/router, 6/dns, 12/hostname, 15/domain, 66/tftp, 67/bootfile, 43/vendor, 125/vivso
    local req_opts="-O 1 -O 3 -O 6 -O 12 -O 15 -O 66 -O 67 -O 43 -O 125"
    local vendor_class="onie_vendor:${ONIE_PLATFORM}"

    # We need to find an interface. For now, try all non-lo/non-dummy
    for iface in $(ip -o link show | grep -v "lo:" | awk -F': ' '{print $2}'); do
      log "Starting udhcpc on $iface..."

      udhcpc -i "$iface" \
             -s /usr/bin/onie-dhcp-event.sh \
             $req_opts \
             -V "$vendor_class" \
             -x 61:"$client_id_hex" \
             -x 77:"$user_class_hex" \
             -x 125:"$vivso_hex" \
             -q -n -t 5

      if [[ -f "$lease_file" ]]; then
        log "DHCP lease obtained on $iface"
        break
      fi
    done
  elif command -v dhclient >/dev/null; then
    # Strategy 2: dhclient
    log "Using dhclient (fallback)..."
    dhclient -1 -v
  else
    log "No DHCP client found."
    return 1
  fi

  if [[ -f "$lease_file" ]]; then
    # shellcheck source=/dev/null
    . "$lease_file"

    # Check standard bootfile
    if [[ -n "$DHCP_BOOTFILE" ]]; then
      local server="${DHCP_TFTP:-$DHCP_SERVER}"
      if [[ -z "$server" ]]; then
        # Fallback to next-server or siaddr
        server="$siaddr"
      fi

      if [[ -n "$server" ]]; then
        url_run "tftp://$server/$DHCP_BOOTFILE" && return 0
      fi
    fi

    # Check ONIE VIVSO (Option 125)
    if [[ -n "$DHCP_VIVSO" ]]; then
      log "VIVSO option found: $DHCP_VIVSO (parsing not fully implemented)"
    fi
  fi

  return 1
}

#######################################
# Try waterfall discovery on presumed DHCP server.
# Globals:
#   DHCP_TFTP
#   DHCP_SERVER
# Arguments:
#   None
# Returns:
#   0 if found, 1 otherwise
#######################################
discover_waterfall() {
  # Try generic waterfall paths on presumed servers
  if [[ -f "/tmp/onie-dhcp-lease.sh" ]]; then
    # shellcheck source=/dev/null
    . "/tmp/onie-dhcp-lease.sh"
    local server="${DHCP_TFTP:-$DHCP_SERVER}"
    if [[ -n "$server" ]]; then
      log "Attempting waterfall on DHCP server: $server"
      local f
      for f in $(get_default_filenames); do
        url_run "tftp://$server/$f" && return 0
      done
    fi
  fi
  return 1
}

#######################################
# Main execution loop.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#   None
#######################################
main() {
  log "Starting ONIE discovery service..."

  import_cmdline

  # Only run if explicitly in ONIE mode or forced (optional)
  # ONIE usually runs this loop indefinitely until success
  while true; do
    log "Starting discovery cycle..."

    # 1. Static URL
    discover_static && exit 0

    # 2. Local Filesystem
    discover_local && exit 0

    # 3. DHCP / Waterfall
    discover_dhcp && exit 0
    discover_waterfall && exit 0

    log "Discovery cycle complete. No installer found. Sleeping..."
    sleep 20
  done
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi

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
      onie_dhcp_user_class=*)
        onie_dhcp_user_class="${x#*=}"
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
    # reboot
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
    if command -v curl >/dev/null; then
      log "Running curl -v -L -o $INSTALLER_PATH $url"
      curl -v -L -o "$INSTALLER_PATH" "$url"
      ret=$?
    elif command -v wget >/dev/null; then
      log "Running wget -T 20 -O $INSTALLER_PATH $url"
      wget -T 20 -O "$INSTALLER_PATH" "$url"
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

str2lenhex16() {
  local str="$1"
  printf "%04X%s" "${#str}" "$(str2hex "$str")"
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
  # Default to SONIE if not set
  local onie_dhcp_user_class="${onie_dhcp_user_class:-SONIE}"

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
  local iface

  # Helper to run udhcpc
  try_udhcpc() {
      if ! command -v udhcpc >/dev/null; then return 1; fi
      log "Using udhcpc..."
      local req_opts="-O 1 -O 3 -O 6 -O 12 -O 15 -O 66 -O 67 -O 43 -O 125"
      local vendor_class="onie_vendor:${ONIE_PLATFORM}"

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
          log "DHCP lease obtained on $iface via udhcpc"
          return 0
        fi
      done
      return 1
  }

  # Helper to run dhclient
  try_dhclient() {
      if ! command -v dhclient >/dev/null; then return 1; fi
      log "Using dhclient (IPv4)..."

      local lease_db="/var/lib/dhcp/dhclient.leases"
      rm -f "$lease_db"
      mkdir -p /var/lib/dhcp
      
      local cf="/etc/dhcp/dhclient_onie.conf"
      mkdir -p /etc/dhcp
      # Request bootfile-name (67), tftp-server-name (66)
      echo "request subnet-mask, broadcast-address, time-offset, routers, domain-name, domain-name-servers, host-name, bootfile-name, tftp-server-name;" > "$cf"
      # Optional: Send User Class to match pxe_client if needed, but let's try default first

      dhclient -4 -1 -v -cf "$cf" -lf "$lease_db" -pf /var/run/dhclient.pid
      local ret=$?
      if [[ $ret -eq 0 ]]; then
         log "dhclient finished successfully."
         
         if [[ -f "$lease_db" ]]; then
             echo "DEBUG: IPv4 Lease File Content:" > /dev/ttyS0
             cat "$lease_db" > /dev/ttyS0
         fi

         # Check if we got an IP?
         if ip -4 addr | grep -q "global"; then
             log "Interface configured with global IP."
             
             if [[ -f "$lease_db" ]]; then
                 # Parse lease for filename (bootfile) and next-server
                 
                 local boot_file server_name next_server
                 
                 # Simple grep/awk parsing (taking last occurrence)
                 boot_file=$(grep "filename " "$lease_db" | tail -n 1 | awk -F'"' '{print $2}')
                 server_name=$(grep "option tftp-server-name" "$lease_db" | tail -n 1 | awk -F'"' '{print $2}')
                 next_server=$(grep "next-server" "$lease_db" | tail -n 1 | awk '{print $2}' | tr -d ';')
                 
                 if [[ -n "$boot_file" ]]; then
                     log "Found filename in IPv4 leases: $boot_file"
                     echo "DHCP_BOOTFILE='$boot_file'" >> "$lease_file"
                 fi
                 
                 local server="${server_name:-$next_server}"
                 if [[ -n "$server" ]]; then
                     log "Found server in IPv4 leases: $server"
                     echo "DHCP_SERVER='$server'" >> "$lease_file"
                     echo "DHCP_TFTP='$server'" >> "$lease_file"
                 fi
             fi
             return 0
         fi
      fi
      return 1
  }

  # Helper to run dhclient for IPv6
  try_dhclient6() {
      echo "DEBUG: Entered try_dhclient6" > /dev/ttyS0
      if ! command -v dhclient >/dev/null; then 
         echo "DEBUG: dhclient NOT FOUND in try_dhclient6" > /dev/ttyS0
         return 1
      fi
      log "Using dhclient (IPv6)..."

      local cf="/etc/dhcp/dhclient6_onie.conf"
      mkdir -p /etc/dhcp
# Prepare User Class Hex (RFC 8415: Len(2 bytes) + Data)
      local uc_hex_str
      uc_hex_str=$(str2lenhex16 "$onie_dhcp_user_class" | sed 's/../&:/g;s/:$//')

      cat > "$cf" <<EOF
option dhcp6.bootfile-url code 59 = string;
option dhcp6.user-class code 15 = string;
option dhcp6.name-servers code 23 = array of ip6-address;
option dhcp6.domain-search code 24 = domain-list;

request dhcp6.name-servers, dhcp6.domain-search, dhcp6.bootfile-url;
send dhcp6.user-class $uc_hex_str;
EOF
      # We use a custom lease file location to separate from OS networking
      local lease_db="/var/lib/dhcp/dhclient6.leases"
      rm -f "$lease_db"
      mkdir -p /var/lib/dhcp
      
      # Find an active interface to run dhclient on
      local iface_found=0
      for iface in $(ip -o link show | grep -v "lo:" | awk -F': ' '{print $2}'); do
          if ip link show "$iface" | grep -q "state UP"; then
              iface_found=1
              break
          fi
      done

      if [[ "$iface_found" -eq 0 ]]; then
          log "No active interface found for dhclient (IPv6)."
          return 1
      fi

      log "Starting dhclient (IPv6) on $iface with config $cf..."
      # Disable DAD to prevent self-loop collision on TAP
      sysctl -w net.ipv6.conf.${iface}.accept_dad=0 >/dev/null 2>&1 || true
      sysctl -w net.ipv6.conf.all.accept_dad=0 >/dev/null 2>&1 || true

      echo "DEBUG: Config content:"
      cat "$cf"
      if ! command -v hexdump >/dev/null; then echo "DEBUG: hexdump NOT FOUND"; else echo "DEBUG: hexdump found"; fi
      
      # Use the config file and default script (we rely on lease file for bootfile-url)
      rm -f /tmp/dhclient6_env

      # Use the config file and custom script
      dhclient -6 -1 -v -cf "$cf" -lf "$lease_db" -pf /var/run/dhclient6.pid "$iface"
      local ret=$?
      
      # DEBUG: Dump lease info
      {
          echo "DEBUG: Checking lease db: $lease_db"
          ls -l /var/lib/dhcp/
          if [[ -f "$lease_db" ]]; then cat "$lease_db"; else echo "Lease file not found"; fi
          echo "DEBUG: Checking captured env:"
          if [[ -f /tmp/dhclient6_env ]]; then cat /tmp/dhclient6_env; else echo "Env file not found"; fi
      } > /dev/ttyS0 2>&1

      if [[ $ret -eq 0 ]]; then
         log "dhclient IPv6 finished successfully."
         # Check if we got a global IPv6 address on the interface
         if ip -6 addr show dev "$iface" | grep -q "global"; then
             log "Interface configured with global IPv6 address."
             
             local boot_url=""
             
             # Check captured env first
             if [[ -f /tmp/dhclient6_env ]]; then
                 # shellcheck source=/dev/null
                 . /tmp/dhclient6_env
                 if [[ -n "$DHCP6_BOOTFILE_URL" ]]; then
                     boot_url="$DHCP6_BOOTFILE_URL"
                     log "Found bootfile-url in script env: $boot_url"
                 fi
             fi
             
             # Fallback to lease file parsing if not found in env
             if [[ -z "$boot_url" ]] && [[ -f "$lease_db" ]]; then
                 # Extract the last bootfile-url (relaxed grep)
                 boot_url=$(grep -i "bootfile-url" "$lease_db" | tail -n 1 | awk -F'"' '{print $2}')
                 if [[ -n "$boot_url" ]]; then
                      log "Found bootfile-url in leases: $boot_url"
                 fi
             fi

             if [[ -n "$boot_url" ]]; then
                 echo "DHCP_BOOTFILE='$boot_url'" > "$lease_file"
                 
                 # Extract Server from URL (handles http://[IPv6]:port/...)
                 local no_proto="${boot_url#*://}"
                 local server_port="${no_proto%%/*}"
                 local server="${server_port%:*}"
                 echo "DHCP_SERVER='$server'" >> "$lease_file"
                 echo "DHCP_TFTP='$server'" >> "$lease_file"

                 # Debug: Ping the server to verify reachability
                 # Remove brackets for ping if it's an IPv6 literal
                 local ping_server="${server#[}"
                 ping_server="${ping_server%]}"
                 log "Pinging server: $ping_server"
                 ip -6 addr > /dev/ttyS0
                 ip -6 route > /dev/ttyS0
                 ip -6 neigh > /dev/ttyS0
                 if [[ "$ping_server" == *:* ]]; then
                     ping -6 -c 3 "$ping_server" || log "Ping -6 failed"
                 else
                     ping -c 3 "$ping_server" || log "Ping failed"
                 fi
             fi

             return 0
         fi
      fi
      return 1
  }

  # Strategy 1: udhcpc (Preferred for ONIE compat)
  if try_udhcpc; then
     log "udhcpc success."
  fi
  
  # Strategy 2: dhclient IPv4 (Concurrent/Sequential)
  if try_dhclient; then
      log "dhclient (IPv4) success."
  fi
  
  # Strategy 3: dhclient IPv6 (Concurrent/Sequential)
  echo "DEBUG: Calling try_dhclient6..." > /dev/ttyS0
  if try_dhclient6; then
      log "dhclient (IPv6) success."
  fi

  # Proceed to process lease file content...


  if [[ -f "$lease_file" ]]; then
    # shellcheck source=/dev/null
    . "$lease_file"

    # Check standard bootfile
    if [[ -n "$DHCP_BOOTFILE" ]]; then
      # If BOOTFILE is a full URL, use it directly
      if [[ "$DHCP_BOOTFILE" == http* ]] || [[ "$DHCP_BOOTFILE" == ftp* ]] || [[ "$DHCP_BOOTFILE" == tftp* ]]; then
           log "Using bootfile URL from DHCP: $DHCP_BOOTFILE"
           url_run "$DHCP_BOOTFILE" && return 0
      else
          local server="${DHCP_TFTP:-$DHCP_SERVER}"
          if [[ -z "$server" ]]; then
            # Fallback to next-server or siaddr
            server="$siaddr"
          fi

          if [[ -n "$server" ]]; then
            # Assume TFTP path relative to server
            url_run "tftp://$server/$DHCP_BOOTFILE" && return 0
          fi
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
  # Force output to serial console to ensure visibility
  {
      echo "DEBUG: onie-discovery.sh STARTED"
      echo "DEBUG: IP Link Show:"
      ip link show
      
      echo "DEBUG: Bringing up interfaces..."
      for iface in $(ip -o link show | grep -v "lo:" | awk -F': ' '{print $2}'); do
          echo "DEBUG: Bringing up $iface"
          ip link set dev "$iface" up
      done
      
      echo "DEBUG: IP Link Show (After UP):"
      ip link show
      
      if command -v udhcpc >/dev/null; then echo "DEBUG: udhcpc found"; else echo "DEBUG: udhcpc NOT found"; fi
      if command -v dhclient >/dev/null; then echo "DEBUG: dhclient found"; else echo "DEBUG: dhclient NOT found"; fi

  } > /dev/ttyS0 2>&1

  log "Starting ONIE discovery service..."
  
  import_cmdline
 
  # Only run if explicitly in ONIE mode or forced (optional)
  # ONIE usually runs this loop indefinitely until success
  while true; do
    log "Starting discovery cycle..." > /dev/ttyS0

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

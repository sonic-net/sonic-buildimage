#!/bin/bash
#
# onie-dhcp-event.sh - Handle udhcpc events for ONIE discovery
#
# This script is called by udhcpc on state changes.
# We only care about 'bound' or 'renew'.

readonly OUTPUT_FILE="/tmp/onie-dhcp-lease.sh"


#######################################
# Convert dotted-decimal subnet mask to CIDR prefix length.
# Globals:
#   None
# Arguments:
#   $1: Dotted-decimal subnet mask (e.g., "255.255.255.0")
# Returns:
#   Prefix length (0-32) to stdout.
#######################################
mask_to_prefix() {
  local mask="$1"
  local -i prefix=0
  local octet
  local IFS=.
  read -r -a octets <<< "$mask"
  for octet in "${octets[@]}"; do
    case "$octet" in
      255) ((prefix+=8)) ;;
      254) ((prefix+=7)) ;;
      252) ((prefix+=6)) ;;
      248) ((prefix+=5)) ;;
      240) ((prefix+=4)) ;;
      224) ((prefix+=3)) ;;
      192) ((prefix+=2)) ;;
      128) ((prefix+=1)) ;;
    esac
  done
  echo "$prefix"
}

#######################################
# Main event handler for udhcpc.
# Globals:
#   OUTPUT_FILE
#   ip, serverid, dns, router, bootfile, domain
#   tftp, vivso, vendor (provided by udhcpc)
# Arguments:
#   $1: Event name (bound, renew, deconfig, leasefail, nak)
# Returns:
#   None
#######################################
main() {
  local event="$1"

  case "$event" in
    bound|renew)
      # Capture standard DHCP options provided by busybox udhcpc
      # We write them as shell variables to be sourced by discovery script
      {
        echo "# DHCP Lease Info"
        echo "DHCP_IP='$ip'"
        echo "DHCP_SERVER='$serverid'"
        echo "DHCP_DNS='$dns'"
        echo "DHCP_ROUTER='$router'"
        echo "DHCP_BOOTFILE='$bootfile'"
        echo "DHCP_DOMAIN='$domain'"
        # Option 66/tftp
        echo "DHCP_TFTP='$tftp'"
        # Option 125 - Vendor Specific
        # Busybox usually exports this as hex string if requested via -O
        # We might need to handle hex decoding in discovery, strictly just dumping here
        echo "DHCP_VIVSO='$vivso'"

        # Others potentially relevant
        echo "DHCP_OPT43='$vendor'"
        echo "DHCP_OPT66='$tftp'"
        echo "DHCP_OPT67='$bootfile'"
      } > "$OUTPUT_FILE"

      # Configure network if requested
      if [[ -n "$ip" ]] && [[ -n "$interface" ]]; then
          echo "Configuring $interface with $ip..."

          # Convert subnet mask to prefix length
          # Default to /24 if subnet is missing
          local prefix=24
          if [[ -n "$subnet" ]]; then
            local calc_prefix
            calc_prefix=$(mask_to_prefix "$subnet")
            # If calculation returned a valid positive integer, use it.
            if [[ "$calc_prefix" -gt 0 ]]; then
                prefix="$calc_prefix"
            fi
          fi

          ip addr add "$ip/$prefix" dev "$interface" 2>/dev/null || true
      fi

      if [[ -n "$router" ]] && [[ -n "$interface" ]]; then
        # Simple default route add (just in case)
        ip route add default via "$router" dev "$interface" 2>/dev/null
      fi

      # We don't exit here, udhcpc continues or exits if -q was passed
      ;;
    deconfig)
      # Interface brought down?
      :
      ;;
    leasefail|nak)
      rm -f "$OUTPUT_FILE"
      ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi

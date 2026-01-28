#!/bin/bash
#
# onie-dhcp-event.sh - Handle udhcpc events for ONIE discovery
#
# This script is called by udhcpc on state changes.
# We only care about 'bound' or 'renew'.

readonly OUTPUT_FILE="/tmp/onie-dhcp-lease.sh"

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

      # Configure network if requested (optional, since we might already be online)
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

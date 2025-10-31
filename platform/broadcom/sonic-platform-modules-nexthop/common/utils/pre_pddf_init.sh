#!/bin/bash
# Installed on all PDDF based Nexthop platforms.
# Runs before PDDF.

# TODO: Move PDDF device JSON file programmatically to
#       /usr/share/sonic/device/${PLATFORM}/pddf/pddf-device.json
#       based on hardware API version.

log() {
  logger -t "pre_pddf_init" "$@"
}

nh_gen pddf_device_json
nh_gen pcie_yaml

ASIC_INIT_PATH="/usr/local/bin/asic_init.sh"
if [ -f "$ASIC_INIT_PATH" ]; then
  log "$ASIC_INIT_PATH found. Executing..."
  "$ASIC_INIT_PATH"
  RETURN_CODE=$?
  if [ $RETURN_CODE -ne 0 ]; then
    log -p error "$ASIC_INIT_PATH exited with error code: $RETURN_CODE"
  else
    log "$ASIC_INIT_PATH executed successfully."
  fi
else
  log -p warning "$ASIC_INIT_PATH not found."
fi

echo "blacklist adm1266" > /etc/modprobe.d/blacklist-adm1266.conf

exit 0

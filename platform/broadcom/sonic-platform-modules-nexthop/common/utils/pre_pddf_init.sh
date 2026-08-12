#!/bin/bash
# Installed on all PDDF based Nexthop platforms.
# Runs before PDDF.

# TODO: Move PDDF device JSON file programmatically to
#       /usr/share/sonic/device/${PLATFORM}/pddf/pddf-device.json
#       based on hardware API version.

log() {
  logger -t "pre_pddf_init" "$@"
}

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

# Run nh_gen after asic_init.sh because some template lookup commands
# require the ASIC to be out of reset. Emits pddf-device.json.base,
# which is consumed by pddfparse.expand_child_cards
nh_gen pddf_device_json_base
nh_gen pcie_yaml

if [[ -f /usr/share/sonic/platform/pddf/pddf-device.json.base ]]; then
  install -m 0644 /usr/share/sonic/platform/pddf/pddf-device.json.base \
                  /usr/share/sonic/platform/pddf/pddf-device.json
  log "seeded /usr/share/sonic/platform/pddf/pddf-device.json from .base"
else
  log -p warning "pddf-device.json.base missing; leaving canonical untouched"
fi

exit 0

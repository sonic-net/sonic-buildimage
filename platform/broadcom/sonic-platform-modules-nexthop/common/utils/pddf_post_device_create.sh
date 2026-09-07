#!/bin/bash
# Runs after PDDF's device_install has brought up the base SYSTEM tree
# (kernel modules loaded, CPLDMUX0 and POWER-DELIVERY-CARD-EEPROM
# instantiated). Invoked by pddf_util.py install via
# /usr/local/bin/pddf_post_device_create.sh.

set -e

log() {
  logger -t "pddf_post_device_create" "$@"
}

log "expanding CHILD_CARDS via pddfparse --expand-child-cards"
/usr/local/bin/pddfparse.py --expand-child-cards
log "expand_child_cards done"

exit 0

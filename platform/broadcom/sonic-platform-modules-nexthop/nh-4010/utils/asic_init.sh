#!/bin/bash

set -e

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
FPGA_BDF="0000:04:00.0"
LOG_TAG="asic_init"

fpga_write() {
  local offset="$1"
  local value="$2"

  fpga write32 "$FPGA_BDF" "$offset" "$value"
  if [ $? -ne 0 ]; then
    logger -t $LOG_TAG -p error "Error writing $value to reg $offset on fpga $FPGA_BDF"
    exit 1
  fi
}

function acquire_lock() {
  if [[ ! -f $LOCKFILE ]]; then
    touch $LOCKFILE
  fi

  logger -t $LOG_TAG "Acquiring ${LOCKFILE}"

  exec {LOCKFD}>${LOCKFILE}
  /usr/bin/flock -x ${LOCKFD}
  trap "/usr/bin/flock -u ${LOCKFD}" EXIT

  logger -t $LOG_TAG "Acquired ${LOCKFILE}"
}

function release_lock() {
  /usr/bin/flock -u ${LOCKFD}
  logger -t $LOG_TAG "Released ${LOCKFILE}"
}

acquire_lock

# Temporary workaround for 0x20d GE FPGA image for i2c.
# Should be a no op in other cases.fpga write32 0000:04:00.0 0x180 0
fpga_write 0x180 0x0
fpga_write 0x180 0x0

# Take the asic out of reset
fpga_write 0x8 0x2a
sleep 2
fpga_write 0x8 0x22
sleep 0.2
fpga_write 0x8 0x422

release_lock

exit 0

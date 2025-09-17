#!/bin/bash

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
FPGA_BDF="0000:04:00.0"
LOG_TAG="asic_init"

fpga_write() {
  local offset="$1"
  local value="$2"
  local bits="$3"

  if [ -n "$bits" ]; then
    fpga write32 "$FPGA_BDF" "$offset" "$value" --bits "$bits"
  else
    fpga write32 "$FPGA_BDF" "$offset" "$value"
  fi

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

function clear_sticky_bits() {
  # This function clears all the sticky bits (Clear On Write) for various
  # power monitoring and other status registers in the GE FPGA.

  # It's safe to just write all 1s to these regs. There are not control bits.
  # If more COW bits are added, we don't have to change this function.

  # Shift Chains Status
  fpga_write 0xf0 0xffffffff
  # Input Status State Change Flags
  fpga_write 0x120 0xffffffff
  # TH5 GPIO State Change Flags
  fpga_write 0x124 0xffffffff
  # TH5 TS I/F GPIO State Change Flags
  fpga_write 0x128 0xffffffff
  # Port 1-32 Mod Present Change Flags
  fpga_write 0x1a0 0xffffffff
  # Port 1-32 Interrupt Change Flags
  fpga_write 0x1a4 0xffffffff
  # Port 1-32 Power Good Change Flags
  fpga_write 0x1a8 0xffffffff
  # Port 33-64 Mod Present Change Flags
  fpga_write 0x1ac 0xffffffff
  # Port 33-64 Interrupt Change Flags
  fpga_write 0x1b0 0xffffffff
  # Port 33-64 Power Good Change Flags
  fpga_write 0x1b4 0xffffffff
  # CPU-Switch Card Status Change Flags
  fpga_write 0x1b8 0xffffffff
  # SFP Mgmt Card Status Change Flags
  fpga_write 0x1bc 0xffffffff
  # Miscellaneous Status 0 Change Flags
  fpga_write 0x1c0 0xffffffff
  # Miscellaneous Status 1 Change Flags
  fpga_write 0x1c4 0xffffffff
  # Fan Card Status Change Flags
  fpga_write 0x1c8 0xffffffff
}

acquire_lock

logger -t $LOG_TAG "NH-4010-r0 asic init, power cycling then releasing reset"

# On R0 we always power cycle the ASIC
fpga_write 0x8 0x1 "3:3"
sleep 2
fpga_write 0x8 0x0 "3:3"
sleep 0.2

# Per HW, do this right before taking the ASIC out of reset.
clear_sticky_bits

# Take the asic out of reset
fpga_write 0x8 0x1 "10:10"

release_lock

exit 0

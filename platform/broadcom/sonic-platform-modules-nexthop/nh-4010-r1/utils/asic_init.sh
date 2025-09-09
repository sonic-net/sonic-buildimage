#!/bin/bash

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
FPGA_BDF="0000:04:00.0"
LOG_TAG="asic_init"
ASIC_PCI_VENDOR_ID="14e4"
LOG_PRIO="user.info"
LOG_ERR="user.err"

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
    logger -t $LOG_TAG -p $LOG_ERR "Error writing $value to reg $offset on fpga $FPGA_BDF"
    exit 1
  fi
}

function acquire_lock() {
  if [[ ! -f $LOCKFILE ]]; then
    touch $LOCKFILE
  fi

  logger -t $LOG_TAG -p $LOG_PRIO "Acquiring ${LOCKFILE}"

  exec {LOCKFD}>${LOCKFILE}
  /usr/bin/flock -x ${LOCKFD}
  trap "/usr/bin/flock -u ${LOCKFD}" EXIT

  logger -t $LOG_TAG -p $LOG_PRIO "Acquired ${LOCKFILE}"
}

function release_lock() {
  /usr/bin/flock -u ${LOCKFD}
  logger -t $LOG_TAG -p $LOG_PRIO "Released ${LOCKFILE}"
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

if [ -f /disable_asic ]; then
  logger -p user.warning -t $LOG_TAG "ASIC init disabled due to /disable_asic file"
  release_lock
  exit 0
fi

acquire_lock

# Per HW, do this right before taking the ASIC out of reset.
clear_sticky_bits

# Take the asic out of reset, first we'll set the reset bit, then clear it
# Setting the reset bit should be a no-op as the FPGA default for this bit is 0.
logger -t $LOG_TAG -p $LOG_PRIO "NH-4010-r1 asic init, putting ASIC into reset, then releasing reset"
fpga_write 0x8 0x0 "10:10"
sleep 0.1
fpga_write 0x8 0x1 "10:10"

# We need to wait for the asic to come up
sleep 1

# Check if switch ASIC is up
lspci -n | grep -q "$ASIC_PCI_VENDOR_ID"
if [ $? -eq 0 ]; then
  logger -t $LOG_TAG -p $LOG_PRIO "Switch ASIC is up"
  release_lock
  exit 0
fi

logger -t $LOG_TAG -p $LOG_ERR "Switch ASIC not present, power cycling"

# Try power cycling, up to two times, or until Switch ASIC chip is found
for attempt in {1..2}; do
  # Powercycle the asic, then take it out of reset
  fpga_write 0x8 0x1 "3:3"
  sleep 2
  fpga_write 0x8 0x0 "3:3"
  sleep 0.2
  fpga_write 0x8 0x1 "10:10"

  # We need to wait for the asic to come up
  sleep 1

  # Check if switch ASIC is up
  lspci -n | grep -q "$ASIC_PCI_VENDOR_ID"
  if [ $? -eq 0 ]; then
    logger -t $LOG_TAG -p $LOG_PRIO "Switch ASIC is up after power cycle attempt $attempt"
    release_lock
    exit 0
  fi
done

logger -t $LOG_TAG -p $LOG_ERR "Switch ASIC not found after power cycle attempt $attempt, giving up."

release_lock

exit 1

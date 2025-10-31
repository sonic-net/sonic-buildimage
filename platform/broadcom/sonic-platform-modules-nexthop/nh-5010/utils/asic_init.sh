#!/bin/bash

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
FPGA_BDF=$(setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0')
KOMODO_FPGA_BDF=$(setpci -s 00:02.1 0x19.b | xargs printf '0000:%s:00.0')
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

komodo_fpga_read() {
  local offset="$1"
  fpga read32 "$KOMODO_FPGA_BDF" "$offset"
  if [ $? -ne 0 ]; then
    logger -t $LOG_TAG -p error "Error reading from reg $offset on fpga $KOMODO_FPGA_BDF"
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

function enable_phy() {
  #Put PHYs in reset
  fpga_write 0x4090 0xFFFF2000
  fpga_write 0x4094 0xFFFF2000
  sleep 1

  #Enable PHY clocks
  fpga_write 0x4090 0xFFFF2100
  fpga_write 0x4094 0xFFFF2100
  sleep 1

  #PHYs out of reset and SERBOOT set to SPI boot
  fpga_write 0x4090 0xFFFF3FFF
  fpga_write 0x4094 0xFFFF3FFF
}

function clear_sticky_bits() {
  # This function clears all the sticky bits (Clear On Write) for various
  # power monitoring and other status registers in the switch/mezz card FPGAs.

  ### Switch Card
  # Shift Chains Status
  fpga_write 0xf0 0xffffffff
  # Input Status State Change Flags
  fpga_write 0x120 0xffffffff
  # Q3D GPIO State Change Flags
  fpga_write 0x124 0xffffffff
  # Q3D TS I/F GPIO State Change Flags
  fpga_write 0x128 0xffffffff
  # Port 33-48 Mod Present Change Flags
  fpga_write 0x1a0 0xffffffff
  # Port 33-48 Interrupt Change Flags
  fpga_write 0x1a4 0xffffffff
  # Port 33-48 Power Good Change Flags
  fpga_write 0x1a8 0xffffffff
  # Port 49-64 Mod Present Change Flags
  fpga_write 0x1ac 0xffffffff
  # Port 49-64 Interrupt Change Flags
  fpga_write 0x1b0 0xffffffff
  # Port 49-64 Power Good Change Flags
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
  # Mezz-Switch Card Status Change Flags
  fpga_write 0x1cc 0xffffffff

  ### Mezz Card
  # Shift Chains Status
  fpga_write 0x40f0 0xffffffff
  # Input Status State Change Flags
  fpga_write 0x4120 0xffffffff
  # Port 1-16 Mod Present Change Flags
  fpga_write 0x41a0 0xffffffff
  # Port 1-16 Interrupt Change Flags
  fpga_write 0x41a4 0xffffffff
  # Port 1-16 Power Good Change Flags
  fpga_write 0x41a8 0xffffffff
  # Port 17-32 Mod Present Change Flags
  fpga_write 0x41ac 0xffffffff
  # Port 17-32 Interrupt Change Flags
  fpga_write 0x41b0 0xffffffff
  # Port 17-32 Power Good Change Flags
  fpga_write 0x41b4 0xffffffff
  # Miscellaneous Status West Change Flags
  fpga_write 0x41c0 0xffffffff
  # Miscellaneous Status East Change Flags
  fpga_write 0x41c4 0xffffffff
  # Mezz-Switch Card Status Change Flags
  fpga_write 0x41cc 0xffffffff
}

if [ -f /disable_asic ]; then
  logger -p user.warning -t $LOG_TAG "ASIC init disabled due to /disable_asic file"
  exit 0
fi

acquire_lock

# Per HW, do this right before taking the ASIC out of reset.
clear_sticky_bits

# Switchcard revision is in Komodo FPGA register 0x44 bottom 4 bits
switchcard_revision=$(($(komodo_fpga_read 0x44) & 0xF))

# Take the asic out of reset
fpga_write 0x8 0x112
sleep 2
fpga_write 0x8 0x102
sleep 0.2
fpga_write 0x8 0x502

enable_phy

release_lock

exit 0

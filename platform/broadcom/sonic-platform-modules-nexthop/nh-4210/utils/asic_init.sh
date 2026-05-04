#!/bin/bash

# Ran during boot up and after syncd reset

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
ASIC_BRIDGE="00:02.1"
FPGA_0_BDF=$(setpci -s 00:01.4 0x19.b | xargs printf '0000:%s:00.0')
FPGA_1_BDF=$(setpci -s 00:01.5 0x19.b | xargs printf '0000:%s:00.0')
ASIC_BDF=$(setpci -s 00:02.1 0x19.b | xargs printf '%s:00.0')
LOG_PRIO="user.info"
LOG_ERR="user.err"

lsmod | grep -q 'linux_ngbde'
IS_OPENNSL_INITIALLY_LOADED=$?

if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
  LOG_TAG="asic_reset"
else
  LOG_TAG="asic_init"
fi

fpga_0_write() {
  local offset="$1"
  local value="$2"
  local bits="$3"

  if [ -n "$bits" ]; then
    fpga write32 "$FPGA_0_BDF" "$offset" "$value" --bits "$bits" > /dev/null
  else
    fpga write32 "$FPGA_0_BDF" "$offset" "$value" > /dev/null
  fi

  if [ $? -ne 0 ]; then
    logger -t $LOG_TAG -p $LOG_ERR "Error writing $value to reg $offset on switch fpga $FPGA_0_BDF"
    exit 1
  fi
}

fpga_1_write() {
  local offset="$1"
  local value="$2"
  local bits="$3"

  if [ -n "$bits" ]; then
    fpga write32 "$FPGA_1_BDF" "$offset" "$value" --bits "$bits" > /dev/null
  else
    fpga write32 "$FPGA_1_BDF" "$offset" "$value" > /dev/null
  fi

  if [ $? -ne 0 ]; then
    logger -t $LOG_TAG -p $LOG_ERR "Error writing $value to reg $offset on mezz fpga $FPGA_1_BDF"
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
  # power monitoring and other status registers in the Sea Eagle FPGAs.
  # FPGA 0, Switch Card, DevID 0x7018
  # FPGA 1, Mezz Card,   DevID 0x7019
  # Are both separate PCIe devices accessed through different BDFs.

  # Carrying over from other products:
  # It is safe to just write all 1s to these regs. They are not control bits.
  # If more COW bits are added, we don't have to change this function.

  # FPGA 0
  # Shift Chains Status
  fpga_0_write 0xf0 0xffffffff
  # Input Status State Change Flags
  fpga_0_write 0x120 0xffffffff
  # TH6 Miscellaneous Status Change Flags
  fpga_0_write 0x124 0xffffffff
  # TH6 Timestamp Interface GPIO State Change Flags
  fpga_0_write 0x128 0xffffffff
  # Interrupt Status 0
  fpga_0_write 0x174 0xffffffff
  # Interrupt Status 1
  fpga_0_write 0x17c 0xffffffff
  # Interrupt Status 2
  fpga_0_write 0x184 0xffffffff
  # PDC Status 0 Change Flags
  fpga_0_write 0x1b0 0xffffffff
  # PDC Status 1 Change Flags
  fpga_0_write 0x1b4 0xffffffff
  # CPU-Switch Card Status Change Flags
  fpga_0_write 0x1b8 0xffffffff
  # SFP Mgmt Card Status Change Flags
  fpga_0_write 0x1bc 0xffffffff
  # Miscellaneous Status 0 Change Flags
  fpga_0_write 0x1c0 0xffffffff
  # Miscellaneous Status 1 Change Flags
  fpga_0_write 0x1c4 0xffffffff

  # FPGA 1
  # Shift Chains Status
  fpga_1_write 0xf0 0xffffffff
  # Input Status State Change Flags
  fpga_1_write 0x120 0xffffffff
  # Interrupt Status 0
  fpga_1_write 0x174 0xffffffff
  # Interrupt Status 1
  fpga_1_write 0x17c 0xffffffff
  # Interrupt Status 2
  fpga_1_write 0x184 0xffffffff
  # Port 1-32 Module Present Change Flags
  fpga_1_write 0x1a0 0xffffffff
  # Port 1-32 Interrupt Change Flags
  fpga_1_write 0x1a4 0xffffffff
  # Port 1-32 Power Good Change Flags
  fpga_1_write 0x1a8 0xffffffff
  # Port 33-64 Module Present Change Flags
  fpga_1_write 0x1ac 0xffffffff
  # Port 33-64 Interrupt Change Flags
  fpga_1_write 0x1b0 0xffffffff
  # Port 33-64 Power Good Change Flags
  fpga_1_write 0x1b4 0xffffffff
  # Mezz 0 Port 1-32 Module Present Change Flags
  fpga_1_write 0x1b8 0xffffffff
  # Mezz 0 Port 1-32 Interrupt Change Flags
  fpga_1_write 0x1bc 0xffffffff
  # Mezz 0 Port 1-32 Power Good Change Flags
  fpga_1_write 0x1c0 0xffffffff
  # Mezz 1 Port 33-64 Module Present Change Flags
  fpga_1_write 0x1c4 0xffffffff
  # Mezz 1 Port 33-64 Interrupt Change Flags
  fpga_1_write 0x1c8 0xffffffff
  # Mezz 1 Port 33-64 Power Good Change Flags
  fpga_1_write 0x1cc 0xffffffff
  # Fan Card Status 0 State Change Flags
  fpga_1_write 0x1d0 0xffffffff
  # Fan Card Status 1 State Change Flags
  fpga_1_write 0x1d4 0xffffffff
}

if [ -f /disable_asic ]; then
  logger -p user.warning -t $LOG_TAG "ASIC init disabled due to /disable_asic file"
  release_lock
  exit 0
fi

acquire_lock

if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
  logger -t $LOG_TAG -p $LOG_PRIO "Removing ASIC modules"
  /etc/init.d/opennsl-modules stop
fi

fpga_0_write 0x8 0x0 "3:3"
fpga_0_write 0x8 0x1 "10:10"

# Try power cycling, up to two times, or until Switch ASIC chip is found
for attempt in {0..2}; do
  logger -t $LOG_TAG -p $LOG_PRIO "Power cycle attempt $attempt"

  fpga_0_write 0x90 0x0 "9:9"
  sleep 1

  fpga_0_write 0x90 0x1 "9:9"
  logger -t $LOG_TAG -p $LOG_PRIO "Waiting for power-up sequence"

  sleep 8

  clear_sticky_bits

  lspci -n | grep -q "$ASIC_BDF"
  if [ $? -eq 0 ]; then
    if [ "$attempt" -eq 0 ]; then
        logger -t "$LOG_TAG" -p "$LOG_PRIO" "Switch ASIC is up"
    else
        logger -t "$LOG_TAG" -p "$LOG_PRIO" "Switch ASIC is up after power cycle $attempt"
    fi

    logger -t $LOG_TAG -p $LOG_PRIO "Current lspci error(s) output"
    output=$(lspci -vvv 2>/dev/null | grep -i -e '^0' -e 'CESta' | grep -B 1 -e 'CESta' | grep -B 1 -e '+ ' -e '+$')
    logger -t $LOG_TAG -p $LOG_PRIO "lspci Errors: ${output}"

    if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules: $(lsmod | grep linux_ngbde)"
      /etc/init.d/opennsl-modules start
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules done: $(lsmod | grep linux_ngbde)"
    fi

    release_lock
    exit 0
  fi
done

logger -t $LOG_TAG -p $LOG_ERR "Switch ASIC not found after power cycle attempts, giving up."

if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
  logger -t $LOG_TAG -p $LOG_PRIO "Reloading ASIC modules despite failure"
  /etc/init.d/opennsl-modules start
fi

release_lock

exit 1

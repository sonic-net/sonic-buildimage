#!/bin/bash

# Ran during boot up and after syncd reset

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
FPGA_BDF=$(setpci -s 00:02.2 0x19.b | xargs printf '0000:%s:00.0')
ASIC_BDF=$(setpci -s 00:01.2 0x19.b | xargs printf '%s:00.0')
LOG_PRIO="user.info"
LOG_ERR="user.err"

lsmod | grep -q 'linux_ngbde'
IS_OPENNSL_INITIALLY_LOADED=$?

if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
  LOG_TAG="asic_reset"
else
  LOG_TAG="asic_init"
fi

fpga_write() {
  local offset="$1"
  local value="$2"
  local bits="$3"

  if [ -n "$bits" ]; then
    fpga write32 "$FPGA_BDF" "$offset" "$value" --bits "$bits" > /dev/null
  else
    fpga write32 "$FPGA_BDF" "$offset" "$value" > /dev/null
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
  # TH6 GPIO State Change Flags
  fpga_write 0x124 0xffffffff
  # TH6 TS I/F GPIO State Change Flags
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
  # Fan Card Status 0 Change Flags
  fpga_write 0x1c8 0xffffffff
  # Fan Card Status 1 Change Flags
  fpga_write 0x1cc 0xffffffff
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


# Try power cycling, up to two times, or until Switch ASIC chip is found
for attempt in {0..2}; do
  # Powercycle the asic, then take it out of reset
  fpga_write 0x8 0x1 "3:3"
  sleep 2
  fpga_write 0x8 0x0 "3:3"
  sleep 0.2
  
  clear_sticky_bits

  fpga_write 0x8 0x1 "10:10"

  # We need to wait for the asic to come up
  sleep 3

  # Check if switch ASIC is up
  lspci -n | grep -q "$ASIC_BDF"
  if [ $? -eq 0 ]; then
    if [ "$attempt" -eq 0 ]; then
        logger -t "$LOG_TAG" -p "$LOG_PRIO" "Switch ASIC is up"
    else
        logger -t "$LOG_TAG" -p "$LOG_PRIO" "Switch ASIC is up after power cycle $attempt"
    fi

    # This entire section may vary
    logger -t $LOG_TAG -p $LOG_PRIO "Current lspci error(s) output"
    output=$(lspci -vvv 2>/dev/null | grep -i -e '^0' -e 'CESta' | grep -B 1 -e 'CESta' | grep -B 1 -e '+ ' -e '+$')
    logger -t $LOG_TAG -p $LOG_PRIO "lspci Errors: ${output}"

    logger -t $LOG_TAG -p $LOG_PRIO "Clearing lspci errors"
    setpci -s "00:01.2" 0x160.l=$(setpci -s "00:01.2" 0x160.l)

    # Enable CommClk use
    setpci -s 01:00.0 0xbc.w=0x40
    setpci -s 0:1.2 0x68.w=0x40
    setpci -s 0:1.2 0x68.w=0x60

    if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules: $(lsmod | grep linux_ngbde)"
      /etc/init.d/opennsl-modules start
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules done: $(lsmod | grep linux_ngbde)"
    fi

    release_lock
    exit 0
  fi
done

logger -t $LOG_TAG -p $LOG_ERR "Switch ASIC not found after power cycle attempt $attempt, giving up, powering it down."
fpga_write 0x8 0x1 "3:3"

release_lock

exit 1

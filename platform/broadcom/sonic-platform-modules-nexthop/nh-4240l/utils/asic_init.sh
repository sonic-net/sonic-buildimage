#!/bin/bash

# Ran during boot up and after syncd reset

LOCKFD=200
LOCKFILE="/var/run/nexthop-asic-init.lock"
ASIC_BRIDGE="00:02.1"
FPGA_BRIDGE="00:01.4"
FPGA_BDF=$(setpci -s $FPGA_BRIDGE 0x19.b | xargs printf '0000:%s:00.0')
ASIC_BDF=$(setpci -s $ASIC_BRIDGE 0x19.b | xargs printf '%s:00.0')
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

fpga_read() {
  local offset="$1"
  local bits="$2"
  local result

  if [ -n "$bits" ]; then
    result=$(fpga read32 "$FPGA_BDF" "$offset" --bits "$bits")
  else
    result=$(fpga read32 "$FPGA_BDF" "$offset")
  fi

  if [ $? -ne 0 ]; then
    logger -t $LOG_TAG -p $LOG_ERR "Error reading reg $offset on fpga $FPGA_BDF"
    exit 1
  fi

  echo "$result"
}

function cleanup() {
  # --end before the unlock, or a queued run's rows get deleted by ours.
  nh_asic_powercycle --end
  /usr/bin/flock -u ${LOCKFD}
  logger -t $LOG_TAG -p $LOG_PRIO "Released ${LOCKFILE}"
}

function acquire_lock() {
  if [[ ! -f $LOCKFILE ]]; then
    touch $LOCKFILE
  fi

  logger -t $LOG_TAG -p $LOG_PRIO "Acquiring ${LOCKFILE}"

  exec {LOCKFD}>${LOCKFILE}
  /usr/bin/flock -x ${LOCKFD}
  trap cleanup EXIT

  logger -t $LOG_TAG -p $LOG_PRIO "Acquired ${LOCKFILE}"
}

if [ -f /disable_asic ]; then
  logger -p user.warning -t $LOG_TAG "ASIC init disabled due to /disable_asic file"
  exit 0
fi

acquire_lock

if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
  logger -t $LOG_TAG -p $LOG_PRIO "Removing ASIC modules"
  /etc/init.d/opennsl-modules stop
fi

# The ASIC is down from here until cleanup runs.
LOG_TAG="$LOG_TAG" nh_asic_powercycle --begin --xcvr-cache

# Set DP_PWR_ON = 1
# DP_POWR_ON should already be 1 in normal circumstances, but it's possible
# a power glitch brings it to 0. The system may kernel panic if we bring up
# the ASIC when DP_PWR_ON = 0.
fpga_write 0x90  0x1 "9:9"

# Try power cycling, up to two times, or until Switch ASIC chip is found
for attempt in {0..2}; do
  # Powercycle the asic, then take it out of reset
  fpga_write 0x8 0x1 "3:3"
  sleep 2
  fpga_write 0x8 0x0 "3:3"
  sleep 0.2

  fpga_write 0x8 0x1 "10:10"

  # We need to wait for the asic to come up
  sleep 3

  # Log ASIC_PGOOD
  pg_bit=$(fpga_read 0x94 "15:15")
  logger -t "$LOG_TAG" -p "$LOG_PRIO" "ASIC_PGOOD = $pg_bit"
  if [ $(( pg_bit )) -eq 1 ]; then
    logger -t "$LOG_TAG" -p "$LOG_PRIO" "ASIC power good"
  else
    logger -t "$LOG_TAG" -p "$LOG_ERR" "ASIC power good bit not set"
  fi

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
    # setpci -s "$ASIC_BRIDGE" 0x160.l=$(setpci -s "$ASIC_BRIDGE" 0x160.l)

    # Enable CommClk use
    # setpci -s "$ASIC_BDF" 0xbc.w=0x40
    # setpci -s "$ASIC_BRIDGE" 0x68.w=0x40
    # setpci -s "$ASIC_BRIDGE" 0x68.w=0x60

    if [ "$IS_OPENNSL_INITIALLY_LOADED" -eq 0 ]; then
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules: $(lsmod | grep linux_ngbde)"
      /etc/init.d/opennsl-modules start
      logger -t $LOG_TAG -p $LOG_PRIO "Inserting ASIC modules done: $(lsmod | grep linux_ngbde)"
    fi

    exit 0
  fi
done

logger -t $LOG_TAG -p $LOG_ERR "Switch ASIC not found after power cycle attempt $attempt, giving up, powering it down."
fpga_write 0x8 0x1 "3:3"

exit 1

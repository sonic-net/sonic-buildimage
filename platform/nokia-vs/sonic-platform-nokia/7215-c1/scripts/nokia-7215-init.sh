#!/bin/bash

c1_log()  { echo "[rotate-usb] $*"; }
c1_err()  { echo "[rotate-usb:ERROR] $*" >&2; }
c1_die()  { c1_err "$1"; return 1; }          # don't exit the whole script
c1_is_root() { [[ $EUID -eq 0 ]]; }

c1_iface_exists() { [[ -e "/sys/class/net/$1" ]]; }

# If name matches eth<digits>, echo numeric suffix; else return non-zero
c1_eth_suffix() {
  local ifc="$1"
  if [[ "$ifc" =~ ^eth([0-9]+)$ ]]; then
    echo "${BASH_REMATCH[1]}"
    return 0
  fi
  return 1
}

# Detect if an interface is on USB (prefer sysfs; fallback to ethtool -i)
c1_is_usb_iface() {
  local ifc="$1"
  # sysfs path check
  if [[ -L "/sys/class/net/$ifc/device" ]]; then
    local p
    p="$(readlink -f "/sys/class/net/$ifc/device" 2>/dev/null || true)"
    [[ "$p" == *"/usb"* ]] && return 0
  fi
  # ethtool heuristic
  if command -v ethtool >/dev/null 2>&1; then
    local info
    info="$(ethtool -i "$ifc" 2>/dev/null || true)"
    grep -qiE 'bus-info:.*usb|driver:.*(r8152|asix|ax88179_178a|cdc_ether|cdc_ncm|aqc111|usbnet)' <<<"$info" \
      && return 0
  fi
  return 1
}

# Find the first eth* that looks like a USB NIC
c1_find_usb_eth() {
  local ifc
  for ifc in /sys/class/net/eth*; do
    [[ -e "$ifc" ]] || continue
    ifc="${ifc##*/}"
    c1_is_usb_iface "$ifc" && { echo "$ifc"; return 0; }
  done
  return 1
}

# unique temp name for collision-free renames
c1_pick_temp_name() {
  local base="__c1_tmp"
  local i=0
  while c1_iface_exists "${base}${i}"; do ((i++)); done
  echo "${base}${i}"
}

# admin-UP check via ip (not operstate)
c1_link_is_up() {
  ip -o link show dev "$1" 2>/dev/null | grep -q ' state UP '
}

c1_run() {
    eval "$@"
}

# ----- public function you will call -----
rotate_usb_to_eth0() {

  # preflight
  c1_is_root || { c1_err "Run as root."; return 1; }
  command -v ip >/dev/null 2>&1 || { c1_err "'ip' command not found."; return 1; }

  # collect eth* interfaces sorted by numeric order
  local -a ETHS=()
  mapfile -t ETHS < <(ls -1 /sys/class/net 2>/dev/null | grep -E '^eth[0-9]+$' | sort -V)
  if ((${#ETHS[@]} == 0)); then
    c1_log "No eth* interfaces found. Nothing to do."
    return 2
  fi

  # identify USB NIC (optional)
  local USB_IF=""
  if ! USB_IF="$(c1_find_usb_eth)"; then
    c1_log "No USB NIC detected among eth*; will reserve eth0 by shifting others +1 if needed."
  else
    c1_log "Detected USB NIC: $USB_IF"
  fi

  # Determine if eth0 exists currently
  local has_eth0=0
  local ifc
  for ifc in "${ETHS[@]}"; do
    [[ "$ifc" == "eth0" ]] && { has_eth0=1; break; }
  done

  # Build desired mapping:
  # - If USB found: USB -> eth0; others -> eth(N+1)
  # - If no USB:
  #     - If eth0 exists: shift EVERY ethN -> eth(N+1) to free eth0
  #     - If eth0 absent: nothing to change (already reserved)
  declare -A DESIRED=()

  if [[ -n "$USB_IF" ]]; then
    # USB present: USB -> eth0; others -> +1
    for ifc in "${ETHS[@]}"; do
      if [[ "$ifc" == "$USB_IF" ]]; then
        DESIRED["$ifc"]="eth0"
      else
        if suffix=$(c1_eth_suffix "$ifc"); then
          DESIRED["$ifc"]="eth$((suffix + 1))"
        fi
      fi
    done
  else
    # USB absent: only shift if eth0 currently exists
    if (( has_eth0 )); then
      for ifc in "${ETHS[@]}"; do
        if suffix=$(c1_eth_suffix "$ifc"); then
          DESIRED["$ifc"]="eth$((suffix + 1))"
        fi
      done
    else
      c1_log "eth0 is already free; nothing to do."
      return 0
    fi
  fi

  # If already in desired state, exit
  local need_change=0
  for ifc in "${!DESIRED[@]}"; do
    [[ "$ifc" != "${DESIRED[$ifc]}" ]] && { need_change=1; break; }
  done
  (( need_change == 0 )) && { c1_log "Names already in desired state."; return 0; }

  # Plan
  c1_log "Planned rotation:"
  for ifc in "${ETHS[@]}"; do
    [[ -n "${DESIRED[$ifc]:-}" ]] || continue
    printf "[rotate-usb]   %s -> %s\n" "$ifc" "${DESIRED[$ifc]}"
  done

  # Stage 1: move to temps (record admin state)
  declare -A TMP=() STATE=()
  for ifc in "${ETHS[@]}"; do
    [[ -n "${DESIRED[$ifc]:-}" ]] || continue
    if c1_link_is_up "$ifc"; then
      STATE["$ifc"]="up"
      c1_run ip link set dev "$ifc" down
    else
      STATE["$ifc"]="down"
    fi
    local tmp; tmp="$(c1_pick_temp_name)"
    TMP["$ifc"]="$tmp"
    c1_run ip link set dev "$ifc" name "$tmp"
  done

  # Stage 2: temps -> final
  local final tmp
  for ifc in "${ETHS[@]}"; do
    [[ -n "${DESIRED[$ifc]:-}" ]] || continue
    tmp="${TMP[$ifc]}"
    final="${DESIRED[$ifc]}"
    c1_run ip link set dev "$tmp" name "$final"
  done

  # Stage 3: restore admin state
  for ifc in "${ETHS[@]}"; do
    [[ -n "${DESIRED[$ifc]:-}" ]] || continue
    final="${DESIRED[$ifc]}"
    if [[ "${STATE[$ifc]}" == "up" ]]; then
      c1_run ip link set dev "$final" up
    fi
  done

  c1_log "Rotation complete."
  return 0
}

# Platform init script
# Check if init is already done previously

KVER=6.12.41+deb13-sonic-arm64

# Load required kernel-mode drivers
load_kernel_drivers() {
    echo "Loading Kernel Drivers"
    lsmod |grep nokia_7215_ixs_c1_cpld > /dev/null
    if [ "$?" != "0" ]; then
    sudo insmod /lib/modules/${KVER}/kernel/extra/nokia_7215_ixs_c1_cpld.ko
    fi
    sudo insmod /lib/modules/${KVER}/kernel/extra/cn9130_cpu_thermal_sensor.ko
    sudo insmod /lib/modules/${KVER}/kernel/extra/nokia_console_fpga.ko
    sudo insmod /lib/modules/${KVER}/kernel/extra/cn9130_led.ko
}

fw_uboot_env_cfg()
{
    echo "Setting up U-Boot environment for Nokia-7215-C1"
    FW_ENV_DEFAULT='/dev/mtd1 0x0 0x10000 0x10000'
    echo $FW_ENV_DEFAULT > /etc/fw_env.config
}

nokia_7215_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/arm64-nokia_ixs7215_c1xa-r0/Nokia-7215-C1/profile.ini
    echo "Nokia-7215-C1: Updating switch mac address ${MAC_ADDR}"
}
file_exists() {
    # Wait 10 seconds max till file exists
    for((i=0; i<10; i++));
    do
        if [ -f $1 ]; then
            return 1
        fi
        sleep 1
    done
    return 0
 }

# - Main entry

lsmod |grep cn9130_cpu_thermal_sensor > /dev/null
if [ "$?" = "0" ]; then
    exit 0
fi

# Install kernel drivers required for i2c bus access
load_kernel_drivers

# Disable sysrq-trigger
echo 0 > /proc/sys/kernel/sysrq

#setting up uboot environment
fw_uboot_env_cfg

# Enumerate power monitor
echo ina230 0x40 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate fan
echo emc2305 0x2f > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate Thermals
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x4A > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate system eeprom
echo 24c64 0x53 > /sys/bus/i2c/devices/i2c-0/new_device

file_exists /sys/bus/i2c/devices/i2c-0/0-0053/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/i2c-0/0-0053/eeprom
else
    echo "SYSEEPROM file not foud"
fi

#Enumurate GPIO
echo 41 > /sys/class/gpio/export
echo 61 > /sys/class/gpio/export
echo 62 > /sys/class/gpio/export
chmod 666 /sys/class/gpio/gpio41/value

# Get list of the mux channels
for((i=0; i<10; i++));
    do
        ismux_bus=$(i2cdetect -l|grep mux|cut -f1)
        if [[ $ismux_bus ]]; then
            break;
        fi
        sleep 1
    done

# Enumerate the SFP eeprom device on each mux channel
for mux in ${ismux_bus}
do
    echo optoe2 0x50 > /sys/bus/i2c/devices/${mux}/new_device
done

# Enable optical SFP Tx
for i in {1..2}
do
    echo 0 > /sys/bus/i2c/devices/0-0041/sfp${i}_tx_disable
done

#slow down fan speed to 50% untill thermal algorithm kicks in
i2c_path="/sys/bus/i2c/devices/0-002f/hwmon/hwmon?"
echo 128 > $i2c_path/pwm1
echo 128 > $i2c_path/pwm2

# Ensure switch is programmed with base MAC addr
nokia_7215_profile

echo "drivers loaded"
rotate_usb_to_eth0

ip link set eth1 name tmpeth1
ip link set eth2 name tmpeth2
ip link set tmpeth1 name eth2
ip link set tmpeth2 name eth1

for i in 1 2 3; do
    ip link set eth$i down
    ethtool -K eth$i rxhash on
    ethtool -X eth$i weight 1 1 0 1
done

exit 0

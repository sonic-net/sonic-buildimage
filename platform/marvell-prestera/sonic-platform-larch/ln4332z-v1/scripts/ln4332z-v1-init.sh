#!/bin/bash
# Platform init for the Lanaotek LN-4332Z1 (arm64-larch_ln4332z-v1).
#
# CPU: SolidRun CEx7 CN913x COM Express type 7 module (cp0 + cp1 + cp2). The
# carrier's I2C devices sit behind three Silabs CP2112 USB-to-I2C bridges, one
# per USB host (see sonic_platform/bridges.py for the map). The bridges get
# their i2c adapter numbers when hid-cp2112 binds, so everything below is
# resolved from the USB topology instead of fixed bus numbers.
#
# Only known addresses are ever accessed through the bridges: a transfer to an
# unknown address can time out and wedge a CP2112 until the board is powered
# off, so no i2cdetect-style scanning here.

set -u

SYS_XHCI=f2510000.usb3      # cp0: PSUs, board temperature, SyncE, ASIC I2C
IO_XHCI=f4500000.usb3       # cp1: QSFP ModPrsL/IntL/ResetL/LPMode expanders
EEPROM_XHCI=f6500000.usb3   # cp2: QSFP28/SFP+ EEPROM muxes, SFP+ control

log() { echo "ln4332z-v1-init: $*"; }

fw_uboot_env_cfg()
{
    # 64 KiB U-Boot environment in the module SPI-NOR (mtd partition
    # "uboot-env"); present it to fw_setenv as one sector - busybox and
    # libubootenv both mishandle 4 KiB sectors for this layout.
    local mtd
    mtd=$(grep -E '"uboot-env"' /proc/mtd | sed -e 's/:.*$//')
    [ -n "$mtd" ] || mtd=mtd1
    echo "/dev/$mtd 0x0 0x10000 0x10000" > /etc/fw_env.config
}

SYSEEPROM=/sys/bus/i2c/devices/0-0050/eeprom
UBOOT_DEFAULT_MAC=00:51:82:11:22:00

# syseeprom_base_mac - Base MAC Address (TLV 0x24) of the ONIE system EEPROM
syseeprom_base_mac()
{
    python3 - "$SYSEEPROM" <<'EOF' 2>/dev/null
import sys
d = open(sys.argv[1], "rb").read()
if d[:8] != b"TlvInfo\0":
    sys.exit(1)
end = 11 + int.from_bytes(d[9:11], "big")
i = 11
while i + 2 <= end:
    t, n = d[i], d[i + 1]
    if t == 0x24 and n == 6:
        print(":".join("%02X" % b for b in d[i + 2:i + 8]))
        sys.exit(0)
    i += 2 + n
sys.exit(1)
EOF
}

ln4332z_v1_profile()
{
    # The unit MAC is the syseeprom Base MAC (what DEVICE_METADATA uses).
    # ethaddr in the U-Boot environment is only a fallback: on these units it
    # can still be the Marvell U-Boot default, which every board shares.
    local mac
    mac=$(syseeprom_base_mac)
    if [ -z "$mac" ]; then
        mac=$(fw_printenv -n ethaddr 2>/dev/null)
        if [ -z "$mac" ] || [ "${mac,,}" = "$UBOOT_DEFAULT_MAC" ]; then
            log "no unit MAC in the syseeprom or the U-Boot environment; leaving switchMacAddress and eth0 untouched"
            return 0
        fi
    fi
    find /usr/share/sonic/device/*ln4332z-v1* -name profile.ini | \
        xargs -r sed -i "s/switchMacAddress=.*/switchMacAddress=$mac/g"
    log "switch MAC address ${mac}"
    # eth0 (OOB) otherwise keeps the U-Boot default MAC 00:51:82:11:22:00.
    # Every unit then has the same MAC on the management network, DHCP gives
    # them the same address and management connectivity comes and goes.
    if ip link set dev eth0 address "$mac"; then
        log "eth0 MAC address ${mac}"
    else
        log "WARNING: could not set the eth0 MAC address to ${mac}"
    fi
}

load_drivers()
{
    modprobe hid_cp2112 || true
    modprobe i2c_mux_pca954x || true
    modprobe gpio_pca953x || true
    modprobe optoe || true
    modprobe pmbus || true
    modprobe lm75 || true
}

# bridge_bus <xhci> -> prints the i2c adapter number of the CP2112 on that host
bridge_bus()
{
    local xhci=$1 dev adap
    for dev in /sys/bus/usb/devices/*-*; do
        case "$(basename "$dev")" in *:*) continue;; esac
        [ "$(cat "$dev/idVendor" 2>/dev/null)" = "10c4" ] || continue
        [ "$(cat "$dev/idProduct" 2>/dev/null)" = "ea90" ] || continue
        case "$(readlink -f "$dev")" in *"$xhci"*) ;; *) continue;; esac
        for adap in "$dev"/*:1.0/0003:*/i2c-*; do
            [ -e "$adap" ] && { basename "$adap" | sed 's/^i2c-//'; return 0; }
        done
    done
    return 1
}

wait_bridges()
{
    local i
    for i in $(seq 1 30); do
        SYS_BUS=$(bridge_bus $SYS_XHCI) && IO_BUS=$(bridge_bus $IO_XHCI) && \
            EEPROM_BUS=$(bridge_bus $EEPROM_XHCI) && return 0
        sleep 1
    done
    SYS_BUS=$(bridge_bus $SYS_XHCI) || SYS_BUS=
    IO_BUS=$(bridge_bus $IO_XHCI) || IO_BUS=
    EEPROM_BUS=$(bridge_bus $EEPROM_XHCI) || EEPROM_BUS=
    log "WARNING: not all CP2112 bridges found (sys=${SYS_BUS:-none} io=${IO_BUS:-none} eeprom=${EEPROM_BUS:-none})"
    return 1
}

# add_dev <bus> <driver> <addr>  - instantiate an i2c device once
add_dev()
{
    local bus=$1 drv=$2 addr=$3
    [ -n "$bus" ] || return 1
    [ -e "/sys/bus/i2c/devices/${bus}-$(printf '%04x' "$addr")" ] && return 0
    echo "$drv $(printf '0x%02x' "$addr")" > "/sys/bus/i2c/devices/i2c-${bus}/new_device"
}

# mux_child <bus> <mux addr> <channel> -> adapter number behind the channel
mux_child()
{
    local link="/sys/bus/i2c/devices/$1-$(printf '%04x' "$2")/channel-$3"
    [ -e "$link" ] || return 1
    basename "$(readlink -f "$link")" | sed 's/^i2c-//'
}

setup_sys_bridge()
{
    [ -n "$SYS_BUS" ] || return 0
    local ch
    add_dev "$SYS_BUS" pca9548 0x70
    add_dev "$SYS_BUS" pca9548 0x71
    sleep 0.5
    # PSU A / PSU B: PMBus at 0x5b on channels 0 / 1
    for ch in 0 1; do
        add_dev "$(mux_child "$SYS_BUS" 0x70 $ch)" pmbus 0x5b || true
    done
    # board temperature sensors on channel 3
    add_dev "$(mux_child "$SYS_BUS" 0x70 3)" lm75 0x48 || true
    add_dev "$(mux_child "$SYS_BUS" 0x70 3)" lm75 0x49 || true
    # PSU model / serial (PMBus MFR_MODEL 0x9a / MFR_SERIAL 0x9e block reads)
    # for pmon, which cannot open the run-time created /dev/i2c-* nodes
    mkdir -p /var/run/platform_cache/ln4332z-v1
    for ch in 0 1; do
        bus=$(mux_child "$SYS_BUS" 0x70 $ch) || continue
        pmbus_string "$bus" 0x9a > /var/run/platform_cache/ln4332z-v1/psu$((ch + 1))_model
        pmbus_string "$bus" 0x9e > /var/run/platform_cache/ln4332z-v1/psu$((ch + 1))_serial
    done
}

# pmbus_string <bus> <reg> - printable part of an SMBus block read at 0x5b
pmbus_string()
{
    local raw
    raw=$(i2cget -f -y "$1" 0x5b "$2" s 2>/dev/null) || { echo; return 0; }
    # "0x43 0x53 ..." (first byte = length) -> ASCII
    python3 -c 'import sys; b=[int(x,16) for x in sys.argv[1:]]; print(bytes(c for c in b[1:] if 32 <= c < 127).decode().strip())' $raw
}

# gpio_out <chip base> <offset> <high|low> - export a pca953x line as output
gpio_out()
{
    local n=$(( $1 + $2 ))
    [ -d /sys/class/gpio/gpio$n ] || echo $n > /sys/class/gpio/export 2>/dev/null
    [ "$(cat /sys/class/gpio/gpio$n/direction 2>/dev/null)" = out ] || \
        echo "$3" > /sys/class/gpio/gpio$n/direction 2>/dev/null
}

setup_io_bridge()
{
    [ -n "$IO_BUS" ] || return 0
    local addr chip base i
    for addr in 0x20 0x21 0x22 0x23 0x24 0x25 0x26 0x27; do
        add_dev "$IO_BUS" pca9555 $addr || true
    done
    sleep 0.5
    # odd expanders: port 0 = ResetL (drive high = out of reset), port 1 =
    # LPMode (drive low = high power).  The PCA9555 powers up as inputs with
    # pull-ups, i.e. LPMode asserted, so do this before xcvrd starts.
    for addr in 0x21 0x23 0x25 0x27; do
        chip=$(ls -d /sys/bus/i2c/devices/${IO_BUS}-00$(printf '%02x' $addr)/gpio/gpiochip* 2>/dev/null | head -1)
        [ -n "$chip" ] || continue
        base=$(cat "$chip/base")
        for i in 0 1 2 3 4 5 6 7; do
            gpio_out "$base" $i high
            gpio_out "$base" $((8 + i)) low
        done
    done
}

setup_eeprom_bridge()
{
    [ -n "$EEPROM_BUS" ] || return 0
    local mux ch bus
    for mux in 0x70 0x71 0x72 0x73 0x74; do
        add_dev "$EEPROM_BUS" pca9548 $mux || true
    done
    add_dev "$EEPROM_BUS" pca9555 0x20 || true      # SFP+ TX_FAULT/TX_DIS/ABS/RS/RX_LOS
    sleep 0.5
    # QSFP28 EEPROMs: every channel of muxes 0x70-0x73 (32 cages), optoe1
    for mux in 0x70 0x71 0x72 0x73; do
        for ch in 0 1 2 3 4 5 6 7; do
            bus=$(mux_child "$EEPROM_BUS" $mux $ch) && add_dev "$bus" optoe1 0x50 || true
        done
    done
    # SFP+ EEPROMs: mux 0x74 channels 0/1, optoe2
    for ch in 0 1; do
        bus=$(mux_child "$EEPROM_BUS" 0x74 $ch) && add_dev "$bus" optoe2 0x50 || true
    done
}

fw_uboot_env_cfg
ln4332z_v1_profile
load_drivers
wait_bridges || true
log "CP2112 bridges: sys=i2c-${SYS_BUS:-?} io=i2c-${IO_BUS:-?} eeprom=i2c-${EEPROM_BUS:-?}"
setup_sys_bridge
setup_io_bridge
setup_eeprom_bridge
exit 0

#!/bin/bash
# Platform init for the Larch LN-4348Y-8Z (arm64-larch_ln4348y-8z-v1).
#
# CPU: Lanaotek CN9130 COM Express type 7 module (cp0 only). The carrier's
# transceiver I2C trees sit behind three Silabs CP2112 USB-to-I2C bridges on a
# USB 2.0 hub (see sonic_platform/bridges.py for the map). The bridges get
# their i2c adapter numbers when hid-cp2112 binds and the hub port of each
# bridge is not documented, so each bridge is identified once by a read at an
# address that exists on that bridge only, and the result is cached for the
# python platform code.
#
# Only known addresses are ever accessed through the bridges: a transfer to an
# unknown address can time out and wedge a CP2112 until the board is powered
# off, so no i2cdetect-style scanning here.

set -u

CACHE_DIR=/var/run/platform_cache/ln4348y-8z-v1

log() { echo "ln4348y-8z-v1-init: $*"; }

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

ln4348y_8z_v1_profile()
{
    local mac
    mac=$(fw_printenv -n ethaddr 2>/dev/null)
    if [ -z "$mac" ]; then
        log "could not read ethaddr from the U-Boot environment; leaving switchMacAddress untouched"
        return 0
    fi
    find /usr/share/sonic/device/*ln4348y-8z-v1* -name profile.ini | \
        xargs -r sed -i "s/switchMacAddress=.*/switchMacAddress=$mac/g"
    log "switch MAC address ${mac}"
}

load_drivers()
{
    modprobe hid_cp2112 || true
    modprobe i2c_mux_pca954x || true
    modprobe gpio_pca953x || true
    modprobe optoe || true
}

# cp2112_buses -> "usbname bus" per CP2112 bridge
cp2112_buses()
{
    local dev adap
    for dev in /sys/bus/usb/devices/*-*; do
        case "$(basename "$dev")" in *:*) continue;; esac
        [ "$(cat "$dev/idVendor" 2>/dev/null)" = "10c4" ] || continue
        [ "$(cat "$dev/idProduct" 2>/dev/null)" = "ea90" ] || continue
        for adap in "$dev"/*:1.0/0003:*/i2c-*; do
            [ -e "$adap" ] && echo "$(basename "$dev") $(basename "$adap" | sed 's/^i2c-//')"
        done
    done
}

# probe <bus> <addr> [reg] - one SMBus read at a known address; success = device answers
probe()
{
    if [ $# -ge 3 ]; then
        i2cget -y "$1" "$2" "$3" >/dev/null 2>&1
    else
        i2cget -y "$1" "$2" >/dev/null 2>&1
    fi
}

identify_bridges()
{
    EEPROM_BUS= IO_BUS= QSFP_BUS=
    local i usb bus rest
    for i in $(seq 1 30); do
        [ "$(cp2112_buses | wc -l)" -ge 3 ] && break
        sleep 1
    done
    while read -r usb bus; do
        [ -n "$bus" ] || continue
        if probe "$bus" 0x75; then                 # PCA9548 U87: EEPROM bridge only
            EEPROM_BUS=$bus
        elif probe "$bus" 0x26 0; then             # PCA9555 U141: IO bridge only
            IO_BUS=$bus
        elif probe "$bus" 0x21 0 && probe "$bus" 0x70; then   # U140 + U89: QSFP bridge
            QSFP_BUS=$bus
        else
            log "WARNING: CP2112 on $usb (i2c-$bus) answers at none of the expected addresses"
        fi
    done <<EOF
$(cp2112_buses)
EOF
    # unresolved bridges: fall back to the hub port order 1/2/3 = eeprom/io/qsfp
    while read -r usb bus; do
        [ -n "$bus" ] || continue
        case "$bus" in "$EEPROM_BUS"|"$IO_BUS"|"$QSFP_BUS") continue;; esac
        case "$usb" in
            *.1) [ -n "$EEPROM_BUS" ] || EEPROM_BUS=$bus;;
            *.2) [ -n "$IO_BUS" ] || IO_BUS=$bus;;
            *.3) [ -n "$QSFP_BUS" ] || QSFP_BUS=$bus;;
        esac
    done <<EOF
$(cp2112_buses)
EOF
    mkdir -p "$CACHE_DIR"
    {
        [ -n "$EEPROM_BUS" ] && echo "eeprom $EEPROM_BUS"
        [ -n "$IO_BUS" ] && echo "io $IO_BUS"
        [ -n "$QSFP_BUS" ] && echo "qsfp $QSFP_BUS"
    } > "$CACHE_DIR/bridges"
    log "CP2112 bridges: eeprom=i2c-${EEPROM_BUS:-?} io=i2c-${IO_BUS:-?} qsfp=i2c-${QSFP_BUS:-?}"
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

# gpio_out <chip base> <offset> <high|low> - export a pca953x line as output
gpio_out()
{
    local n=$(( $1 + $2 ))
    [ -d /sys/class/gpio/gpio$n ] || echo $n > /sys/class/gpio/export 2>/dev/null
    [ "$(cat /sys/class/gpio/gpio$n/direction 2>/dev/null)" = out ] || \
        echo "$3" > /sys/class/gpio/gpio$n/direction 2>/dev/null
}

setup_eeprom_bridge()
{
    [ -n "$EEPROM_BUS" ] || return 0
    local mux ch bus
    for mux in 0x70 0x71 0x72 0x73 0x74 0x75; do
        add_dev "$EEPROM_BUS" pca9548 $mux || true
    done
    sleep 0.5
    # SFP28 EEPROMs: every channel of the six muxes (48 cages), optoe2
    for mux in 0x70 0x71 0x72 0x73 0x74 0x75; do
        for ch in 0 1 2 3 4 5 6 7; do
            bus=$(mux_child "$EEPROM_BUS" $mux $ch) && add_dev "$bus" optoe2 0x50 || true
        done
    done
}

setup_io_bridge()
{
    [ -n "$IO_BUS" ] || return 0
    local addr
    # 0x20-0x22 ABS, 0x23-0x25 RX_LOS, 0x26 TX_FAULT (port 0) / RS (port 1); all inputs
    for addr in 0x20 0x21 0x22 0x23 0x24 0x25 0x26; do
        add_dev "$IO_BUS" pca9555 $addr || true
    done
}

setup_qsfp_bridge()
{
    [ -n "$QSFP_BUS" ] || return 0
    local ch bus chip base i
    add_dev "$QSFP_BUS" pca9548 0x70 || true
    add_dev "$QSFP_BUS" pca9555 0x20 || true       # ModPrsL (port 0) / IntL (port 1)
    add_dev "$QSFP_BUS" pca9555 0x21 || true       # ResetL (port 0) / LPMode (port 1)
    sleep 0.5
    # ResetL high = out of reset, LPMode low = high power. The PCA9555 powers
    # up as inputs with pull-ups, i.e. LPMode asserted, so do this before xcvrd.
    chip=$(ls -d /sys/bus/i2c/devices/${QSFP_BUS}-0021/gpio/gpiochip* 2>/dev/null | head -1)
    if [ -n "$chip" ]; then
        base=$(cat "$chip/base")
        for i in 0 1 2 3 4 5 6 7; do
            gpio_out "$base" $i high
            gpio_out "$base" $((8 + i)) low
        done
    fi
    for ch in 0 1 2 3 4 5 6 7; do
        bus=$(mux_child "$QSFP_BUS" 0x70 $ch) && add_dev "$bus" optoe1 0x50 || true
    done
}

syseeprom_image()
{
    # the module TlvInfo EEPROM has not been located yet: build the substitute
    # image (MAC from the U-Boot environment) here on the host, where
    # /var/run/platform_cache is writable, for pmon's syseepromd
    mkdir -p "$CACHE_DIR"
    python3 -c 'from sonic_platform.eeprom import Eeprom; Eeprom()' 2>/dev/null || \
        log "WARNING: could not build the system EEPROM image"
}

fw_uboot_env_cfg
ln4348y_8z_v1_profile
load_drivers
syseeprom_image
identify_bridges
setup_eeprom_bridge
setup_io_bridge
setup_qsfp_bridge
exit 0

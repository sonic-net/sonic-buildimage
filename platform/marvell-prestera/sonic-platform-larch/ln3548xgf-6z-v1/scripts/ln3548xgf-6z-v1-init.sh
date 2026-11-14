#!/bin/bash

# Platform init script for ln3548xgf-6z-v1
# v3: resolve every EEPROM adapter by PCA9548 address/channel, never port+10.

ln3548xgf-6z-v1_profile()
{
    MAC_ADDR=$(fw_printenv -n ethaddr)
    find /usr/share/sonic/device/*ln3548xgf-6z-v1* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "ln3548xgf-6z-v1: Updating switch mac address ${MAC_ADDR}"
    sudo ifconfig eth0 hw ether $MAC_ADDR

    sudo modprobe optoe
}

sfp_init()
{
    # Linux allocates PCA9548 child bus numbers dynamically. Resolve each bus
    # through the mux channel symlink instead of assuming buses 11-64.
    bind_optoe()
    {
        mux_addr=$1
        channel=$2
        driver=$3
        channel_path="/sys/bus/i2c/devices/1-00${mux_addr}/channel-${channel}"

        if [ ! -e "$channel_path" ]; then
            echo "Missing PCA9548 channel: $channel_path" >&2
            return 1
        fi

        bus_path=$(readlink -f "$channel_path")
        bus_name=$(basename "$bus_path")
        bus=${bus_name#i2c-}
        device_path="/sys/bus/i2c/devices/${bus}-0050"

        if [ -e "$device_path" ]; then
            current_driver=$(cat "$device_path/name" 2>/dev/null || true)
            if [ "$current_driver" = "$driver" ]; then
                return 0
            fi

            echo "Rebinding bus ${bus} address 0x50: ${current_driver} -> ${driver}"
            echo 0x50 > "/sys/bus/i2c/devices/i2c-${bus}/delete_device" || return 1
        fi

        echo "${driver} 0x50" > "/sys/bus/i2c/devices/i2c-${bus}/new_device"
    }

    # SFP 1-48: muxes 0x70-0x75, all eight channels, optoe2.
    for mux_addr in 70 71 72 73 74 75; do
        for channel in $(seq 0 7); do
            bind_optoe "$mux_addr" "$channel" optoe2 || true
        done
    done

    # QSFP 1-6: mux 0x76, channels 0-5, optoe1.
    for channel in $(seq 0 5); do
        bind_optoe 76 "$channel" optoe1 || true
    done
}

sfp_mux_init()
{
    # CP_I2C1 is Linux I2C bus 1. Seven PCA9548 devices provide 54 EEPROM
    # channels. Child bus numbers are dynamic and are resolved by sfp_init().
    sudo modprobe i2c_mux_pca954x

    for addr in 70 71 72 73 74 75 76; do
        if [ -d /sys/bus/i2c/devices/i2c-1 ] && [ ! -e "/sys/bus/i2c/devices/1-00${addr}" ]; then
            echo "pca9548 0x${addr}" > /sys/bus/i2c/devices/i2c-1/new_device || true
        fi
    done

    udevadm settle || true
    sleep 1

    # All seven muxes share bus 1 and every channel carries an EEPROM at
    # 0x50. The driver default (idle-as-is) leaves the last channel of each
    # mux connected, so two muxes end up driving 0x50 concurrently and wedge
    # the bus (mv64xxx "I2C bus locked"). Force idle-disconnect on all of
    # them, including the DTB-declared ones (0x71/0x72/0x76).
    for addr in 70 71 72 73 74 75 76; do
        if [ -e "/sys/bus/i2c/devices/1-00${addr}/idle_state" ]; then
            echo -2 > "/sys/bus/i2c/devices/1-00${addr}/idle_state" || true
        fi
    done
}

sfp_gpio_init()
{
    # PCA9555 chips 0-7 are on CP_MSS_I2C (Linux bus 2):
    #   0x20-0x22 SFP MOD_ABS, 0x23-0x25 SFP RX_LOS,
    #   0x26 QSFP ModPrsL/IntL, 0x27 QSFP ResetL/LPMode.
    # PCA9555 chips 8-10 are on CP_I2C1 (Linux bus 1): SFP TX_DISABLE.
    sudo modprobe gpio_pca953x

    for addr in 20 21 22 23 24 25 26 27; do
        if [ -d /sys/bus/i2c/devices/i2c-2 ] && [ ! -e "/sys/bus/i2c/devices/2-00${addr}" ]; then
            echo "pca9555 0x${addr}" > /sys/bus/i2c/devices/i2c-2/new_device || true
        fi
    done

    for addr in 20 21 22; do
        if [ -d /sys/bus/i2c/devices/i2c-1 ] && [ ! -e "/sys/bus/i2c/devices/1-00${addr}" ]; then
            echo "pca9555 0x${addr}" > /sys/bus/i2c/devices/i2c-1/new_device || true
        fi
    done

    udevadm settle || true
}

i2c_init()
{
    # Enumerate the system PCA9548 mux on CP_I2C0 (Linux bus 0).
    # This creates virtual buses 3-10 used by PSU, thermals, and FPGA.
    sudo modprobe i2c_mux_pca954x
    if [ -d /sys/bus/i2c/devices/i2c-0 ] && [ ! -e /sys/bus/i2c/devices/0-0070 ]; then
        echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-0/new_device || true
        sleep 1
    fi

    # PSU A: PMBus on bus 3 (PCA9548 channel 0)
    if [ -d /sys/bus/i2c/devices/i2c-3 ] && [ ! -e /sys/bus/i2c/devices/3-0058 ]; then
        echo pmbus 0x58 > /sys/bus/i2c/devices/i2c-3/new_device || true
    fi
    # PSU A: EEPROM on bus 3
    if [ -d /sys/bus/i2c/devices/i2c-3 ] && [ ! -e /sys/bus/i2c/devices/3-0050 ]; then
        echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-3/new_device || true
    fi

    # PSU B: PMBus on bus 4 (PCA9548 channel 1)
    if [ -d /sys/bus/i2c/devices/i2c-4 ] && [ ! -e /sys/bus/i2c/devices/4-0058 ]; then
        echo pmbus 0x58 > /sys/bus/i2c/devices/i2c-4/new_device || true
    fi
    # PSU B: EEPROM on bus 4
    if [ -d /sys/bus/i2c/devices/i2c-4 ] && [ ! -e /sys/bus/i2c/devices/4-0050 ]; then
        echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-4/new_device || true
    fi

    # Temperature sensors on bus 5 (PCA9548 channel 2)
    if [ -d /sys/bus/i2c/devices/i2c-5 ]; then
        [ ! -e /sys/bus/i2c/devices/5-0048 ] && echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-5/new_device || true
        [ ! -e /sys/bus/i2c/devices/5-0049 ] && echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-5/new_device || true
    fi
}

# - Main entry
ln3548xgf-6z-v1_profile
i2c_init
sfp_gpio_init
sfp_mux_init
sfp_init

exit 0

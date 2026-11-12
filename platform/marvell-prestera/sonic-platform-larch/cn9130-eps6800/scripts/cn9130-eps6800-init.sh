#!/bin/bash

# Platform init script for cn9130-eps6800

init_sfp()
{
    BUSES=(
        50 51 52
        55 56 57
        60 61 62
        65 66 67
        70 71 72
        75 76 77
        80 81 82
        85 86 87
        100 101 102
        105 106 107
        110 111 112
        115 116 117
        120 121 122
        125 126 127
        130 131 132
        135 136 137
        53 58 63 68 73 78
    )

    for bus in "${BUSES[@]}"; do
        new_dev="/sys/bus/i2c/devices/i2c-${bus}/new_device"

        echo "optoe2 0x50" > "$new_dev"
    done
}

cn9130-eps6800_profile()
{
    # Read original_platform from machine.conf
    if [ -f /etc/machine.conf ]; then
        original_platform=$(grep -E '^onie_original_platform=' /etc/machine.conf | cut -d'=' -f2)
    elif [ -f /host/machine.conf ]; then
        original_platform=$(grep -E '^onie_original_platform=' /host/machine.conf | cut -d'=' -f2)
    else
        original_platform=""
    fi

    # Apply patch only for specific platform
    if [ "$original_platform" = "arm64-accton_ecs5550_54x-r0" ]; then
        # Patch /etc/fw_env.config
        if [ -w /etc/fw_env.config ]; then
            echo "/dev/mtd1 0x0 0x10000 0x10000" > /etc/fw_env.config
            echo "cn9130-eps6800: Patched /etc/fw_env.config for fw_printenv"
        else
            echo "Warning: Cannot write to /etc/fw_env.config"
        fi
    fi

    MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)

    if [ -z "$MAC_ADDR" ]; then

        echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"

        echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2

        return 0

    fi
    find /usr/share/sonic/device/*cn9130-eps6800* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
    echo "cn9130-eps6800: Updating switch mac address ${MAC_ADDR}"

    modprobe accton-cpld-spi
    modprobe accton-cpld-i2c
    modprobe i2c-wishbone-cpld
    modprobe i2c-mux-accton-cpld

    modprobe lm75
    echo pmbus 0x5b > /sys/bus/i2c/devices/i2c-59/new_device
    echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-59/new_device

    echo pmbus 0x5a > /sys/bus/i2c/devices/i2c-54/new_device
    echo 24c02 0x52 > /sys/bus/i2c/devices/i2c-54/new_device

    init_sfp
}

# - Main entry
cn9130-eps6800_profile

exit 0

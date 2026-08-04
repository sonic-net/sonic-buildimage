#!/bin/bash

### BEGIN INIT INFO
# Provides:          setup-board
# Required-Start:
# Required-Stop:
# Should-Start:
# Should-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Setup S6000 board.
### END INIT INFO


check_speed()
{
    if [ $1 == 1 ];
    then
        echo "2.5GT/s"
    else
        echo "5GT/s"
    fi
}

change_pcie_speed()
{
    echo "---------Change Dell S6000 PCIe link speed-------"
    dev_array=(00\:01.0 01\:00.0
               00\:02.0 02\:00.0)

    speed=$1

    for dev in "${dev_array[@]}"
    do
        if [ ! -e "/sys/bus/pci/devices/$dev" ]; then
            dev="0000:$dev"
        fi

        if [ ! -e "/sys/bus/pci/devices/$dev" ]; then
            echo "Error: device $dev not found"
            return
        fi

        lc=$(setpci -s $dev CAP_EXP+0c.L)
        ls=$(setpci -s $dev CAP_EXP+12.W)
        cur_speed=$(("0x$ls" & 0xF))

        echo "Device:" $dev "Current link speed:" $(check_speed "$cur_speed")

        lc2=$(setpci -s $dev CAP_EXP+30.L)
        lc2n=$(printf "%08x" $((("0x$lc2" & 0xFFFFFFF0) | $speed)))

        setpci -s $dev CAP_EXP+30.L=$lc2n
        lc=$(setpci -s $dev CAP_EXP+10.L)
        lcn=$(printf "%08x" $(("0x$lc" | 0x20)))

        setpci -s $dev CAP_EXP+10.L=$lcn
        sleep 0.1
        ls=$(setpci -s $dev CAP_EXP+12.W)
        link_sp=$(("0x$ls" & 0xF))
        echo "New link speed:" $(check_speed "$link_sp")
    done
}

add_i2c_devices() {

    echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-1/new_device
    echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-1/new_device
    echo dni_dps460 0x58 > /sys/bus/i2c/devices/i2c-1/new_device
    echo dni_dps460 0x59 > /sys/bus/i2c/devices/i2c-1/new_device
    echo jc42 0x18 > /sys/bus/i2c/devices/i2c-10/new_device
    echo emc1403 0x4d > /sys/bus/i2c/devices/i2c-10/new_device
    echo spd 0x50 > /sys/bus/i2c/devices/i2c-10/new_device
    echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-10/new_device
    echo max6620 0x29 > /sys/bus/i2c/devices/i2c-11/new_device
    echo max6620 0x2a > /sys/bus/i2c/devices/i2c-11/new_device
    echo ltc4215 0x40 > /sys/bus/i2c/devices/i2c-11/new_device
    echo ltc4215 0x42 > /sys/bus/i2c/devices/i2c-11/new_device
    echo tmp75 0x4c > /sys/bus/i2c/devices/i2c-11/new_device
    echo tmp75 0x4d > /sys/bus/i2c/devices/i2c-11/new_device
    echo tmp75 0x4e > /sys/bus/i2c/devices/i2c-11/new_device
    echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-11/new_device
    echo 24c02 0x52 > /sys/bus/i2c/devices/i2c-11/new_device
    echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-11/new_device
    for i in `seq 0 31`; do
        echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-$((20+i))/new_device
    done
}

remove_i2c_devices() {
    echo 0x50 > /sys/bus/i2c/devices/i2c-1/delete_device
    echo 0x51 > /sys/bus/i2c/devices/i2c-1/delete_device
    echo 0x58 > /sys/bus/i2c/devices/i2c-1/delete_device
    echo 0x59 > /sys/bus/i2c/devices/i2c-1/delete_device
    echo 0x18 > /sys/bus/i2c/devices/i2c-10/delete_device
    echo 0x4d > /sys/bus/i2c/devices/i2c-10/delete_device
    echo 0x50 > /sys/bus/i2c/devices/i2c-10/delete_device
    echo 0x53 > /sys/bus/i2c/devices/i2c-10/delete_device
    echo 0x29 > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x2a > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x40 > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x42 > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x4c > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x4d > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x4e > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x51 > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x52 > /sys/bus/i2c/devices/i2c-11/delete_device
    echo 0x53 > /sys/bus/i2c/devices/i2c-11/delete_device
    for i in `seq 0 31`; do
        echo 0x50 > /sys/bus/i2c/devices/i2c-$((20+i))/delete_device
    done
}

# Enable/Disable low power mode on all QSFP ports
switch_board_qsfp_lpmode() {
    case $1 in
        "enable")   value=0xffff
                    ;;
        "disable")  value=0x0
                    ;;
        *)          echo "s6000_platform: switch_board_qsfp_lpmode: invalid command $1!"
                    return
                    ;;
    esac
    echo $value > /sys/bus/platform/devices/dell-s6000-cpld.0/qsfp_lpmode
}

set_max6620_dynamic_reg() {
    DEVICE1=/sys/bus/i2c/devices/i2c-11/11-0029/hwmon/hwmon*/fan1_div
    DEVICE2=/sys/bus/i2c/devices/i2c-11/11-002a/hwmon/hwmon*/fan1_div

    # Retry three times
    for count in `seq 1 3`; do
        if [ -w $DEVICE1 -o -w $DEVICE2 ]; then
            for i in `seq 1 4`; do
                echo $1 > /sys/bus/i2c/devices/i2c-11/11-0029/hwmon/hwmon*/fan${i}_div
            done
            for i in `seq 1 2`; do
                echo $1 > /sys/bus/i2c/devices/i2c-11/11-002a/hwmon/hwmon*/fan${i}_div
            done
            return
        fi
        # Sleep for 3 seconds to wait for device tree to be ready
        sleep 3
    done
}

install_python_api_package() {
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
    rv=$(pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl)
}

remove_python_api_package() {
    rv=$(pip3 show sonic-platform > /dev/null 2>/dev/null)
    if [ $? -eq 0 ]; then
        rv=$(pip3 uninstall -y sonic-platform > /dev/null 2>/dev/null)
    fi
}

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

if [ ! -e /etc/sonic/sfp_lock ]; then
    touch /etc/sonic/sfp_lock
fi

if [[ "$1" == "init" ]]; then
        depmod -a
        modprobe nvram
        modprobe i2c_mux_gpio
        modprobe dell_s6000_platform
        install_python_api_package
        #Use 1 for PCIe Gen1, 2 for PCIe Gen2
        change_pcie_speed 1
        add_i2c_devices
        set_max6620_dynamic_reg 4
        /usr/local/bin/set-fan-speed 15000
        switch_board_qsfp_lpmode "disable"
        /usr/local/bin/reset-qsfp
elif [[ "$1" == "deinit" ]]; then
        remove_i2c_devices
        rmmod dell_s6000_platform
        rmmod nvram
        rmmod i2c_mux_gpio
        remove_python_api_package
else
     echo "s6000_platform : Invalid option !"
fi

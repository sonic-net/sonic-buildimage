#!/bin/bash

#platform init script for Dell n3248pxe

source dell_i2c_utils.sh

#Attach/Detach the system devices
sys_devices() {
    case $1 in
        "new_device")    #syseeprom
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         #Attach Fan Controller
                         i2c_config "echo emc2305 0x2c > /sys/bus/i2c/devices/i2c-7/$1"
                         #Attach temperature monitor
                         i2c_config "echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4f > /sys/bus/i2c/devices/i2c-7/$1"
                         #Attach PSU Controller
                         i2c_config "echo dps460 0x5e > /sys/bus/i2c/devices/i2c-10/$1"
                         i2c_config "echo dps460 0x5e > /sys/bus/i2c/devices/i2c-11/$1"
                         #Attach PSU EEPROM
                         i2c_config "echo 24c02 0x56 > /sys/bus/i2c/devices/i2c-10/$1"
                         i2c_config "echo 24c02 0x56 > /sys/bus/i2c/devices/i2c-11/$1"
                         #Attach Fan EEPROM
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-15/$1"
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-16/$1"
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-17/$1"
                         ;;
        "delete_device") i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         i2c_config "echo 0x2c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x49 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4a > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4b > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4f > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x5e > /sys/bus/i2c/devices/i2c-10/$1"
                         i2c_config "echo 0x5e > /sys/bus/i2c/devices/i2c-11/$1"
                         i2c_config "echo 0x56 > /sys/bus/i2c/devices/i2c-10/$1"
                         i2c_config "echo 0x56 > /sys/bus/i2c/devices/i2c-11/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-15/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-16/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-17/$1"
                         ;;
        *)               echo "n3248pxe_platform: main_board_mux : invalid command !"
                         ;;
    esac
}

#Attach/Detach the SFP modules on PCA9548_2
switch_board_sfp() {
    case $1 in
        "new_device")    i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-20/$1"
                         i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-21/$1"
                         i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-22/$1"
                         i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-23/$1"
                         ;;
        "delete_device") i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-20/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-21/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-22/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-23/$1"
                         ;;
        *)               echo "n3248pxe_platform: switch_board_sfp: invalid command !"
                         ;;
    esac
}

#Forcibly bring quad-port phy out of reset for 48-1G port functionality

platform_firmware_versions() {

FIRMWARE_VERSION_FILE=/var/log/firmware_versions
rm -rf ${FIRMWARE_VERSION_FILE}
# Get BIOS version
echo "BIOS: `dmidecode -s system-version `" > $FIRMWARE_VERSION_FILE
# Get CPU CPLD version
echo "CPU CPLD: $((`cat /sys/devices/platform/dell-n3248pxe-cpld.0/cpu_cpld_mjr_ver`)).$((`cat /sys/devices/platform/dell-n3248pxe-cpld.0/cpu_cpld_mnr_ver`))" >> $FIRMWARE_VERSION_FILE
# Get SYS CPLD version
echo "SYS CPLD: $((`cat /sys/devices/platform/dell-n3248pxe-cpld.0/sys_cpld_mjr_ver`)).$((`cat /sys/devices/platform/dell-n3248pxe-cpld.0/sys_cpld_mnr_ver`))" >> $FIRMWARE_VERSION_FILE

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

get_reboot_cause() {
    REBOOT_REASON_FILE="/host/reboot-cause/platform/reboot_reason"
    mkdir -p $(dirname $REBOOT_REASON_FILE)

    # Handle First Boot into software version with reboot cause determination support
    if [[ ! -e $REBOOT_REASON_FILE ]]; then
        echo "0x0" > $REBOOT_REASON_FILE
    else
        cat /sys/devices/platform/dell-n3248pxe-cpld.0/reboot_cause > $REBOOT_REASON_FILE
    fi
}


if [[ "$1" == "init" ]]; then
   modprobe i2c-dev
   modprobe i2c-mux-pca954x
   modprobe pmbus
   modprobe emc2305
   modprobe dps200
   modprobe dell_n3248pxe_platform

   sys_devices "new_device"
   get_reboot_cause
   switch_board_sfp "new_device"
   echo 0xf0 > /sys/devices/platform/dell-n3248pxe-cpld.0/sfp_txdis
   install_python_api_package
   platform_firmware_versions
   echo -2 > /sys/bus/i2c/devices/i2c-0/0-0071/idle_state
elif [[ "$1" == "deinit" ]]; then
    switch_board_sfp "delete_device"
    sysdevices "delete_device"

    modprobe -r dell_n3248pxe_platform
    modprobe -r dps200
    modprobe -r emc2305
    modprobe -r pmbus
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
else
     echo "n3248pxe_platform : Invalid option !"
fi

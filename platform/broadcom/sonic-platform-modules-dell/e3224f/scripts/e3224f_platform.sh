#!/bin/bash

#platform init script for Dell E3224F

source dell_i2c_utils.sh

SONIC_VERSION=$(cat /etc/sonic/sonic_version.yml | grep "build_version" | sed -e "s/build_version: //g;s/'//g")
FIRST_BOOT_FILE="/host/image-${SONIC_VERSION}/platform/firsttime"

#Attach/Detach the system devices
sys_devices() {
    case $1 in
        "new_device")    #syseeprom 
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         #Attach Fan Controller
                         i2c_config "echo emc2305 0x2c > /sys/bus/i2c/devices/i2c-7/$1"
                         #Attach temperature monitor
                         i2c_config "echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo tmp75 0x4f > /sys/bus/i2c/devices/i2c-7/$1"
                         #Attach Fan EEPROM
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-15/$1"
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-16/$1"
                         i2c_config "echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-17/$1"
                         ;;
        "delete_device") i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-2/$1"
                         i2c_config "echo 0x2c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x48 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x49 > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4a > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4b > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4c > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x4f > /sys/bus/i2c/devices/i2c-7/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-15/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-16/$1"
                         i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-17/$1"
                         ;;
        *)               echo "e3224f_platform: main_board_mux : invalid command !"
                         ;;
    esac
}

#Attach/Detach the SFP modules on PCA9548_2
switch_board_sfp() {
    case $1 in
        "new_device")
                         # SFP ports
                         for ((i=27;i<=50;i++));
                         do
                             i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1"
                         done
                         # SFP+ ports
                         for ((i=20;i<=23;i++));
                         do
                             i2c_config "echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1"
                         done
                         # QSFP ports
                         i2c_config "echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-24/$1"
                         i2c_config "echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-25/$1"

                         ;;
        "delete_device")
                         for ((i=20;i<=50;i++));
                         do
                             i2c_config "echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1"
                         done
                         ;;
        "media_down")
                         for ((i=20;i<=23;i++));
                         do
                             # Tx disable for 10G BaseT copper optics
                             eeprom=/sys/bus/i2c/devices/i2c-$i/$i-0050/eeprom

                             # Gen2 or Gen3 copper optics
                             # Check for F10 encoding (starts with '0f10' or 'df10') at offset 96 and 7 byte size
                             # and then compare the 'product id' but skip other part of F10 string
                             f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                             if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29.... ]]; then
                                 cmd="\x01\x00\x09\x00\x01\x02"
                                 echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                             fi
                         done
                         ;;
        *)               echo "e3224f_platform: switch_board_sfp: invalid command !"
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
echo "CPU CPLD:  $((`cat /sys/devices/platform/dell-e3224f-cpld.0/cpu_cpld_mjr_ver`)).$((`cat /sys/devices/platform/dell-e3224f-cpld.0/cpu_cpld_mnr_ver`))" >> $FIRMWARE_VERSION_FILE
# Get SYS CPLD version
echo "SYS CPLD:  $((`cat /sys/devices/platform/dell-e3224f-cpld.0/sys_cpld_mjr_ver`)).$((`cat /sys/devices/platform/dell-e3224f-cpld.0/sys_cpld_mnr_ver`))" >> $FIRMWARE_VERSION_FILE
# Get PORT CPLD version
echo "PORT CPLD: $((`cat /sys/devices/platform/dell-e3224f-cpld.0/port_cpld_mjr_ver`)).$((`cat /sys/devices/platform/dell-e3224f-cpld.0/port_cpld_mnr_ver`))" >> $FIRMWARE_VERSION_FILE

}

install_python_api_package() {
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)

    pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl
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
        cat /sys/devices/platform/dell-e3224f-cpld.0/reboot_cause > $REBOOT_REASON_FILE
    fi
    echo "0x0" > /sys/devices/platform/dell-e3224f-cpld.0/reboot_cause
}


if [[ "$1" == "init" ]]; then
    if [ -f $FIRST_BOOT_FILE ]; then
        systemctl enable system-health.service
        systemctl start --no-block system-health.service
    fi
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe pmbus
    modprobe emc2305
    modprobe dell_e3224f_platform

    sys_devices "new_device"
    get_reboot_cause
    switch_board_sfp "new_device"
    switch_board_sfp "media_down"
    echo 0x00 > /sys/devices/platform/dell-e3224f-cpld.0/sfp_txdis
    echo 0xf0 > /sys/devices/platform/dell-e3224f-cpld.0/sfpplus_txdis
    echo 0xf3 > /sys/devices/platform/dell-e3224f-cpld.0/qsfp_rst
    echo 0x00 > /sys/devices/platform/dell-e3224f-cpld.0/qsfp_lpmode
    install_python_api_package
    platform_firmware_versions
elif [[ "$1" == "deinit" ]]; then
    switch_board_sfp "media_down"
    switch_board_sfp "delete_device"
    sys_devices "delete_device"

    modprobe -r dell_e3224f_platform

    modprobe -r emc2305
    modprobe -r pmbus
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
elif [ "$1" == "media_down" ]; then
    switch_board_sfp $1
else
     echo "e3224f_platform : Invalid option !"
fi

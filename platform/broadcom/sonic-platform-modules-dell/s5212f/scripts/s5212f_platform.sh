#!/bin/bash

init_devnum() {
    found=0
    for devnum in 0 1; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # iSMT adapter can be at either dffd0000 or dfff0000
        if [[ $devname == 'SMBus iSMT adapter at '* ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "cannot find iSMT" && exit 1
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    echo 24c16 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        "delete_device") echo 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        *)               echo "s5212f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_mux() {
    case $1 in
        "new_device")
                      for ((i=603;i<=604;i++));
                      do
                          echo "Attaching PCA9548 @ 0x74"
                          echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
                      done

                      ;;
        "delete_device")
                      for ((i=603;i<=604;i++));
                      do
                          echo "Detaching PCA9548 @ 0x74"
                          echo 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
                      done

                      ;;
        *)            echo "s5212f_platform: switch_board_qsfp_mux: invalid command !"
                      ;;
    esac
    sleep 2
}

#Attach/Detach 64 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
        case $1 in
        "new_device")
                        for ((i=14;i<=16;i++));
                        do
                            echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=14;i<=16;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "s5212f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

#Attach/Detach 2 instances of EEPROM driver SFP+ ports. Use optoe2 (dual address devices)
#eeprom can dump data using below command
switch_board_sfp() {
    case $1 in
        "new_device")
            for ((i=2;i<=13;i++));
            do
                echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;

        "delete_device")
            for ((i=2;i<=13;i++));
            do
                echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;

        *)              echo "s5212f_platform: switch_board_sfp: invalid command !"
            ;;
    esac
}

#Modsel 3 ports to applicable QSFP type modules
#This enables the adapter to respond for i2c commands
switch_board_modsel() {
	resource="/sys/bus/pci/devices/0000:03:00.0/resource0"
	for ((i=1;i<=15;i++));
	do
		port_addr=$(( 16384 + ((i - 1) * 16)))
		hex=$( printf "0x%x" $port_addr )
		python /usr/bin/pcisysfs.py --set --offset $hex --val 0x10 --res $resource  > /dev/null 2>&1
	done
}

#This enables the led control for CPU and default states 
switch_board_led_default() {
	resource="/sys/bus/pci/devices/0000:03:00.0/resource0"
	python /usr/bin/pcisysfs.py --set --offset 0x24 --val 0x194 --res $resource  > /dev/null 2>&1
}

# Readout firmware version of the system and
# store in /var/log/firmware_versions
platform_firmware_versions() {
       FIRMWARE_VERSION_FILE=/var/log/firmware_versions
       rm -rf ${FIRMWARE_VERSION_FILE}
       echo "BIOS: `dmidecode -s system-version `" > $FIRMWARE_VERSION_FILE
       ## Get FPGA version
       r=`/usr/bin/pcisysfs.py  --get --offset 0x00 --res /sys/bus/pci/devices/0000\:03\:00.0/resource0 | sed  '1d; s/.*\(....\)$/\1/; s/\(..\{1\}\)/\1./'`
       r_min=$(echo $r | sed 's/.*\(..\)$/0x\1/')
       r_maj=$(echo $r | sed 's/^\(..\).*/0x\1/')
       echo "FPGA: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

       ## Get BMC Firmware Revision
       r=`cat /sys/class/ipmi/ipmi0/device/bmc/firmware_revision`
       echo "BMC: $r" >> $FIRMWARE_VERSION_FILE

       #System CPLD 0x31 on i2c bus 601 ( physical FPGA I2C-2)
       r_min=`/usr/sbin/i2cget -y 601 0x31 0x0 | sed ' s/.*\(0x..\)$/\1/'`
       r_maj=`/usr/sbin/i2cget -y 601 0x31 0x1 | sed ' s/.*\(0x..\)$/\1/'`
       echo "System CPLD: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

       #Slave CPLD 1 0x30 on i2c bus 600 ( physical FPGA I2C-1)
       r_min=`/usr/sbin/i2cget -y 600 0x30 0x0 | sed ' s/.*\(0x..\)$/\1/'`
       r_maj=`/usr/sbin/i2cget -y 600 0x30 0x1 | sed ' s/.*\(0x..\)$/\1/'`
       echo "Slave CPLD 1: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE
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
    resource="/sys/bus/pci/devices/0000:03:00.0/resource0"

    mkdir -p $(dirname $REBOOT_REASON_FILE)

    # Handle First Boot into software version with reboot cause determination support
    if [[ ! -e $REBOOT_REASON_FILE ]]; then
        echo "0" > $REBOOT_REASON_FILE
    else
        /usr/bin/pcisysfs.py --get --offset 0x18 --res $resource | sed '1d; s/.*:\(.*\)$/\1/;' > $REBOOT_REASON_FILE
    fi
    /usr/bin/pcisysfs.py --set --val 0x0 --offset 0x18 --res $resource
}

init_devnum

if [ "$1" == "init" ]; then
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si kipmid_max_busy_us=1000
    modprobe i2c_ocores
    modprobe dell_s5212f_fpga_ocores
    sys_eeprom "new_device"
    get_reboot_cause
    switch_board_qsfp_mux "new_device"
    switch_board_qsfp "new_device"
    switch_board_sfp "new_device"
    switch_board_modsel
    switch_board_led_default
    install_python_api_package
    platform_firmware_versions
    echo -2 > /sys/bus/i2c/drivers/pca954x/603-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/604-0074/idle_state
elif [ "$1" == "deinit" ]; then
    sys_eeprom "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_sfp "delete_device"
    switch_board_qsfp_mux "delete_device"

    modprobe -r dell_s5212f_fpga_ocores
    modprobe -r i2c_ocores
    modprobe -r acpi_ipmi
    modprobe -r ipmi_si
    modprobe -r ipmi_devintf
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
else
     echo "s5212f_platform : Invalid option !"
fi

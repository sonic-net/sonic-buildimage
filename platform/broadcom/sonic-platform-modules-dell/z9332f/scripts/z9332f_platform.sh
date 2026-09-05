#!/bin/bash

init_devnum() {
    found=0
    for devnum in 0; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # I801 adapter f000
        if [[ $devname == 'SMBus I801 adapter at '* ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "cannot find I801" && exit 1
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    echo 24lc64t 0x56 > /sys/bus/i2c/devices/i2c-0/$1
                         ;;
        "delete_device") echo 0x56 > /sys/bus/i2c/devices/i2c-0/$1
                         ;;
        *)               echo "z9332f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_mux() {
    case $1 in
        "new_device")
                      for ((i=603;i<=607;i++));
                      do
                          echo "Attaching PCA9548 @ 0x74"
                          echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
                      done

                      ;;
        "delete_device")
                      for ((i=603;i<=607;i++));
                      do
                          echo "Detaching PCA9548 @ 0x74"
                          echo 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
                      done

                      ;;
        *)            echo "z9332f_platform: switch_board_qsfp_mux: invalid command !"
                      ;;
    esac
    sleep 2
}

#Attach/Detach 64 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
        case $1 in
        "new_device")
                        for ((i=10;i<=41;i++));
                        do
                            echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=10;i<=41;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "z9332f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

#Attach/Detach 2 instances of EEPROM driver SFP+ ports
#eeprom can dump data using below command
switch_board_sfp() {
        case $1 in
        "new_device")
                        for ((i=1;i<=2;i++));
                        do
                            echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=1;i<=2;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "z9332f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

#Modsel 64 ports to applicable QSFP type modules
#This enables the adapter to respond for i2c commands
#Low Power mode enabled by default
switch_board_modsel() {
	resource="/sys/bus/pci/devices/0000:09:00.0/resource0"
	for ((i=1;i<=32;i++));
	do
		port_addr=$(( 16384 + ((i - 1) * 16)))
		hex=$( printf "0x%x" $port_addr )
		/usr/bin/pcisysfs.py --set --offset $hex --val 0x50 --res $resource  > /dev/null 2>&1
	done
}

#This enables the led control for CPU and default states 
switch_board_led_default() {
	/usr/sbin/i2cset -y 5 0x0d 0x62 0xd0
}

# Readout firmware version of the system and
# store in /var/log/firmware_versions
platform_firmware_versions() {

	FIRMWARE_VERSION_FILE=/var/log/firmware_versions

	rm -rf ${FIRMWARE_VERSION_FILE}
	echo "BIOS:`dmidecode -t bios | grep   Version |  awk -F":" '{print $2}'`" > $FIRMWARE_VERSION_FILE
	# Get FPGA version
	r=`/usr/bin/pcisysfs.py  --get --offset 0x00 --res /sys/bus/pci/devices/0000\:09\:00.0/resource0 | sed  '1d; s/.*\(....\)$/\1/; s/\(..\{1\}\)/\1./'`
	r_min=$(echo $r | sed 's/.*\(..\)$/0x\1/')
	r_maj=$(echo $r | sed 's/^\(..\).*/0x\1/')
	echo "FPGA: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	## Get BMC Firmware Revision
	#r=`docker exec -it pmon ipmitool mc info | awk '/Firmware Revision/ { print $NF }'`
	r=`cat /sys/class/ipmi/ipmi0/device/bmc/firmware_revision`
	echo "BMC: $r" >> $FIRMWARE_VERSION_FILE

	#BaseBoard CPLD 0x0d on i2c bus 5 ( physical FPGA I2C-5)
	ver=`/usr/sbin/i2cget -y 5 0x0d 0x0`
	echo "Baseboard CPLD: $((ver))" >> $FIRMWARE_VERSION_FILE

	#Switch CPLD 1 0x30 on i2c bus 4 ( physical FPGA I2C-4)
	ver=`/usr/sbin/i2cget -y 4 0x30 0x0`
	echo "Switch CPLD 1: $((ver))" >> $FIRMWARE_VERSION_FILE

	#Switch CPLD 1 0x30 on i2c bus 4 ( physical FPGA I2C-4)
	ver=`/usr/sbin/i2cget -y 4 0x31 0x0`
	echo "Switch CPLD 2: $((ver))" >> $FIRMWARE_VERSION_FILE
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
        echo "0" > $REBOOT_REASON_FILE
    else
        /usr/sbin/i2cget -y 5 0x0d 0x06  > $REBOOT_REASON_FILE
    fi
}


init_devnum

if [ "$1" == "init" ]; then
    modprobe i2c-dev
    modprobe cls-i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si kipmid_max_busy_us=2500
    modprobe cls-i2c-ocore
    modprobe cls-switchboard 
    modprobe mc24lc64t 
    #insmod /lib/modules/`uname -r`/extra/mc24lc64t.ko
    sys_eeprom "new_device"
    switch_board_qsfp "new_device"
    switch_board_sfp "new_device"
    switch_board_led_default
    install_python_api_package
  # /usr/bin/qsfp_irq_enable.py
    platform_firmware_versions
    get_reboot_cause
    echo 2500 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us
    # Set the PCA9548 mux behavior
    echo -2 > /sys/bus/i2c/drivers/cls_pca954x/3-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/cls_pca954x/3-0071/idle_state
    echo -2 > /sys/bus/i2c/drivers/cls_pca954x/3-0072/idle_state
    echo -2 > /sys/bus/i2c/drivers/cls_pca954x/3-0073/idle_state

elif [ "$1" == "deinit" ]; then
    sys_eeprom "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_sfp "delete_device"

    modprobe -r mc24lc64t
    modprobe -r cls-switchboard
    modprobe -r cls-i2c-ocore
    modprobe -r acpi_ipmi
    modprobe -r ipmi_si
    modprobe -r ipmi_devintf
    modprobe -r cls-i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
else
     echo "z9332f_platform : Invalid option !"
fi


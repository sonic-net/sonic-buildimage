#!/bin/bash

init_devnum() {
    found=0
    for devnum in 0 1; do
        devname=`cat /sys/bus/i2c/devices/i2c-${devnum}/name`
        # I801 adapter 
        if [[ $devname == 'SMBus I801 adapter at '* ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "cannot find I801" && exit 1
}

# Function to detect pci bus number for FPGA 

init_fpga_busnum() {
    found=0
    for fpga_busnum in 1 2; do
        fpga_vendorid=`cat /sys/bus/pci/devices/0000:0$fpga_busnum:00.0/subsystem_vendor`
        if [[ $fpga_vendorid == '0x1204' ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "Cannot read pcie device bus for fpga" && exit 1
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    echo 24c08 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        "delete_device") echo 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        *)               echo "z9864f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_sfp_mux() {
    case $1 in
        "new_device")
                        #OSFP 1 to 32 I2C mux bus # 602,603,604,605
                        #OSFP 33 to 64 I2C mux bus # 606,607,608,609
                        #SFP+ port  I2C mux bus # 601
                        for i in 602 603 604 605 606 607 608 609 601
                        do
                           sleep 0.5
                           if [[ $i == 601 ]]; then
                               echo "Attaching PCA9546 @ 0x71"
                               echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                               sleep 0.1
                               if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0071/channel-0 ]; then
                                   echo 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                                   sleep 3
                                   echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                               fi
                               break
                           fi
                           echo "Attaching PCA9548 @ 0x70"
                           echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                           sleep 0.2
                           if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0070/channel-0 ]; then
                               echo 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                               sleep 3
                               echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                           fi
                        done
                        sleep 1

                        for i in 602 603 604 605 606 607 608 609 601
                        do
                           if [[ $i == 601 ]]; then
                               if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0071/channel-0 ]; then
                                   logger -p syslog.err "*******H/W Error : i2c mux $i failed to populate******"
                               fi
                               break
                           fi
                           if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0070/channel-0 ]; then
                               logger -p syslog.err "*******H/W Error : i2c mux $i failed to populate******"
                           fi
                        done
                        ;;

        "delete_device")
                        #OSFP 1 to 32 I2C mux bus # 602,603,604,605
                        #OSFP 33 to 64 I2C mux bus # 606,607,608,609
                        #SFP+ port  I2C mux bus # 601
                        for i in 602 603 604 605 606 607 608 609 601
                        do
                           if [[ $i == 601 ]]; then
                               echo "Detaching PCA95468 @ 0x71"
                               echo 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                           fi
                           echo "Detaching PCA9548 @ 0x70"
                           echo 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
        *)              echo "z9864f_platform: switch_board_qsfp_sfp_mux: invalid command !"
                        ;;
    esac
    sleep 2
}

#Attach/Detach 2 instances of EEPROM driver SFP+ ports
#eeprom can dump data using below command
switch_board_sfp() {
        case $1 in
        "new_device")
                        for ((i=66;i<=67;i++));
                        do
                            echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=66;i<=67;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "z9864f_platform: switch_board_sfp: invalid command !"
                        ;;
    esac
}

#Attach/Detach 32 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
        case $1 in
        "new_device")
                        /usr/bin/python3 /usr/local/bin/load_optics_driver.py "02:65" "optoe3"
                        ;;
 
        "delete_device")
                        for ((i=2;i<=65;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "z9864f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

BOOT_TYPE=""
get_boot_type()
{
    file="/proc/cmdline"
    sub="SONIC_BOOT_TYPE"
    while read -r line; do
        if grep -q "$sub" <<< "$line"; then
            for word in $line
            do
                if [[ $word == SONIC_BOOT_TYPE* ]]; then
                    BOOT_TYPE="$(cut -d'=' -f2 <<<"$word")"
                fi
            done
        fi
    done <$file
}

switch_board_module_reset() {
    # setting low power mode to support 100G QSFP via adapter
    /usr/sbin/i2cset -y 600 0x41 0x44 0x00
    /usr/sbin/i2cset -y 600 0x41 0x45 0x00
    /usr/sbin/i2cset -y 600 0x41 0x46 0x00
    /usr/sbin/i2cset -y 600 0x41 0x47 0x00
    /usr/sbin/i2cset -y 600 0x45 0x44 0x00
    /usr/sbin/i2cset -y 600 0x45 0x45 0x00
    /usr/sbin/i2cset -y 600 0x45 0x46 0x00
    /usr/sbin/i2cset -y 600 0x45 0x47 0x00

    # set reset bits
    /usr/sbin/i2cset -y 600 0x41 0x40 0xff
    /usr/sbin/i2cset -y 600 0x41 0x41 0xff
    /usr/sbin/i2cset -y 600 0x41 0x42 0xff
    /usr/sbin/i2cset -y 600 0x41 0x43 0xff
    /usr/sbin/i2cset -y 600 0x45 0x40 0xff
    /usr/sbin/i2cset -y 600 0x45 0x41 0xff
    /usr/sbin/i2cset -y 600 0x45 0x42 0xff
    /usr/sbin/i2cset -y 600 0x45 0x43 0xff
    /usr/sbin/i2cset -y 600 0x41 0x50 0x00
    echo "z9864f_platform: switch_board_module_reset"
}

switch_board_modenable() {
	#enable reset logic for i2c
	/usr/sbin/i2cset -y 600 0x41 0x40 0x00
	/usr/sbin/i2cset -y 600 0x41 0x41 0x00
	/usr/sbin/i2cset -y 600 0x41 0x42 0x00
	/usr/sbin/i2cset -y 600 0x41 0x43 0x00
	/usr/sbin/i2cset -y 600 0x45 0x40 0x00
	/usr/sbin/i2cset -y 600 0x45 0x41 0x00
	/usr/sbin/i2cset -y 600 0x45 0x42 0x00
	/usr/sbin/i2cset -y 600 0x45 0x43 0x00
        /usr/sbin/i2cset -y 600 0x41 0x50 0x00

}

platform_firmware_versions() {
	FIRMWARE_VERSION_FILE=/var/log/firmware_versions
	rm -rf ${FIRMWARE_VERSION_FILE}

	#echo "BIOS: `dmidecode -s system-version `" > $FIRMWARE_VERSION_FILE
	echo "BIOS: `dmidecode -s bios-version`" > $FIRMWARE_VERSION_FILE

	## Get FPGA version, Only Maj version available
	r=`/usr/bin/pcisysfs.py  --get --offset 0x04 --res /sys/bus/pci/devices/0000\:0$fpga_busnum\:00.0/resource0 | sed  '1d; s/.*\(....\)$/\1/;'`
	r_maj=$(echo $r | sed 's/.*\(..\)$/0x\1/')
	r_min=0
	echo "FPGA: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	## Get BMC Firmware Revision
	r=`ipmitool mc info | awk '/Firmware Revision/ { print $NF }'`
	echo "BMC: $r" >> $FIRMWARE_VERSION_FILE

	#System CPLD 0x3d on i2c bus 1 ( physical FPGA I2C-1)
	r_maj=`/usr/sbin/i2cget -y $devnum 0x3d 0x1 | sed ' s/.*\(.\)$/\1/'`
	r_min=0
	echo "System CPLD: $r_maj.$((r_min))" >> $FIRMWARE_VERSION_FILE

	#PortA CPLD 1 0x41 on i2c bus 600 ( physical FPGA I2C-600)
	r_maj=`/usr/sbin/i2cget -y 600 0x41 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_min=0
	echo "Secondary CPLD 1: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#PortB CPLD 2 0x45 on i2c bus 600 ( physical FPGA I2C-600)
	r_maj=`/usr/sbin/i2cget -y 600 0x45 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_min=0
	echo "Secondary CPLD 2: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE
}

#This enables the led control for CPU and default states 
switch_board_led_default() {
	resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
	python /usr/bin/pcisysfs.py --set --offset 0x24 --val 0x94 --res $resource  > /dev/null 2>&1
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
        /usr/sbin/i2cget -y $devnum 0x3d 0x28 > $REBOOT_REASON_FILE
    fi
    /usr/sbin/i2cset -y $devnum 0x3d 0x28 0x0
}

init_devnum
init_fpga_busnum
PLATFORM_READY_CHECK=/var/run/platform_ready

if [ "$1" == "init" ]; then
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si kipmid_max_busy_us=3000
    modprobe dell_z9864f_fpga
    sys_eeprom "new_device"
    sleep 1
    # Create a flag file to denote the platform is ready
    touch $PLATFORM_READY_CHECK
    switch_board_qsfp_sfp_mux "new_device"
    switch_board_qsfp "new_device"
    switch_board_sfp "new_device"
    sleep 1
    get_reboot_cause
    get_boot_type
    switch_board_module_reset
    sleep 1
    switch_board_modenable
    sleep 1
    #switch_board_led_default
    install_python_api_package
    platform_firmware_versions
    sleep 1
    echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us
    echo -2 > /sys/bus/i2c/drivers/pca954x/603-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/605-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/607-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/602-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/604-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/606-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/608-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/609-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/601-0071/idle_state

elif [ "$1" == "deinit" ]; then
    if [ ! -f "/tmp/warm-reboot-progress" ]; then
        switch_board_module_reset
    fi
    # Remove the flag file to denote the platform is no more ready
    rm -f $PLATFORM_READY_CHECK
elif [ "$1" == "media_down" ]; then
    switch_board_module_reset
else
    echo "z9864f_platform : Invalid option !"
fi


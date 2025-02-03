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
        if [[ $fpga_vendorid == '0x1172' ]]; then
            found=1
            break
        fi
    done

    [ $found -eq 0 ] && echo "Cannot read pcie device bus for fpga" && exit 1
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    echo 24c16 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        "delete_device") echo 0x50 > /sys/bus/i2c/devices/i2c-${devnum}/$1
                         ;;
        *)               echo "z9664f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_sfp_mux() {
    case $1 in
        "new_device")
                        #QSFP56DD 1 to 32 I2C mux bus # 601,603,605,607
                        #QSFP56DD 33 to 64 I2C mux bus # 602,604,606,608
                        #SFP+ port  I2C mux bus # 609
                        for i in 601 603 605 607 602 604 606 608 609
                        do
                           sleep 0.5
                           if [[ $i == 609 ]]; then
                               echo "Attaching PCA9546 @ 0x71"
                               echo pca9546 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                               sleep 0.1
                               if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0071/channel-0 ]; then
                                   echo 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
                                   sleep 3
                                   echo pca9546 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
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

                        for i in 601 603 605 607 602 604 606 608 609
                        do
                           if [[ $i == 609 ]]; then
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
                        #QSFP56DD 1 to 32 I2C mux bus # 601,603,605,607
                        #QSFP56DD 33 to 64 I2C mux bus # 602,604,606,608
                        #SFP+ port  I2C mux bus # 609
                        #QSFP56DD 1 to 32
                        for i in 601 603 605 607 602 604 606 608 609
                        do
                           if [[ $i == 609 ]]; then
                               echo "Detaching PCA9546 @ 0x71"
                               echo 0x71 > /sys/bus/i2c/devices/i2c-$i/$1
			       break
                           fi
                           echo "Detaching PCA9548 @ 0x70"
                           echo 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
        *)              echo "z9664f_platform: switch_board_qsfp_sfp_mux: invalid command !"
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

        "media_down")
                        resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
                        for ((i=66;i<=67;i++));
                        do
                            port_addr=$(( 16388 + ((i-2)*16)))
                            hex=$( printf "0x%x" $port_addr )
                            not_present=$(( `/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2` & 1))
                            if [ "$not_present" -ne 1 ]; then
                                # Tx disable for 10G BaseT copper optics
                                eeprom=/sys/bus/i2c/devices/i2c-$i/$i-0050/eeprom

                                # Gen2 or Gen3 copper optics
                                # Check for F10 encoding (starts with '0f10' or 'df10') at offset 96 and 7 byte size
                                # and then compare the 'product id' but skip other part of F10 string
                                f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                    cmd="\x01\x00\x09\x00\x01\x02"
                                    echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    port[$i-1]=1
                                fi
                            fi
                        done

			# During shutdown/reboot check the SFP Optics with QSA adapter
			if [[ $2 == "shut_down" ]]; then
                            for ((i=2;i<=65;i++));
                            do
                                port_addr=$(( 16388 + ((i-2)*16)))
                                hex=$( printf "0x%x" $port_addr )
                                not_present=$(( $((16#$((`/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2`)))) & 0x10))
                                if [ "$not_present" == 0 ]; then
                                    eeprom=/sys/bus/i2c/devices/i2c-$i/$i-0050/eeprom
                                    f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                    if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                        cmd="\x01\x00\x09\x00\x01\x02"
                                        echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    fi
                                fi
                            done
                        fi
                        ;;

        *)              echo "z9664f_platform: switch_board_sfp: invalid command !"
                        ;;
    esac
}

#Attach/Detach 32 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
        case $1 in
        "new_device")
			for ((i=2;i<=65;i++));
			do 
			    echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
			done
                        ;;
 
        "delete_device")
                        for ((i=2;i<=65;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "z9664f_platform: switch_board_qsfp: invalid command !"
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
    resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
    for ((i=1;i<=64;i++));
    do
        port_addr=$(( 16384 + ((i - 1) * 16)))
        hex=$( printf "0x%x" $port_addr )
        python /usr/bin/pcisysfs.py --set --offset $hex --val 0x47 --res $resource  > /dev/null 2>&1
    done
}

#Modsel 64 ports to applicable QSFP type modules
#This enables the adapter to respond for i2c commands
switch_board_modsel() {
        get_boot_type
        if [[ ${BOOT_TYPE} == "fast-reboot" ]]; then
                switch_board_module_reset
        fi
	resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
	for ((i=1;i<=64;i++));
	do
		port_addr=$(( 16384 + ((i - 1) * 16)))
		hex=$( printf "0x%x" $port_addr )
		python /usr/bin/pcisysfs.py --set --offset $hex --val 0x51 --res $resource  > /dev/null 2>&1
	done
	for ((i=65;i<=66;i++));
	do
		port_addr=$(( 16384 + ((i - 1) * 16)))
		hex=$( printf "0x%x" $port_addr )
		if [ ${port[$i]} ]
		then
		    val=0x51
		else
		    val=0x50
		fi
		python /usr/bin/pcisysfs.py --set --offset $hex --val $val --res $resource  > /dev/null 2>&1
	done
}

platform_firmware_versions() {
	FIRMWARE_VERSION_FILE=/var/log/firmware_versions
	rm -rf ${FIRMWARE_VERSION_FILE}
	echo "BIOS: `dmidecode -s system-version `" > $FIRMWARE_VERSION_FILE
	## Get FPGA version
	r=`/usr/bin/pcisysfs.py  --get --offset 0x00 --res /sys/bus/pci/devices/0000\:0$fpga_busnum\:00.0/resource0 | sed  '1d; s/.*\(....\)$/\1/; s/\(..\{1\}\)/\1./'`
	r_min=$(echo $r | sed 's/.*\(..\)$/0x\1/')
	r_maj=$(echo $r | sed 's/^\(..\).*/0x\1/')
	echo "FPGA: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	## Get BMC Firmware Revision
	r=`ipmitool mc info | awk '/Firmware Revision/ { print $NF }'`
	echo "BMC: $r" >> $FIRMWARE_VERSION_FILE

	#System CPLD 0x31 on i2c bus 600 ( physical FPGA I2C-2)
	r_min=`/usr/sbin/i2cget -y 600 0x38 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x38 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "System CPLD: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#Secondary CPLD 1 0x30 on i2c bus 600 ( physical FPGA I2C-1)
	r_min=`/usr/sbin/i2cget -y 600 0x30 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x30 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "Secondary CPLD 1: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#Secondary CPLD 2 0x31 on i2c bus 600 ( physical FPGA I2C-1)
	r_min=`/usr/sbin/i2cget -y 600 0x31 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x31 0x1 | sed ' s/.*\(0x..\)$/\1/'`
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

    pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl --force-reinstall -q --root-user-action=ignore
}

remove_python_api_package() {
    rv=$(pip3 show sonic-platform > /dev/null 2>/dev/null)
    if [ $? -eq 0 ]; then
        rv=$(pip3 uninstall -y sonic-platform > /dev/null 2>/dev/null)
    fi
}

get_reboot_cause() {
    REBOOT_REASON_FILE="/host/reboot-cause/platform/reboot_reason"
    resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"

    mkdir -p $(dirname $REBOOT_REASON_FILE)

    # Handle First Boot into software version with reboot cause determination support
    if [[ ! -e $REBOOT_REASON_FILE ]]; then
        echo "0" > $REBOOT_REASON_FILE
    else
        /usr/bin/pcisysfs.py --get --offset 0x18 --res $resource | sed '1d; s/.*:\(.*\)$/\1/;' > $REBOOT_REASON_FILE
    fi
    /usr/bin/pcisysfs.py --set --val 0x0 --offset 0x18 --res $resource
}

# Disable FPGA port scan
switch_fpga_port_scan_disable() {
	resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
	python /usr/bin/pcisysfs.py --set --offset 0x8000 --val 0x10200  --res $resource  > /dev/null 2>&1
}

init_devnum
init_fpga_busnum

if [ "$1" == "init" ]; then
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si kipmid_max_busy_us=3000
    modprobe i2c_ocores
    modprobe dell_z9664f_fpga_ocores
    modprobe mc24lc64t
    sys_eeprom "new_device"
    sleep 1
    switch_fpga_port_scan_disable
    sleep 1
    switch_board_qsfp_sfp_mux "new_device"
    switch_board_qsfp "new_device"
    switch_board_sfp "new_device"
    switch_board_sfp "media_down"
    switch_board_modsel
    switch_board_led_default
    install_python_api_package
    platform_firmware_versions
    get_reboot_cause
    echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us
    echo -2 > /sys/bus/i2c/drivers/pca954x/601-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/603-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/605-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/607-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/602-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/604-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/606-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/608-0070/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/609-0071/idle_state

elif [ "$1" == "deinit" ]; then
    sys_eeprom "delete_device"
    switch_board_sfp "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_qsfp_sfp_mux "delete_device"
    modprobe -r  mc24lc64t
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
elif [ "$1" == "media_down" ]; then
    switch_board_sfp $1 "shut_down"
else
    echo "z9664f_platform : Invalid option !"
fi


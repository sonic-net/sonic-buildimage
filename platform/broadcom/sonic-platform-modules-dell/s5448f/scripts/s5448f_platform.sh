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
        *)               echo "s5448f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_sfp_mux() {
    case $1 in
        "new_device")
                         for ((i=603;i<=611;i++));
                         do
                            if [[ $i == 610 ]]; then
                                continue
                            fi
                            sleep 0.1
                            echo "Attaching PCA9548 @ 0x70"
                            echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                         done
                         sleep 1
                         for ((i=603;i<=611;i++));
                         do
                            if [[ $i == 610 ]]; then
                                continue
                            fi
                            if [ ! -d /sys/bus/i2c/devices/i2c-$i/$i-0070/channel-0 ]; then
                                logger -p syslog.err "*******H/W Error : i2c mux $i failed to populate******"
                            fi
                         done
                         ;;
        "delete_device")
                        for ((i=603;i<=611;i++));
                        do
                        if [[ $i == 610 ]]; then
                            continue
                        fi
                          echo "Detaching PCA9548 @ 0x70"
                          echo 0x70 > /sys/bus/i2c/devices/i2c-$i/$1
                        done

                      ;;
        *)            echo "s5448f_platform: switch_board_qsfp_sfp_mux: invalid command !"
                      ;;
    esac
    sleep 2
}

#Attach/Detach 50 instances of EEPROM driver SFP+/SFPDD ports
#eeprom can dump data using below command
switch_board_sfp() {
        case $1 in
        "new_device")
                        for ((i=10;i<=57;i++));
                        do
                           echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        for ((i=58;i<=59;i++));
                        do
                            echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=10;i<=57;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        for ((i=58;i<=59;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        "media_down")
                        # Tx disable for 10G BaseT copper optics
                        # Gen2 or Gen3 copper optics
                        # Check for F10 encoding (starts with '0f10' or 'df10') at offset 96 and 7 byte size
                        # and then compare the 'product id' but skip other part of F10 string

                        # eeprom path of optics is not sequence in s5448f platform
                        # /sys/bus/i2c/devices/i2c-<addr>/<addr>-0050/eeprom
                        # i2c address of the 58 ports
                        i2c_map=(0 8 6 9 7 18 20 19 21 22 24 23 25 26 28 27 29 30 32 31 33
                                 10 12 11 13 14 16 15 17 50 52 51 53 54 56 55 57 42 44 43
                                 45 46 48 47 49 34 36 35 37 38 40 39 41 3 5 2 4 59 58)

                        resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
                        for ((i=5,j=0;i<=52;i++,j++));
                        do
                            port_addr=$(( 16388 + ((i-3)*32)))
                            hex=$( printf "0x%x" $port_addr )
                            val=`/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2`
                            hex_val=$( printf "%x" 0x`echo -n $val`)
                            not_present=$(( 16#$hex_val & 1))
                            if [ "$not_present" -ne 1 ]; then
                                eeprom=/sys/bus/i2c/devices/i2c-${i2c_map[$i]}/${i2c_map[$i]}-0050/eeprom
                                f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                    cmd="\x01\x00\x09\x00\x01\x02"
                                    echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    port[$i+$j]=1
                                fi
                            fi
                        done
                        for ((i=57;i<=58;i++));
                        do
                            port_addr=$(( 16388 + ((i+47)*16)))
                            hex=$( printf "0x%x" $port_addr )
                            val=`/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2`
                            hex_val=$( printf "%x" 0x`echo -n $val`)
                            not_present=$(( 16#$hex_val & 1))
                            if [ "$not_present" -ne 1 ]; then
                                eeprom=/sys/bus/i2c/devices/i2c-${i2c_map[$i]}/${i2c_map[$i]}-0050/eeprom
                                f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                    cmd="\x01\x00\x09\x00\x01\x02"
                                    echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    port[48+$i]=1
                                fi
                            fi
                        done

                        # During shutdown/reboot check the SFP Optics with QSA adapter
                        if [[ $2 == "shut_down" ]]; then
                            for ((i=1;i<=4;i++));
                            do
                                port_addr=$(( 16388 + ((i-1)*16)))
                                hex=$( printf "0x%x" $port_addr )
                                not_present=$(( $((16#$((`/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2`)))) & 0x10))
                                if [ "$not_present" == 0 ]; then
                                    eeprom=/sys/bus/i2c/devices/i2c-${i2c_map[$i]}/${i2c_map[$i]}-0050/eeprom
                                    f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                    if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                        cmd="\x01\x00\x09\x00\x01\x02"
                                        echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    fi
                                fi
                            done
                            for ((i=53;i<=56;i++));
                            do
                                port_addr=$(( 16388 + ((i+47)*16)))
                                hex=$( printf "0x%x" $port_addr )
                                not_present=$(( $((16#$((`/usr/bin/pcisysfs.py --get --offset $hex --res $resource | cut -d : -f 2`)))) & 0x10))
                                if [ "$not_present" == 0 ]; then
                                    eeprom=/sys/bus/i2c/devices/i2c-${i2c_map[$i]}/${i2c_map[$i]}-0050/eeprom
                                    f10_encoding=`hexdump -n7 -s96 $eeprom -e'7/1 "%02x"' 2>&1`
                                    if [[ $f10_encoding =~ ^[0d]f10....28....|^[0d]f10....29....|^[0d]f10..1111.... ]]; then
                                        cmd="\x01\x00\x09\x00\x01\x02"
                                        echo -n -e $cmd | dd bs=1 count=6 of=$eeprom seek=506 obs=1 status=none
                                    fi
                                fi
                            done
                        fi

                        ;;

        *)              echo "z9332f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

#Attach/Detach 8 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
        case $1 in
        "new_device")
                        for ((i=2;i<=9;i++));
                        do
                            echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;
 
        "delete_device")
                        for ((i=2;i<=9;i++));
                        do
                            echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
                        done
                        ;;

        *)              echo "s5448f_platform: switch_board_qsfp: invalid command !"
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
    resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
    for ((i=1;i<=106;i++));
    do
        port_addr=$(( 16384 + ((i - 1) * 16)))
        hex=$( printf "0x%x" $port_addr )
        python /usr/bin/pcisysfs.py --set --offset $hex --val 0x47 --res $resource  > /dev/null 2>&1
    done
}

#Modsel 106 ports to applicable QSFP type modules
#This enables the adapter to respond for i2c commands
switch_board_modsel() {
        get_boot_type
        if [[ ${BOOT_TYPE} == "fast-reboot" ]]; then
                switch_board_module_reset
        fi
	resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
	for ((i=1;i<=106;i++));
	do
		port_addr=$(( 16384 + ((i - 1) * 16)))
		hex=$( printf "0x%x" $port_addr )
		if [ ${port[$i]} ]
		then
		    val=0x11
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
	r=`/usr/bin/pcisysfs.py  --get --offset 0x00 --res /sys/bus/pci/devices/0000\:04\:00.0/resource0 | sed  '1d; s/.*\(....\)$/\1/; s/\(..\{1\}\)/\1./'`
	r_min=$(echo $r | sed 's/.*\(..\)$/0x\1/')
	r_maj=$(echo $r | sed 's/^\(..\).*/0x\1/')
	echo "FPGA: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	## Get BMC Firmware Revision
	r=`ipmitool mc info | awk '/Firmware Revision/ { print $NF }'`
	echo "BMC: $r" >> $FIRMWARE_VERSION_FILE

	#System CPLD 0x31 on i2c bus 601 ( physical FPGA I2C-2)
	r_min=`/usr/sbin/i2cget -y 601 0x31 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 601 0x31 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "System CPLD: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#Secondary CPLD 1 0x30 on i2c bus 600 ( physical FPGA I2C-1)
	r_min=`/usr/sbin/i2cget -y 600 0x30 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x30 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "Secondary CPLD 1: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#Secondary CPLD 2 0x31 on i2c bus 600 ( physical FPGA I2C-1)
	r_min=`/usr/sbin/i2cget -y 600 0x31 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x31 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "Secondary CPLD 2: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

	#Secondary CPLD 3 0x32 on i2c bus 600 ( physical FPGA I2C-1)
	r_min=`/usr/sbin/i2cget -y 600 0x32 0x0 | sed ' s/.*\(0x..\)$/\1/'`
	r_maj=`/usr/sbin/i2cget -y 600 0x32 0x1 | sed ' s/.*\(0x..\)$/\1/'`
	echo "Secondary CPLD 3: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE
}

#This enables the led control for CPU and default states 
switch_board_led_default() {
	resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
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
    resource="/sys/bus/pci/devices/0000:04:00.0/resource0"

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
PLATFORM_READY_CHECK=/var/run/platform_ready

if [ "$1" == "init" ]; then
    sleep 1
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si kipmid_max_busy_us=3000
    modprobe i2c_ocores
    modprobe dell_s5448f_fpga_ocores
    modprobe mc24lc64t
    sys_eeprom "new_device"
    sleep 1
    # Create a flag file to denote the platform is ready
    touch $PLATFORM_READY_CHECK
    switch_board_qsfp_sfp_mux "new_device"
    switch_board_qsfp "new_device"
    switch_board_sfp "new_device"
    switch_board_sfp "media_down"
    switch_board_modsel
    switch_board_led_default
    install_python_api_package
    #python /usr/bin/qsfp_irq_enable.py
    platform_firmware_versions
    get_reboot_cause
    echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us
elif [ "$1" == "deinit" ]; then
    sys_eeprom "delete_device"
    switch_board_qsfp_sfp_mux "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_sfp "delete_device"
    remove_python_api_package
    # Remove the flag file to denote the platform is no more ready
    rm -f $PLATFORM_READY_CHECK
elif [ "$1" == "media_down" ]; then
    switch_board_sfp $1 "shut_down"
else
     echo "s5448f_platform : Invalid option !"
fi

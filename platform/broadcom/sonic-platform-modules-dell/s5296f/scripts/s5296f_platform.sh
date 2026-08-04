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
    b=''
    for bb in 0 1; do
        if [ "$(cat /sys/bus/i2c/devices/i2c-${bb}/name)" = 'SMBus iSMT adapter at dff9f000' ]; then
            b=$bb
            break
        fi
    done
    if [ "$b" = '' ]; then
        echo "s5296f_platform: sys_eeprom : cannot find I2C bus!"
        return
    fi
    
    case $1 in
        "new_device")    echo 24c16 0x50 > /sys/bus/i2c/devices/i2c-${b}/$1
                         ;;

        "delete_device") echo 0x50 > /sys/bus/i2c/devices/i2c-${b}/$1
                         ;;

        *)               echo "s5296f_platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

#Attach/Detach the MUX connecting all QSFPs
switch_board_qsfp_mux() {
    case $1 in
        "new_device")
            for ((i=603;i<=615;i++));
            do
                echo "Attaching PCA9548 @ 0x74"
                echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;
        
        "delete_device")
            for ((i=603;i<=615;i++));
            do
                echo "Detaching PCA9548 @ 0x74"
                echo 0x74 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;
        
        *)
            echo "s5296f_platform: switch_board_qsfp_mux: invalid command !"
            ;;
    esac
    sleep 2
}

#Attach/Detach 64 instances of EEPROM driver QSFP ports
#eeprom can dump data using below command
switch_board_qsfp() {
    case $1 in
        "new_device")
            for ((i=2;i<=105;i++));
            do
                echo sff8436 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;
        
        "delete_device")
            for ((i=2;i<=105;i++));
            do
                echo 0x50 > /sys/bus/i2c/devices/i2c-$i/$1
            done
            ;;

        *)              echo "s5296f_platform: switch_board_qsfp: invalid command !"
                        ;;
    esac
}

#Modsel 64 ports to applicable QSFP type modules
#This enables the adapter to respond for i2c commands
switch_board_modsel() {
    resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
    for ((i=1;i<=104;i++));
    do
        port_addr=$(( 16384 + ((i - 1) * 16)))
        hex=$( printf "0x%x" $port_addr )
        python /usr/bin/pcisysfs.py --set --offset $hex --val 0x10 --res $resource  > /dev/null 2>&1
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

    #Slave CPLD 2 0x31 on i2c bus 600 ( physical FPGA I2C-1)
    r_min=`/usr/sbin/i2cget -y 600 0x31 0x0 | sed ' s/.*\(0x..\)$/\1/'`
    r_maj=`/usr/sbin/i2cget -y 600 0x31 0x1 | sed ' s/.*\(0x..\)$/\1/'`
    echo "Slave CPLD 2: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

    #Slave CPLD 3 0x32 on i2c bus 600 ( physical FPGA I2C-1)
    r_min=`/usr/sbin/i2cget -y 600 0x32 0x0 | sed ' s/.*\(0x..\)$/\1/'`
    r_maj=`/usr/sbin/i2cget -y 600 0x32 0x1 | sed ' s/.*\(0x..\)$/\1/'`
    echo "Slave CPLD 3: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE

    #Slave CPLD 3 0x32 on i2c bus 600 ( physical FPGA I2C-1)
    r_min=`/usr/sbin/i2cget -y 600 0x33 0x0 | sed ' s/.*\(0x..\)$/\1/'`
    r_maj=`/usr/sbin/i2cget -y 600 0x33 0x1 | sed ' s/.*\(0x..\)$/\1/'`
    echo "Slave CPLD 4: $((r_maj)).$((r_min))" >> $FIRMWARE_VERSION_FILE
}

#This enables the led control for CPU and default states 
switch_board_led_default() {
    resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
    python /usr/bin/pcisysfs.py --set --offset 0x24 --val 0x194 --res $resource  > /dev/null 2>&1
}
init_devnum

if [ "$1" == "init" ]; then
    modprobe i2c-dev
    modprobe i2c-mux-pca954x
    modprobe ipmi_devintf
    modprobe ipmi_si
    modprobe i2c_ocores
    modprobe dell_s5296f_fpga_ocores
    sys_eeprom "new_device"
    switch_board_qsfp_mux "new_device"
    switch_board_qsfp "new_device"
    switch_board_modsel
    switch_board_led_default
    #python /usr/bin/qsfp_irq_enable.py
    platform_firmware_versions
    echo -2 > /sys/bus/i2c/drivers/pca954x/603-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/604-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/605-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/606-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/607-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/608-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/609-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/610-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/611-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/612-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/613-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/614-0074/idle_state
    echo -2 > /sys/bus/i2c/drivers/pca954x/615-0074/idle_state

elif [ "$1" == "deinit" ]; then
    sys_eeprom "delete_device"
    switch_board_qsfp "delete_device"
    switch_board_qsfp_mux "delete_device"

    modprobe -r dell_s5296f_fpga_ocores
    modprobe -r i2c_ocores
    modprobe -r acpi_ipmi
    modprobe -r ipmi_si
    modprobe -r ipmi_devintf
    modprobe -r i2c-mux-pca954x
    modprobe -r i2c-dev
    remove_python_api_package
else
    echo "s5296f_platform : Invalid option !"
fi


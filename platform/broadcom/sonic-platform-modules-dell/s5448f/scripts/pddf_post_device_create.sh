#!/bin/bash

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

#This enables the led control for CPU and default states
switch_board_led_default() {
        resource="/sys/bus/pci/devices/0000:04:00.0/resource0"
        python /usr/bin/pcisysfs.py --set --offset 0x24 --val 0x94 --res $resource  > /dev/null 2>&1
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

switch_board_modsel
switch_board_led_default
platform_firmware_versions
echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us

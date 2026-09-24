#!/bin/bash
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

switch_board_modsel() {
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

#This enables the led control for CPU and default states
switch_board_led_default() {
    resource="/sys/bus/pci/devices/0000:0$fpga_busnum:00.0/resource0"
    python /usr/bin/pcisysfs.py --set --offset 0x24 --val 0x94 --res $resource  > /dev/null 2>&1
}

# Readout firmware version of the system and
# store in /var/log/firmware_versions
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

init_fpga_busnum
switch_board_modsel
switch_board_led_default
platform_firmware_versions
echo 3000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us



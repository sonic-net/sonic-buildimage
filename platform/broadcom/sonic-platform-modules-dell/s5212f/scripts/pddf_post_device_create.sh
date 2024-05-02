#!/bin/bash
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

switch_board_modsel
switch_board_led_default
platform_firmware_versions
echo 1000 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us


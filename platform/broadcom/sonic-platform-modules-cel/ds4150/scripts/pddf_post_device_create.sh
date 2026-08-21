#!/bin/bash

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE_SYS_PATH="/sys/bus/platform/devices/sys_cpld/bmc_present"
BMC_PRESENCE=`cat ${BMC_PRESENCE_SYS_PATH}`
ker_name=$(uname -r)
driver_path=/usr/lib/modules/${ker_name}/extra/
imc_driver=i2c-imc.ko

modprobe pddf_custom_fpga_extend

# Add read permission for tlv eeprom
tlv_eeprom_path='/sys/bus/i2c/devices/i2c-0/0-0056/eeprom'
if [ -e ${tlv_eeprom_path} ]; then
        sudo chmod a+r ${tlv_eeprom_path}
        echo "Add read permission for tlv eeprom"
fi

# Reset management PHY and switch to get IP address from DHCP server
#Write 0x00 to reg 0xa05 of baseboard cpld, wait mayb 0.1~0.5 sec then write 0xff to reg 0xa05
echo 0xa05 0x00 >/sys/devices/platform/sys_cpld/setreg
sleep 0.5
echo 0xa05 0xff >/sys/devices/platform/sys_cpld/setreg

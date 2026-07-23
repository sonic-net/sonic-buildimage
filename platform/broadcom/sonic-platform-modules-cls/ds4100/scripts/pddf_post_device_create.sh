#!/bin/bash

# Get BMC mode
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
BMC_PRESENCE_SYS_PATH="/sys/bus/platform/devices/sys_cpld/bmc_present"
BMC_PRESENCE=`cat ${BMC_PRESENCE_SYS_PATH}`
ker_name=$(uname -r)
driver_path=/usr/lib/modules/${ker_name}/extra/
imc_driver=i2c-imc.ko

# none bmc
if [ ${BMC_PRESENCE} == "0" ]; then

	# enable security_mode for ucd90160
	#security_mode=$(i2ctransfer -y 8 w1@0x34 0xf1 r7)
	#array=(${security_mode// / })
	#security_mode=${array[6]}
	#if [ ${security_mode} == "0x00" ]; then 
	#		i2cset -f -y 8 0x34 0xf1 0x06 0x31 0x32 0x33 0x34 0x35 0x36 i
	#		i2cset -f -y 8 0x34 0x11
	#fi 

    # attach ucdxxxx devices, bus 8, address 0x34 and 0x35
    #echo ucd90160 0x34 > /sys/bus/i2c/devices/i2c-8/new_device
    #echo ucd90120 0x35 > /sys/bus/i2c/devices/i2c-8/new_device

    #insmod ads7142 driver
    modprobe ti-ads7142 g_chn_count=2
    sleep 1

    # attach mp5023 device, bus 8
    echo mp5023   0x40 > /sys/bus/i2c/devices/i2c-8/new_device
    # attach ads7142 device, bus 57
    echo ads7142  0x1b > /sys/bus/i2c/devices/i2c-57/new_device
fi
modprobe pddf_custom_fpga_extend
sleep 2
if [ "${BMC_PRESENCE}" = "0" ]; then
    if [ -e ${driver_path}${imc_driver}-bk ]; then
        mv ${driver_path}${imc_driver}-bk ${driver_path}${imc_driver}
        sleep 3
        echo "Insmod Intel iMC SMBus Controller driver"
        insmod ${driver_path}${imc_driver}
    fi
fi

# Add read permission for tlv eeprom
tlv_eeprom_path='/sys/bus/i2c/devices/i2c-0/0-0056/eeprom'
if [ -e ${tlv_eeprom_path} ]; then
        sudo chmod a+r ${tlv_eeprom_path}
        echo "Add read permission for tlv eeprom"
fi

# Set default of the PCA9535 is to have all IO configured as inputs, to reset OSFP at start up
function set_pca9535_registers(){
    local BUS_1=13
    local BUS_2=14
    local VALUE=0x00
    local OUTPUT_REG0=0x02
    local CONFIG_REG0=0x06
    local ADDR_RANGE_1=(0x22 0x23)
    local ADDR_RANGE_2=(0x24 0x25)

   # Set the initial state of output reg0 to 0x00
    for addr in "${ADDR_RANGE_1[@]}"; do
        sudo i2cset -f -y $BUS_1 $addr $OUTPUT_REG0 $VALUE
    done

    for addr in "${ADDR_RANGE_2[@]}"; do
        sudo i2cset -f -y $BUS_2 $addr $OUTPUT_REG0 $VALUE
    done

    # Set the initial state of config reg0 to output mode
    for addr in "${ADDR_RANGE_1[@]}"; do
        sudo i2cset -f -y $BUS_1 $addr $CONFIG_REG0 $VALUE
    done

    for addr in "${ADDR_RANGE_2[@]}"; do
        sudo i2cset -f -y $BUS_2 $addr $CONFIG_REG0 $VALUE
    done
}

set_pca9535_registers

#Set SYS LED status to Green
if [ "${BMC_PRESENCE}" = "0" ]; then
    i2cset -y -f 6 0x0d 0x62 0xdc
else
    ipmitool raw 0x3a 0x43 0x01 0x02 0x01 0x00
fi

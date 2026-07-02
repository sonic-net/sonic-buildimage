#!/bin/bash

# install custom fpga device
sleep 3

modprobe pddf_custom_fpga_extend

# install Intel iMC SMBus Controller driver with nobmc
pddf_device_path='/usr/share/sonic/platform/pddf/pddf-device.json'
ker_name=$(uname -r)
driver_path=/usr/lib/modules/${ker_name}/extra/
imc_driver=i2c-imc.ko
bmc_present=$(grep -oP '"bmc_present" : "\K[^"]+' $pddf_device_path)

if [ "$bmc_present" = "False" ]; then
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

function set_pca9535_registers(){
    local BUS_1=13
    local BUS_2=14
    local VALUE=0x00
    local OUTPUT_REG0=0x02
    local CONFIG_REG0=0x06
    local ADDR_RANGE_1=(0x20 0x21 0x22 0x23)
    local ADDR_RANGE_2=(0x24 0x25 0x26 0x27)

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
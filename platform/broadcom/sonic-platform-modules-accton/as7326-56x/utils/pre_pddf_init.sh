#!/bin/bash

# Steps to check syseeprom i2c address
modprobe i2c-i801
modprobe i2c-dev
use_57_eeprom=true
i2cget -y -f 0 0x56 0x0
if [ $? -eq 0 ]; then
    use_57_eeprom=false
    echo "The board has system EEPROM at I2C address 0x56."
fi

if $use_57_eeprom ; then
    echo "The board has system EEPROM at I2C address 0x57."
    # syseeprom is at the i2c address 0x57. Change the PDDF JSON file
    if [ -f /usr/share/sonic/device/x86_64-accton_as7326_56x-r0/pddf_support ] && \
       [ -f /usr/share/sonic/device/x86_64-accton_as7326_56x-r0/pddf/pddf-device.json ]; then
        sed -i 's@"topo_info": {"parent_bus": "0x0", "dev_addr": "0x56", "dev_type": "24c04"},@\
            "topo_info": {"parent_bus": "0x0", "dev_addr": "0x57", "dev_type": "24c02"},@g' \
            /usr/share/sonic/device/x86_64-accton_as7326_56x-r0/pddf/pddf-device.json
        sync
    fi
fi

#!/bin/bash

# Steps to check syseeprom i2c address
modprobe i2c-i801
modprobe i2c-dev
at_id=false

(i2cset -y -f 0 0x77 0x1) > /dev/null 2>&1
(i2cset -y -f 0 0x76 0x4) > /dev/null 2>&1


(i2cget -y -f 0 0x60 0xfd) > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "get at_id"

    at_1=$(i2cget -y -f 0 0x60 0xfd)

    if [[ ${at_1} -eq 0x41 ]]; then
        echo "get at_1 is 0x41"
        at_2=$(i2cget -y -f 0  0x60 0xfe)

        if [[ ${at_2} -eq 0x54 ]]; then
            echo "get at_2 is 0x54"
            at_id=true
        fi
    else
        cpld_id=$(i2cget -y -f 0 0x60 0x1) #This CPD version that not support to record at_id
        if [[ ${cpld_id} -eq 0x5 ]]; then
            at_id=true
        fi

    fi
fi

if $at_id ; then
    echo "This CPLD is for AT used PSU"
    echo "PSU-1 pmbus use parent_bus:0x9, dev_addr:0x58"
    echo "PSU-1 eeprom use parent_bus:0x9, dev_addr:0x50"
    echo "PSU-2 pmbus use parent_bus:0xa, dev_addr:0x5b"
    echo "PSU-2 eeprom use parent_bus:0xa, dev_addr:0x53"

    if [ -f /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf_support ] && \
       [ -f /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json ]; then
        #Change the PDDF JSON file
        sed -i 's@{ "chn":"0", "dev":"PSU2"},@\
                 { "chn":"0", "dev":"PSU1" },@g' \
        /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "chn":"1", "dev":"PSU1"}@\
                 { "chn":"1", "dev":"PSU2" }@g' \
        /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync

        sed -i 's@"topo_info":{ "parent_bus":"0xa", "dev_addr":"0x5b", "dev_type":"psu_pmbus"},@\
            "topo_info": {"parent_bus":"0x9", "dev_addr":"0x58" , "dev_type": "psu_pmbus" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_model_name", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x9a", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10"},@\
                 { "attr_name":"psu_model_name", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x9a", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json    
        sync
        sed -i 's@{ "attr_name":"psu_mfr_id", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0X99", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10"},@\
                 { "attr_name":"psu_mfr_id", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0X99", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_fan_dir", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0xc3", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"5"},@\
                 { "attr_name":"psu_fan_dir", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0xc3", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"5" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json    
        sync
        sed -i 's@{ "attr_name":"psu_v_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8b", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_v_out", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x8b", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json    
        sync
        sed -i 's@{ "attr_name":"psu_i_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8c", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_i_out", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x8c", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_p_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x96", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_p_out", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x96", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_fan1_speed_rpm", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x90", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_fan1_speed_rpm", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x90", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_temp1_input", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8d", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"}@\
                 { "attr_name":"psu_temp1_input", "attr_devaddr":"0x58" , "attr_devtype":"pmbus", "attr_offset":"0x8d", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" }@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync

        sed -i 's@"topo_info":{ "parent_bus":"0xa", "dev_addr":"0x53", "dev_type":"psu_eeprom"},@\
                "topo_info": {"parent_bus":"0x9", "dev_addr":"0x50" , "dev_type": "psu_eeprom" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync


        sed -i 's@"topo_info": { "parent_bus":"0x9", "dev_addr":"0x58", "dev_type":"psu_pmbus"},@\
                "topo_info": {"parent_bus":"0xa", "dev_addr":"0x5b", "dev_type": "psu_pmbus" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync

        sed -i 's@{ "attr_name":"psu_model_name", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x9a", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10"},@\
                 { "attr_name":"psu_model_name", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x9a", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync

        sed -i 's@{ "attr_name":"psu_mfr_id", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0X99", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10"},@\
                 { "attr_name":"psu_mfr_id", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0X99", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"10" },@g' \
             /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_fan_dir", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0xc3", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"5"},@\
                 { "attr_name":"psu_fan_dir", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0xc3", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"5" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_v_out", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x8b", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_v_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8b", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_i_out", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x8c", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_i_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8c", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_p_out", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x96", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_p_out", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x96", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_fan1_speed_rpm", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x90", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"},@\
                 { "attr_name":"psu_fan1_speed_rpm", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x90", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
        sed -i 's@{ "attr_name":"psu_temp1_input", "attr_devaddr":"0x58", "attr_devtype":"pmbus", "attr_offset":"0x8d", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2"}@\
                 { "attr_name":"psu_temp1_input", "attr_devaddr":"0x5b", "attr_devtype":"pmbus", "attr_offset":"0x8d", "attr_mask":"0x0", "attr_cmpval":"0xff", "attr_len":"2" }@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync

        sed -i 's@"topo_info":{ "parent_bus":"0x9", "dev_addr":"0x50", "dev_type":"psu_eeprom"},@\
                 "topo_info": {"parent_bus":"0xa", "dev_addr":"0x53", "dev_type": "psu_eeprom" },@g' \
            /usr/share/sonic/device/x86_64-accton_as7816_64x-r0/pddf/pddf-device.json
        sync
    fi
fi

(i2cset -y -f 0 0x76 0x0) > /dev/null 2>&1
(i2cset -y -f 0 0x77 0x0) > /dev/null 2>&1
echo "Pre PDDF init steps completed successully"

#For others, psu i2c bus use below,
#PSU-1 pmbus  use "parent_bus":"0xa", "dev_addr":"0x5b"
#PSU-1 eeprom use "parent_bus":"0xa", "dev_addr":"0x53"
#PSU-2 pmbus  use "parent_bus":"0x9", "dev_addr":"0x58"
#PSU-2 eeprom use "parent_bus":"0x9", "dev_addr":"0x50"

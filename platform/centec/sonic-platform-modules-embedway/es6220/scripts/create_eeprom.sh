#! /bin/bash

#find cache of eeprom and delete, otherwise will dislay cache
cache_file="/var/cache/sonic/decode-syseeprom/syseeprom_cache"
if [ -f $cache_file ]; then
    rm -f $cache_file
fi
    
#create new device for eeprom
tmp=`i2cdetect -l | grep CP` 
bus_num=${tmp:4:1}

path="/sys/bus/i2c/devices/i2c-${bus_num}"

cd $path
echo "24c512 0x56" > new_device

sudo dd if=/sys/bus/i2c/devices/${bus_num}-0056/eeprom of=/home/admin/eeprom.bin bs=1 count=512

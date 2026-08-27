#!/bin/bash
modprobe -r i2c-ismt
sleep 0.1
modprobe -r i2c-i801

ker_name=$(uname -r)

# Has customized those drivers,so rename them to lose effect
psu_driver=pddf_psu_driver_module.ko
driver_path=/usr/lib/modules/${ker_name}/extra/
if [ -e ${driver_path}${psu_driver} ]; then
    mv ${driver_path}${psu_driver} ${driver_path}${psu_driver}-bk
fi

echo 'pddf psu driver module has rename now'

# Prevent i2c-imc registering i2C first, causing i2C conflict
#Its removed in device, after validation this has to be done in code
imc_driver=i2c-imc.ko
if [ -e ${driver_path}${imc_driver} ]; then
    mv ${driver_path}${imc_driver} ${driver_path}${imc_driver}-bk
fi

sync

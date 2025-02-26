#!/bin/bash

# Has customized those drivers,so rename them to lose effect
psu_driver=pddf_psu_driver_module.ko
ker_name=$(uname -r)
driver_path=/usr/lib/modules/${ker_name}/extra/
if [ -e ${driver_path}${psu_driver} ]; then
    mv ${driver_path}${psu_driver} ${driver_path}${psu_driver}-bk
fi

echo 'pddf psu driver module has rename now'

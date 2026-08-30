#!/bin/bash

# Has customized those drivers,so rename them to lose effect
ker_name=$(uname -r)

pddf_psu_driver="/usr/lib/modules/${ker_name}/extra/pddf_psu_driver_module.ko"
if [ -e ${pddf_psu_driver} ]; then
    mv ${pddf_psu_driver} ${pddf_psu_driver}-bk
fi
echo "${pddf_psu_driver} driver module has rename now"

sync

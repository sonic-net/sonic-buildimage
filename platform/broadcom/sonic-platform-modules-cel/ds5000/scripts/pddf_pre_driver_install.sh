#!/bin/bash

touch /usr/local/sdr_dump

# Has customized those drivers,so rename them to lose effect
ker_name=$(uname -r)

i2c_mux_pca954x_driver="/lib/modules/${ker_name}/kernel/drivers/i2c/muxes/i2c-mux-pca954x.ko"
if [ -e ${i2c_mux_pca954x_driver} ]; then
    mv ${i2c_mux_pca954x_driver} ${i2c_mux_pca954x_driver}-bk
fi
echo "${i2c_mux_pca954x_driver} driver module has rename now"

sync

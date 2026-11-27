#!/bin/bash

# Has customized those drivers,so rename them to prevent loading
kmod_path=/lib/modules/$(uname -r)
kmod_extra_path=${kmod_path}/extra

i2c_mux_pca954x_driver="${kmod_path}/kernel/drivers/i2c/muxes/i2c-mux-pca954x.ko"
if [ -e ${i2c_mux_pca954x_driver} ]; then
    mv ${i2c_mux_pca954x_driver} ${i2c_mux_pca954x_driver}-bk
fi
echo "Suppress ${i2c_mux_pca954x_driver} module now."

xcvr_driver="${kmod_extra_path}/pddf_xcvr_driver_module.ko"
if [ -e ${xcvr_driver} ]; then
    mv ${xcvr_driver} ${xcvr_driver}-bk
fi

echo "suppress pddf's xcvr driver module now."

sync

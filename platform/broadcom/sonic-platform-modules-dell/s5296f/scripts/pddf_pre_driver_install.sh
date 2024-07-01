#!/bin/bash
sys_eeprom "delete_device"
switch_board_qsfp "delete_device"
switch_board_sfp "delete_device"
switch_board_qsfp_mux "delete_device"

modprobe -r dell_s5296f_fpga_ocores
modprobe -r i2c_ocores
modprobe -r i2c_ismt 
modprobe -r i2c-i801 
modprobe -r acpi_ipmi
modprobe -r ipmi_si
modprobe -r ipmi_devintf
modprobe -r i2c-mux-pca954x
modprobe -r i2c-dev


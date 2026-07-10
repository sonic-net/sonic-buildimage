#!/bin/bash
modprobe -r dell_z9664f_fpga_ocores 
modprobe -r mc24lc64t
modprobe -r i2c_ismt 
modprobe -r i2c-i801

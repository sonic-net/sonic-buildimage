#!/bin/sh

echo 1 > /sys/bus/i2c/devices/0-0033/poe_en_ctrl

# Set temporary matrix
#poetool port set_temp_matrix 0 33 255
#poetool port set_temp_matrix 1 38 255
#poetool port set_temp_matrix 2 34 255
#poetool port set_temp_matrix 3 37 255
#poetool port set_temp_matrix 4 35 255
#poetool port set_temp_matrix 5 36 255
#poetool port set_temp_matrix 6 39 255
#poetool port set_temp_matrix 7 32 255
#poetool port set_temp_matrix 8 47 255
#poetool port set_temp_matrix 9 40 255
#poetool port set_temp_matrix 10 44 255
#poetool port set_temp_matrix 11 43 255
#poetool port set_temp_matrix 12 45 255
#poetool port set_temp_matrix 13 42 255
#poetool port set_temp_matrix 14 46 255
#poetool port set_temp_matrix 15 41 255
#poetool port set_temp_matrix 16 49 255
#poetool port set_temp_matrix 17 54 255
#poetool port set_temp_matrix 18 50 255
#poetool port set_temp_matrix 19 53 255
#poetool port set_temp_matrix 20 52 255
#poetool port set_temp_matrix 21 51 255
#poetool port set_temp_matrix 22 48 255
#poetool port set_temp_matrix 23 55 255
#poetool port set_temp_matrix 24 63 255
#poetool port set_temp_matrix 25 56 255
#poetool port set_temp_matrix 26 60 255
#poetool port set_temp_matrix 27 59 255
#poetool port set_temp_matrix 28 61 255
#poetool port set_temp_matrix 29 58 255
#poetool port set_temp_matrix 30 62 255
#poetool port set_temp_matrix 31 57 255
#poetool port set_temp_matrix 32 5 2
#poetool port set_temp_matrix 33 4 3
#poetool port set_temp_matrix 34 7 0
#poetool port set_temp_matrix 35 6 1
#poetool port set_temp_matrix 36 9 14
#poetool port set_temp_matrix 37 8 15
#poetool port set_temp_matrix 38 11 12
#poetool port set_temp_matrix 39 10 13
#poetool port set_temp_matrix 40 21 18
#poetool port set_temp_matrix 41 20 19
#poetool port set_temp_matrix 42 23 16
#poetool port set_temp_matrix 43 22 17
#poetool port set_temp_matrix 44 25 30
#poetool port set_temp_matrix 45 24 31
#poetool port set_temp_matrix 46 27 28
#poetool port set_temp_matrix 47 26 29
#
## PSUx2 = {power bank 3}; PSUx1 = {power bank 1/2}
## For 220v PSUx2
##poe_main 0x09 3 2185 570 440
## For 220v PSUx1
##poe_main 0x09 1 1410 570 440
##poe_main 0x09 2 1410 570 440
#
## For 110v PSUx2
#poetool mgmt set_power_banks 3 1810 570 440 0xa
## For 110v PSUx1
#poetool mgmt set_power_banks 1 810 570 440 0xa
#poetool mgmt set_power_banks 2 810 570 440 0xa
#
## Program global matrix
#poetool port program_global_matrix
## Save system settings
#poetool system save_setting

# PoE power bank setting
poe_power.sh
# reload PoE configuration
#poe_cfg_init.py


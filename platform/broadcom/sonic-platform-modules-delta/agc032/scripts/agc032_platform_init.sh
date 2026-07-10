#!/bin/bash

#platform init script for Delta agc032

#fan speed monitor start
/usr/share/sonic/device/x86_64-delta_agc032-r0/fancontrol.service /usr/share/sonic/device/x86_64-delta_agc032-r0/fancontrol &

# On Delta-agc032 platform,
# syseeprom is on the different mux with main board after CPLD,
# we have to switch CPLD mux from main board to syseeprom before getting syseeprom data.
#
# The purpose of switching CPLD mux and getting syseeprom information here
# is to store data in syseeprom_cache.
# After that, we can read "syseeprom_cache" directly when executing command
# "show platform syseeprom", "sudo decode-syseeprom", "show version".

echo 0 |sudo tee /sys/devices/platform/delta-agc032-cpupld.0/cpu_i2c_mux_sel
decode-syseeprom
echo 1 |sudo tee /sys/devices/platform/delta-agc032-cpupld.0/cpu_i2c_mux_sel

exit 0


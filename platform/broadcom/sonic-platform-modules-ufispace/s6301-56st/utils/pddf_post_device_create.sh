#!/bin/bash
echo "Reset port led"
echo 0 > /sys/devices/platform/x86_64_ufispace_s6301_56st_lpc//mb_cpld/port_led_clear
sleep 0.5
echo 1 > /sys/devices/platform/x86_64_ufispace_s6301_56st_lpc//mb_cpld/port_led_clear

curr_led=$(pddf_ledutil getstatusled SYS_LED)
pddf_ledutil setstatusled SYS_LED green
echo "Set System $curr_led to green" 

echo "PDDF device post-create completed"

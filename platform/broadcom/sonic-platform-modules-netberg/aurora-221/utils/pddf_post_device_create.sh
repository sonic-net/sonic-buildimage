#!/bin/bash
curr_led=$(pddf_ledutil getstatusled SYS_LED)
pddf_ledutil setstatusled SYS_LED green
echo "Set System $curr_led to green" 

echo "PDDF device post-create completed"

#!/bin/bash
#This script is used to disable SMF Watchdog Timer

wd_status=-1
enabled=0

# Disable Watchdog if enabled
wd_status=$(io_rd_wr.py  --get --offset 0x207 | cut -d " " -f3)

if [[ $wd_status -eq $enabled ]]; then
    echo "Disabling Watchdog Timer.."
    io_rd_wr.py --set --val 1 --offset 0x207
fi

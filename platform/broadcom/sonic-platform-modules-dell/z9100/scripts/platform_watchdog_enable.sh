#!/bin/bash
#This script is used to enable SMF Watchdog Timer

# Set watchdog to 180 seconds
io_rd_wr.py --set --val 3 --offset 0x206

# Enable watchdog timer
io_rd_wr.py --set --val 0 --offset 0x207

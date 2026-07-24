#!/bin/bash
#This script is used to power off IO modules
# IOM can be controlled via SMF using mailbox registers 
# write 0x2 to 0x04D9 to power off IOM 1
io_rd_wr.py --set --val 0x04 --offset 0x210
io_rd_wr.py --set --val 0xd9 --offset 0x211
io_rd_wr.py --set --val 0x2 --offset 0x213
# write 0x2 to 0x04DA to power off IOM 2
io_rd_wr.py --set --val 0xda --offset 0x211
io_rd_wr.py --set --val 0x2 --offset 0x213
# write 0x2 to 0x04DB to power off IOM 3
io_rd_wr.py --set --val 0xdb --offset 0x211
io_rd_wr.py --set --val 0x2 --offset 0x213
# write 0x2 to 0x04DC to power off IOM 4
io_rd_wr.py --set --val 0xdc --offset 0x211
io_rd_wr.py --set --val 0x2 --offset 0x213
